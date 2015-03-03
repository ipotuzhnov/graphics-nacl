#include "url_download_stream.h"

UrlDownloadStream::UrlDownloadStream() {
	pool_ = DataPool::create();
}

void UrlDownloadStream::addData(const void *data, int size) {
	pool_->add_data(data, size);
}

void UrlDownloadStream::close() {
	// Check file size
	if (pool_->get_length() <= 0) {
		error_ = "Error: No data was delivered.";
		return;
	} else {
		if (!pool_->has_data(0, pool_->get_length())) {
			error_ = "Error: Not all data was delivered.";
			return;
		}
	}
	pool_->set_eof();
}

GP<DataPool> UrlDownloadStream::getPool() {
	return pool_;
}

std::string UrlDownloadStream::getError() {
	return error_;
}
