#include <algorithm>
#include <cmath>
#include <limits>
#include <string>
#include <vector>
#include <utility>
#include <map>
#include <sstream>

#include "ppapi/cpp/image_data.h"
#include "ppapi/cpp/input_event.h"
#include "ppapi/cpp/instance.h"
#include "ppapi/cpp/module.h"
#include "ppapi/cpp/point.h"
#include "ppapi/cpp/var.h"
#include "ppapi/cpp/var_array.h"
#include "ppapi/cpp/var_dictionary.h"
#include "ppapi/utility/completion_callback_factory.h"

#include "../helpers/messages.h"
#include "../helpers/message_helper.h"
#include "../loader/url_loader_handler.h"
#include "decoder.h"

#ifdef WIN32
#undef PostMessage
// Allow 'this' in initializer list
#pragma warning(disable : 4355)
#endif
#undef mbstate_t

class GraphicsInstance: public pp::Instance {
private:
	uint32_t color_;
	pp::Size size_;
	std::shared_ptr<UrlDownloadStream> stream_;
	std::shared_ptr<DjVuDecoder> decoder_;

	// Arguments
	std::map<std::string, std::string> args_;

	void ParseArgs(uint32_t argc, const char* argn[], const char* argv[]) {
		for (uint32_t i = 0; i < argc; ++i) {
			std::string tag = std::string(argn[i]);
			std::string value = std::string(argv[i]);
			args_[tag] = value;
		}
	}

	void DownloadStart() {
		std::string url;
		for (auto it = args_.begin(); it != args_.end(); ++it) {
			if (it->first == "docsrc") {
				url = it->second;
			}
		}
		// @TODO (ilia) create appropriative error later
		if (url.empty()) { //@TODO (ilia) return erorr here
			PostErrorMessage(this, "Can't download file: <docsrc> is empty");
			return;
		}

		stream_ = std::make_shared<UrlDownloadStream>();
		URLLoaderHandler* handler = URLLoaderHandler::Create(this, url, stream_);

		if (handler != NULL) {
			// Starts asynchronous download. When download is finished or when an
			// error occurs, |handler| posts the results back to the browser
			// vis PostMessage and self-destroys.
			handler->Start();
		}
	}

	void DecodeStart() {
		decoder_->startDocumentDecode(this, stream_);
	}

	void DecodePage(pp::VarDictionary page) {
		decoder_->startPageDecode(page.Get("pageId").AsString(), page.Get("pageNum").AsInt(), page.Get("width").AsInt(), page.Get("height").AsInt());
	}

	void SendPage(std::string pageId) {
		decoder_->sendPage(pageId);
	}

	void ReleasePage(std::string pageId) {
		decoder_->releasePage(pageId);
	}


public:
	explicit GraphicsInstance(PP_Instance instance)
		: pp::Instance(instance),
		decoder_(std::make_shared<DjVuDecoder>()),
		size_(0, 0) {}

	virtual bool Init(uint32_t argc, const char* argn[], const char* argv[]) {
		ParseArgs(argc, argn, argv);
		return true;
	}

	// Handle message from JavaScript
	virtual void HandleMessage(const pp::Var& var_message) {		
		if (!var_message.is_dictionary()) {
			// @TODO (ilia) create appropriative error later
			PostErrorMessage(this, "Module doesn't understand this type of messages. Only dictionaries!");
			return;
		}

		pp::VarDictionary dictionary_message(var_message);
		// @TODO (ilia) create appropriative error later
		//if ( ! IsMessageValid(dictionary_message) )
		//	return;

		std::string message = dictionary_message.Get("message").AsString();
		pp::Var message_args = dictionary_message.Get("args");
		if (message == PPD_DOWNLOAD_START) {
			DownloadStart();
		} else if (message == PPD_DECODE_START) {
			DecodeStart();
		} else if (message == PPD_DECODE_PAGE) {
			if ( ! message_args.is_dictionary() )
				return; // TODO (ilia) error
			DecodePage(pp::VarDictionary(message_args));
		} else if (message == PPD_GET_PAGE) {
			if ( !message_args.is_string() )
				return; // TODO (ilia) error
			SendPage(message_args.AsString());
		} else if (message == PPD_RELEASE_PAGE) {
			if ( !message_args.is_string() )
				return; // TODO (ilia) error
			ReleasePage(message_args.AsString());
		}
	}

};

class GraphicsModule : public pp::Module {
public:
	GraphicsModule() : pp::Module() { }
	virtual ~GraphicsModule() {}

	virtual pp::Instance* CreateInstance(PP_Instance instance) {
		return new GraphicsInstance(instance);
	}
};

namespace pp {

	Module* CreateModule() {
		return new GraphicsModule();
	}

}  // namespace pp
