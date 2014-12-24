#include <algorithm>
#include <cmath>
#include <limits>
#include <string>
#include <vector>
#include <utility>
#include <map>
#include <sstream>

#include "ppapi/cpp/graphics_2d.h"
#include "ppapi/cpp/image_data.h"
#include "ppapi/cpp/input_event.h"
#include "ppapi/cpp/instance.h"
#include "ppapi/cpp/module.h"
#include "ppapi/cpp/point.h"
#include "ppapi/cpp/var.h"
#include "ppapi/cpp/var_array.h"
#include "ppapi/cpp/var_dictionary.h"
#include "ppapi/utility/completion_callback_factory.h"

#include "helpers/message_helper.h"
#include "loader/url_loader_handler.h"
#include "decoder/decoder.h"

#ifdef WIN32
#undef PostMessage
// Allow 'this' in initializer list
#pragma warning(disable : 4355)
#endif
#undef mbstate_t

namespace {

	enum class plugin_type { unknown, decoder, page };

	// Plugin errors
	const char* const kErrorUnknownType = "browser:error:unknown type";

	// Message format <target>:<message_type>:<command>:<args>
	// Instead of string messages we'll use Arrays or Dictionaries
	// So new message format is Dictionary { target: <target>, type: <message_type>, name: <command, event or object_name>, args: <string, array or dictionary> } 

	// ============================================================================================================
	// plugin_type::decoder messages
	// ============================================================================================================
	// Income
	const char* const kDecoderCommandDownloadStart = "decoder:command:download:start";
	const char* const kDecoderCommandDecodeStart = "decoder:command:decode:start";
	const char* const kDecoderCommandRenderStart = "decoder:command:render:<int: pageNum, int: width, int: height>";
	// Outcome
	const char* const kBrowserNotifyDownlaodProgress = "browser:notify:download:<int: progress>";
	const char* const kBrowserNotifyDownlaodFinished = "browser:notify:download:finished";
	const char* const kPageNotifyPageRendered = "page:notify:rendered:page<int: pageNum, int: width, int: height>";

	// ============================================================================================================
	// plugin_type::page messages
	// ============================================================================================================
	// Income
	const char* const kPageCommandRenderStart = "page:command:render:start";
	// Outcome
	const char* const kDecoderNotifyRender = "decoder:commad:render:<int: pageNum, int: width, int: height>";

	// @TODO (ilia) delete this trash later
	const char* const kReplyString = "hello from NaCl!";
	const char* const kDownloadUrl = "download:Start";
	const char* const kDownloadOk = "download:Ok";
	const char* const kDjVuOk = "djvu:Ok";

	const uint32_t kDefaultColor = 0x2266aa;

	uint32_t MakeColor(uint8_t r, uint8_t g, uint8_t b) {
		uint8_t a = 255;
		PP_ImageDataFormat format = pp::ImageData::GetNativeImageDataFormat();
		if (format == PP_IMAGEDATAFORMAT_BGRA_PREMUL) {
			return (a << 24) | (r << 16) | (g << 8) | b;
		} else {
			return (a << 24) | (b << 16) | (g << 8) | r;
		}
	}

	void FillRect(pp::ImageData* image, int left, int top, int width, int height, uint32_t color) {
		for (int y = 0; y < height; y++) {
			for (int x = 0; x < width; x++) {
				*image->GetAddr32(pp::Point(x, y)) = 0xff0000ff;//MakeColor(255, 0, 100);//0x00ff00ff;
				// 0xff(alpha)00(red)00(green)00(blue) - black color
			}
		}
	}

	// From http://stackoverflow.com/questions/236129/split-a-string-in-c/236803#236803
	// The first puts the results in a pre-constructed vector, the second returns a new vector.
	std::vector<std::string> &split(const std::string &s, char delim, std::vector<std::string> &elems) {
		std::stringstream ss(s);
		std::string item;
		while (std::getline(ss, item, delim)) {
			elems.push_back(item);
		}
		return elems;
	}

	std::vector<std::string> split(const std::string &s, char delim) {
		std::vector<std::string> elems;
		split(s, delim, elems);
		return elems;
	}

} // namespace

class GraphicsInstance: public pp::Instance {
private:
	uint32_t color_;
	pp::Size size_;
	pp::Graphics2D context_;
	std::shared_ptr<UrlDownloadStream> stream_;
	std::shared_ptr<DjVuDecoder> decoder_;
	plugin_type type_;

	// Arguments
	std::map<std::string, std::string> args_;

	void ParseArgs(uint32_t argc, const char* argn[], const char* argv[]) {
		for (uint32_t i = 0; i < argc; ++i) {
			std::string tag = std::string(argn[i]);
			std::string value = std::string(argv[i]);
			args_[tag] = value;
		}
	}

	bool ParseMessage(std::string message) {
		 
	}

	void CheckType() {
		std::string type = "plugin_type";
		if (args_[type] == "decoder") {
			type_ = plugin_type::decoder;
		} else if (args_[type] == "page") {
			type_ = plugin_type::page;
		} else {
			type_ = plugin_type::unknown;
		}
		/*
		for (auto it = args_.begin(); it != args_.end(); ++it) {
			if (it->first == "plugin_type") {
				if (it->second == "decoder")
					type_ = plugin_type::decoder;
				else if (it->second == "page")
					type_ = plugin_type::page;
			}
		}
		*/
	}

	static void FlushCallback(void* instance, int32_t result) {
		// Here we can get instance
		// GraphicsInstance* instance1 = static_cast<GraphicsInstance*>(instance); 
	}

	// Handle different types of messages

	void HandleDecoderMessage(pp::Var target, pp::Var message_type, pp::Var name, pp::Var args) {
		if (message_type == "command") {
			if (name == "download") {
				if (args == "start") {
					DecoderDownloadStart();
				}
			} else if (name == "decode") {
				if (args == "start") {
					DecoderDecodeStart();
				}
			}
		}
	}

	void DecoderDownloadStart() {
		std::string url;
		for (auto it = args_.begin(); it != args_.end(); ++it) {
			if (it->first == "docsrc") {
				url = it->second;
			}
		}
		// @TODO (ilia) create appropriative error later
		if (url.empty()) { //@TODO (ilia) return erorr here
			pp::Var var_reply("Can't download file: <docsrc> is empty");
			PostMessage(var_reply);
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

	void DecoderDecodeStart() {
		if (!decoder_)
			decoder_ = std::make_shared<DjVuDecoder>(this, stream_);
		decoder_->start();
	}

public:
	explicit GraphicsInstance(PP_Instance instance)
		: pp::Instance(instance),
		type_(plugin_type::unknown),
		color_(kDefaultColor), 
		size_(0, 0) {}

	virtual bool Init(uint32_t argc, const char* argn[], const char* argv[]) {
		ParseArgs(argc, argn, argv);
		CheckType();
		return true;
	}

	// Handle message from JavaScript
	virtual void HandleMessage(const pp::Var& var_message) {
		// Don't communicate with browser
		if (type_ == plugin_type::unknown) {
			pp::Var var_reply(kErrorUnknownType);
			PostMessage(var_reply);
			return;
		}
		
		if (!var_message.is_dictionary()) {
			// @TODO (ilia) create appropriative error later
			PostErrorMessage(this, "Module doesn't understand this type of messages. Only dictionaries!");
			/*
			pp::Var var_reply("Module doesn't understand this type of messages. Only dictionaries!");
			PostMessage(var_reply);
			*/
			return;
		}

		pp::VarDictionary dictionary_message(var_message);

		// Handle decoder messages
		if (type_ == plugin_type::decoder) {
			// @TODO (ilia) create appropriative error later
			if (dictionary_message.Get("target").is_null()) {
				pp::Var var_reply("Message target is undefiend");
				PostMessage(var_reply);
				return;
			}
			if (dictionary_message.Get("type").is_null()) {
				pp::Var var_reply("Message type is undefiend");
				PostMessage(var_reply);
				return;
			}
			if (dictionary_message.Get("name").is_null()) {
				pp::Var var_reply("Message command name is undefiend");
				PostMessage(var_reply);
				return;
			}

			HandleDecoderMessage(dictionary_message.Get("target"), dictionary_message.Get("type"), dictionary_message.Get("name"), dictionary_message.Get("args"));
			return;
			//std::string target = message.substr(0, message.substr(0
		}

		struct ArrayMessage {
			std::string target;
			std::string message_type;
			std::string command;
			std::vector<std::string> args;
		};
		struct VarMessage {
			pp::Var target;
			pp::Var message_type;
			pp::Var command;
			std::vector<pp::Var> args;
		};

		VarMessage var_message_array;
		if (var_message.is_array()) {
			pp::VarArray asArray(var_message);
			if (asArray.GetLength() < 4) {
				pp::Var var_reply("Array shoul have at least 3 elements");
				PostMessage(var_reply);
				return;
			}
			var_message_array.target = asArray.Get(0);
			var_message_array.message_type = asArray.Get(1);
			var_message_array.command = asArray.Get(2);
			for (int i = 3; i < asArray.GetLength(); i++) {
				var_message_array.args.push_back(asArray.Get(i));
			}
			int y = 2;
		}

		// Check if message is string
		if (!var_message.is_string())
			return;

		// Handle decoder messages
		if (type_ == plugin_type::decoder) {
			std::string message = var_message.AsString();
			// @TODO (ilia) decide how and when parse the message
			//ParseMessage(message);
			auto parsed = split(message, ':');
			// @TODO (ilia) create appropriative error later
			if (parsed.size() < 4) {
				pp::Var var_reply("undefiend command, not all arguments provided");
				PostMessage(var_reply);
				return;
			}
			auto target = parsed.at(0);
			auto message_type = parsed.at(1);
			auto command = parsed.at(2);
			auto args = parsed.at(3);

			HandleDecoderMessage(target, message_type, command, args);
			return;
			//std::string target = message.substr(0, message.substr(0
		}

		return;






		std::string message = var_message.AsString();
		if (message == kDownloadUrl) {
			std::string url;
			for (auto it = args_.begin(); it != args_.end(); ++it) {
				if (it->first == "docsrc") {
					url = it->second;
				}
			}

			if (url.empty()) { //@TODO (ilia) return erorr here
				url = "http://localhost/docs/1.djvu";
			}

			//pp::Var var_reply_url(url);
			//PostMessage(var_reply_url);

			stream_ = std::make_shared<UrlDownloadStream>();
			URLLoaderHandler* handler = URLLoaderHandler::Create(this, url, stream_);

			if (handler != NULL) {
				// Starts asynchronous download. When download is finished or when an
				// error occurs, |handler| posts the results back to the browser
				// vis PostMessage and self-destroys.
				handler->Start();
			}
		} else if (message == kDownloadOk) {
			// Start decoding
			if (!decoder_)
				decoder_ = std::make_shared<DjVuDecoder>(this, stream_);
			decoder_->start();
		} else if (message == kDjVuOk) {
			// Redraw
			if (decoder_) {
				pp::ImageData image = *decoder_->getBmp()->getBmp();
				context_.ReplaceContents(&image);
				pp::CompletionCallback cc(FlushCallback, this);
				context_.Flush(cc);
			}
		} else {
			pp::Var var_reply(kReplyString);
			PostMessage(var_reply);
		}
	}

	// On view changed event
	virtual void DidChangeView(const pp::View& view) {
		pp::Size new_size = view.GetRect().size();

		if (new_size.width() == size_.width() &&
			new_size.height() == size_.height()) {
				// No change. We don't care about the position, only the size.
				return;
		}

		context_ = pp::Graphics2D(this, new_size, false);
		if (!BindGraphics(context_))
			return;
		size_ = new_size;

		PP_ImageDataFormat format = pp::ImageData::GetNativeImageDataFormat();
		const bool kDontInitToZero = false;
		pp::ImageData image(this, format, size_, kDontInitToZero);
		if(image.is_null())
			return;
		FillRect(&image, 0, 0, size_.width(), size_.height(), color_);

		context_.ReplaceContents(&image);
		pp::CompletionCallback cc(FlushCallback, this);

		context_.Flush(cc);
	}

};

class GraphicsModule : public pp::Module {
public:
	GraphicsModule() : pp::Module() {}
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
