#include <algorithm>
#include <cmath>
#include <limits>
#include <string>
#include <vector>
#include <utility>
#include <map>
#include <sstream>
#include <thread>

#include "ppapi/cpp/image_data.h"
#include "ppapi/cpp/input_event.h"
#include "ppapi/cpp/instance.h"
#include "ppapi/cpp/module.h"
#include "ppapi/cpp/point.h"
#include "ppapi/cpp/var.h"
#include "ppapi/cpp/var_array.h"
#include "ppapi/cpp/var_dictionary.h"
#include "ppapi/utility/completion_callback_factory.h"
#include "error_handling/error_handling.h"

#include "../helpers/messages.h"
#include "../helpers/message_helper.h"
#include "../helpers/safe_instance.h"
#include "../loader/url_loader_handler.h"
#include "decoder.h"

#ifdef WIN32
#undef PostMessage
// Allow 'this' in initializer list
#pragma warning(disable : 4355)
#endif
#undef mbstate_t

// Global instance for error_handling =(
std::shared_ptr<SafeInstance> g_safeInstance;

class GraphicsInstance: public pp::Instance {
private:
	pp::Size size_;
	std::shared_ptr<SafeInstance> safeInstance_;
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

		if (url.empty())
			return PostErrorMessage(safeInstance_, "Can't download file: <docsrc> is not set");

		stream_ = std::make_shared<UrlDownloadStream>();
		URLLoaderHandler* handler = URLLoaderHandler::Create(safeInstance_, url, stream_);

		if (handler != NULL) {
			// Starts asynchronous download. When download is finished or when an
			// error occurs, |handler| posts the results back to the browser
			// vis PostMessage and self-destroys.
			handler->Start();
		}
	}

	void DecodeStart() {
		decoder_->startDocumentDecode(safeInstance_, stream_);
	}

	void DecodePage(pp::VarDictionary page) {
		decoder_->startPageDecode(page.Get("pageId").AsString(), page.Get("pageNum").AsInt(), page.Get("size"), page.Get("frame"));
	}

	void SendPageAsBase64(std::string pageId) {
		decoder_->sendPageAsBase64(pageId);
	}

	void ReleasePage(std::string pageId) {
		decoder_->releasePage(pageId);
	}

	void GetPageText(pp::VarDictionary page) {
		decoder_->getPageText(page.Get("pageId").AsString(), page.Get("pageNum").AsInt());
	}

	void ExceptionHandlingThreadFunction_() {
	}

public:
	explicit GraphicsInstance(PP_Instance instance)
		: pp::Instance(instance),
		safeInstance_(std::make_shared<SafeInstance>(this)),
		decoder_(std::make_shared<DjVuDecoder>()),
		size_(0, 0) {
			g_safeInstance = std::make_shared<SafeInstance>(this);
	}

	~GraphicsInstance() {
		safeInstance_->isValid = false;
	}

	virtual bool Init(uint32_t argc, const char* argn[], const char* argv[]) {
		ParseArgs(argc, argn, argv);

		return true;
	}

	// Handle message from JavaScript
	virtual void HandleMessage(const pp::Var& var_message) {
		if (!var_message.is_dictionary())
			return PostErrorMessage(safeInstance_, "Module doesn't understand this type of messages. Only dictionaries!");

		pp::VarDictionary dictionary_message(var_message);

		// TODO (ilia) check if null by the way
		if (!dictionary_message.Get("message").is_string())
			return PostErrorMessage(safeInstance_, "Message is not a string");

		std::string message = dictionary_message.Get("message").AsString();
		pp::Var message_args = dictionary_message.Get("args");
		if (message == PPD_DOWNLOAD_START) {
			DownloadStart();
		} else if (message == PPD_DECODE_START) {
			DecodeStart();
		} else if (message == PPD_DECODE_PAGE) {
			if ( ! message_args.is_dictionary() )
				return PostErrorMessage(safeInstance_, "Args for message " + message + " should be a dictionary");

			DecodePage(pp::VarDictionary(message_args));
		} else if (message == PPD_GET_PAGE_AS_BASE64) {
			if ( !message_args.is_string() )
				return PostErrorMessage(safeInstance_, "Args for message " + message + " should be a string");

			SendPageAsBase64(message_args.AsString());
		} else if (message == PPD_RELEASE_PAGE) {
			if ( !message_args.is_string() )
				return PostErrorMessage(safeInstance_, "Args for message " + message + " should be a string");

			ReleasePage(message_args.AsString());
		} else if (message == PPD_GET_PAGE_TEXT) {
			if ( !message_args.is_dictionary() )
				return PostErrorMessage(safeInstance_, "Args for message " + message + " should be a string");

			GetPageText(pp::VarDictionary(message_args));
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
