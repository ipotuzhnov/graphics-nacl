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
	uint32_t color_;
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

	static void DumpJson(const char* json) {
		size_t size = strlen(json) + 1;  // +1 for NULL.
		std::string out(json, size);
		PostExceptionMessage(g_safeInstance, out);
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
			PostErrorMessage(safeInstance_, "Can't download file: <docsrc> is empty");
			return;
		}

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
		//decoder_->startPageDecode(page.Get("pageId").AsString(), page.Get("pageNum").AsInt(), pp::VarDictionary(page.Get("size")), pp::VarDictionary(page.Get("frame")));
	}

	void SendPage(std::string pageId) {
		decoder_->sendPage(pageId);
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
		PostLogMessage(safeInstance_, "DidCreate.");

		/* Request exception callbacks with JSON. */
		//EHRequestExceptionsJson(DumpJson);

		/* Report back if the request was honored. */
		/*
		if (!EHHanderInstalled()) {
			PostLogMessage(safeInstance_, "Stack traces not available, so don't expect them.");
		} else {
			PostLogMessage(safeInstance_, "Stack traces are on.");
		}
		*/
		return true;
	}

	// Handle message from JavaScript
	virtual void HandleMessage(const pp::Var& var_message) {		
		if (!var_message.is_dictionary()) {
			// @TODO (ilia) create appropriative error later
			PostErrorMessage(safeInstance_, "Module doesn't understand this type of messages. Only dictionaries!");
			return;
		}

		pp::VarDictionary dictionary_message(var_message);
		// @TODO (ilia) create appropriative error later
		//if ( ! IsMessageValid(dictionary_message) )
		//	return;

		// TODO (ilia) check if null by the way
		if (!dictionary_message.Get("message").is_string()) {
			PostErrorMessage(safeInstance_, "Message is not a string");
			return;
		}

		std::string message = dictionary_message.Get("message").AsString();
		pp::Var message_args = dictionary_message.Get("args");
		if (message == PPD_DOWNLOAD_START) {
			DownloadStart();
		} else if (message == PPD_DECODE_START) {
			DecodeStart();
		} else if (message == PPD_DECODE_PAGE) {
			PostLogMessage(safeInstance_, "LOG: before module::DecodePage()");
			if ( ! message_args.is_dictionary() )
				return; // TODO (ilia) error
			DecodePage(pp::VarDictionary(message_args));
			PostLogMessage(safeInstance_, "LOG: after module::DecodePage()");
		} else if (message == PPD_GET_PAGE) {
			if ( !message_args.is_string() )
				return; // TODO (ilia) error
			SendPage(message_args.AsString());
		} else if (message == PPD_GET_PAGE_AS_BASE64) {
			PostLogMessage(safeInstance_, "LOG: before module::SendPageAsBase64()");
			if ( !message_args.is_string() )
				return; // TODO (ilia) error
			SendPageAsBase64(message_args.AsString());
			PostLogMessage(safeInstance_, "LOG: after module::SendPageAsBase64()");
		} else if (message == PPD_RELEASE_PAGE) {
			if ( !message_args.is_string() )
				return; // TODO (ilia) error
			ReleasePage(message_args.AsString());
		} else if (message == PPD_GET_PAGE_TEXT) {
			if ( !message_args.is_dictionary() )
				return; // TODO (ilia) error
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
