// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "ppapi/c/pp_errors.h"
#include "ppapi/c/ppb_instance.h"
#include "ppapi/cpp/module.h"
#include "ppapi/cpp/var.h"

#include "url_loader_handler.h"
#include "../helpers/message_helper.h"

#ifdef WIN32
#undef min
#undef max
#undef PostMessage

// Allow 'this' in initializer list
#pragma warning(disable : 4355)
#endif

URLLoaderHandler* URLLoaderHandler::Create(pp::Instance* instance, const std::string& url, std::shared_ptr<UrlDownloadStream> stream) {
	return new URLLoaderHandler(instance, url, stream);
}

URLLoaderHandler::URLLoaderHandler(pp::Instance* instance, const std::string& url, std::shared_ptr<UrlDownloadStream> stream)
	: instance_(instance),
	url_(url),
	url_request_(instance),
	url_loader_(instance),
	buffer_(new char[READ_BUFFER_SIZE]),
	download_progress_(0),
	bytes_received_(0),
	total_bytes_to_be_received_(0),
	stream_(stream),
	cc_factory_(this) {
		url_request_.SetURL(url);
		url_request_.SetMethod("GET");
		url_request_.SetRecordDownloadProgress(true);
}

URLLoaderHandler::~URLLoaderHandler() {
	delete[] buffer_;
	buffer_ = NULL;
}

void URLLoaderHandler::Start() {
	pp::CompletionCallback cc =
		cc_factory_.NewCallback(&URLLoaderHandler::OnOpen);
	url_loader_.Open(url_request_, cc);
}

void URLLoaderHandler::OnOpen(int32_t result) {
	if (result != PP_OK) {
		ReportResultAndDie(url_, "pp::URLLoader::Open() failed" + url_, false);
		return;
	}
	// Here you would process the headers. A real program would want to at least
	// check the HTTP code and potentially cancel the request.
	// pp::URLResponseInfo response = loader_.GetResponseInfo();
	pp::URLResponseInfo response = url_loader_.GetResponseInfo();
	if (response.is_null()) {
		// @TODO (ilia) create appropriative error later
		//pp::Var var_reply("Error: response is null");
		//instance_->PostMessage(var_reply);

		ReportResultAndDie(url_, "Error: response is null", false);
		return;
	}
	int32_t status = response.GetStatusCode();
	// @TODO (ilia) create appropriative error later
	std::string status_line = "Status: " + std::to_string(status) + " line: " + response.GetStatusLine().AsString();
	if (status != 200) {
		ReportResultAndDie(url_, status_line, false);
		return;
	}


	// Try to figure out how many bytes of data are going to be downloaded in
	// order to allocate memory for the response body in advance (this will
	// reduce heap traffic and also the amount of memory allocated).
	// It is not a problem if this fails, it just means that the
	// url_response_body_.insert() call in URLLoaderHandler::AppendDataBytes()
	// will allocate the memory later on.
	int64_t bytes_received = 0;
	int64_t total_bytes_to_be_received = 0;
	if (url_loader_.GetDownloadProgress(&bytes_received, &total_bytes_to_be_received)) {
		if (total_bytes_to_be_received > 0) {
			url_response_body_.reserve(total_bytes_to_be_received);
		}
		bytes_received_ = bytes_received;
		total_bytes_to_be_received_ = total_bytes_to_be_received;
		int new_download_progress = 100 * bytes_received / total_bytes_to_be_received;
		if (new_download_progress > download_progress_) {
			download_progress_ = new_download_progress;			
			// @TODO (ilia) clean it later
			//std::string download_progress_str = "browser:notify:download:" + std::to_string(download_progress_);
			//pp::Var var_reply(download_progress_str);
			//instance_->PostMessage(var_reply);
			PostMessageToInstance(instance_, CreateDictionaryReply("browser", "notify", "download", download_progress_));
		}
	}
	// We will not use the download progress anymore, so just disable it.
	url_request_.SetRecordDownloadProgress(false);

	// Start streaming.
	ReadBody();
}

void URLLoaderHandler::AppendDataBytes(const char* buffer, int32_t num_bytes) {
	if (num_bytes <= 0)
		return;
	// Make sure we don't get a buffer overrun.
	num_bytes = std::min(READ_BUFFER_SIZE, num_bytes);
	// Note that we do *not* try to minimally increase the amount of allocated
	// memory here by calling url_response_body_.reserve().  Doing so causes a
	// lot of string reallocations that kills performance for large files.
	url_response_body_.insert(
		url_response_body_.end(), buffer, buffer + num_bytes);
	stream_->addData(buffer, num_bytes);

	// Update download progress
	bytes_received_ += num_bytes;
	int new_download_progress = 100 * bytes_received_ / total_bytes_to_be_received_;
	if (new_download_progress > download_progress_) {
		download_progress_ = new_download_progress;
		// @TODO (ilia) clean it later
		//std::string download_progress_str = "browser:notify:download:" + std::to_string(download_progress_);
		//pp::Var var_reply(download_progress_str);
		//instance_->PostMessage(var_reply);
		PostMessageToInstance(instance_, CreateDictionaryReply("browser", "notify", "download", download_progress_));
	}
}

void URLLoaderHandler::OnRead(int32_t result) {
	if (result == PP_OK) {
		// Streaming the file is complete, delete the read buffer since it is
		// no longer needed.
		delete[] buffer_;
		buffer_ = NULL;
		ReportResultAndDie(url_, url_response_body_, true);
	} else if (result > 0) {
		// The URLLoader just filled "result" number of bytes into our buffer.
		// Save them and perform another read.
		AppendDataBytes(buffer_, result);
		ReadBody();
	} else {
		// A read error occurred.
		ReportResultAndDie(
			url_, "pp::URLLoader::ReadResponseBody() result<0", false);
	}
}

void URLLoaderHandler::ReadBody() {
	// Note that you specifically want an "optional" callback here. This will
	// allow ReadBody() to return synchronously, ignoring your completion
	// callback, if data is available. For fast connections and large files,
	// reading as fast as we can will make a large performance difference
	// However, in the case of a synchronous return, we need to be sure to run
	// the callback we created since the loader won't do anything with it.
	pp::CompletionCallback cc =
		cc_factory_.NewOptionalCallback(&URLLoaderHandler::OnRead);
	int32_t result = PP_OK;
	do {
		result = url_loader_.ReadResponseBody(buffer_, READ_BUFFER_SIZE, cc);
		// Handle streaming data directly. Note that we *don't* want to call
		// OnRead here, since in the case of result > 0 it will schedule
		// another call to this function. If the network is very fast, we could
		// end up with a deeply recursive stack.
		if (result > 0) {
			AppendDataBytes(buffer_, result);
		}
	} while (result > 0);

	if (result != PP_OK_COMPLETIONPENDING) {
		// Either we reached the end of the stream (result == PP_OK) or there was
		// an error. We want OnRead to get called no matter what to handle
		// that case, whether the error is synchronous or asynchronous. If the
		// result code *is* COMPLETIONPENDING, our callback will be called
		// asynchronously.
		cc.Run(result);
	}
}

void URLLoaderHandler::ReportResultAndDie(const std::string& fname, const std::string& text, bool success) {
	stream_->close();
	ReportResult(fname, text, success);
	delete this;
}

void URLLoaderHandler::ReportResult(const std::string& fname, const std::string& text, bool success) {
	std::string out;
	if (success) {
		//printf("URLLoaderHandler::ReportResult(Ok).\n");
		out = "URLLoaderHandler::ReportResult(Ok).\n";
		out = "browser:notify:download:finished";

		PostMessageToInstance(instance_, CreateDictionaryReply("browser", "notify", "download", "finished"));
		//OutputDebugStringA(out.c_str());
	} else {
		//printf("URLLoaderHandler::ReportResult(Err). %s\n", text.c_str());
		out = "URLLoaderHandler::ReportResult(Err). %s\n" + text;
		out = "browser:notify:download:error";

		PostErrorMessage(instance_, text);
		PostMessageToInstance(instance_, CreateDictionaryReply("browser", "notify", "download", "error"));
		//OutputDebugStringA(out.c_str());
	}
	/*
	//fflush(stdout);
	if (instance_) {
		//pp::Var var_result(fname + "\n");
		pp::Var var_result(out.c_str());
		instance_->PostMessage(var_result);
	}
	*/
}
