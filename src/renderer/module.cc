#include <algorithm>
#include <cmath>
#include <limits>
#include <string>
#include <vector>
#include <utility>
#include <map>
#include <sstream>

#include <memory>

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
#include "bitmap.h"

#ifdef WIN32
#undef PostMessage
// Allow 'this' in initializer list
#pragma warning(disable : 4355)
#endif
#undef mbstate_t

namespace {

	// @TODO (ilia)
	// Message format <target>:<message_type>:<command>:<args>
	// Instead of string messages we'll use Arrays or Dictionaries
	// So new message format is Dictionary { target: <target>, type: <message_type>, name: <command, event or object_name>, args: <string, array or dictionary> } 

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

} // namespace

class GraphicsInstance: public pp::Instance {
private:
	pp::Size size_;
	pp::Graphics2D context_;

	bool isReceiving_;
	std::shared_ptr<renderer::Bitmap> bitmap_;

	// Arguments
	std::map<std::string, std::string> args_;

	// Messages
	// ppb - Pepper Browser
	// ppp - Pepper Plugin
	// gnd - Graphics NaCl Decoder
	// gnr - Graphics NaCl Renderer

	// Page is decoded
	// args = pageId
	const char* const PPR_PAGE_READY = "PPR_PAGE_READY";
	// Receiving page from JS
	// args = pp::Dictionary
	const char* const PPR_SEND_PAGE = "PPR_SEND_PAGE";

	// Request page pp::Dictionary
	const char* const PPD_GET_PAGE = "PPD_GET_PAGE";



	void ParseArgs(uint32_t argc, const char* argn[], const char* argv[]) {
		for (uint32_t i = 0; i < argc; ++i) {
			std::string tag = std::string(argn[i]);
			std::string value = std::string(argv[i]);
			args_[tag] = value;
		}
	}

	static void FlushCallback(void* instance, int32_t result) {
		// Here we can get instance
		// GraphicsInstance* instance_ = static_cast<GraphicsInstance*>(instance); 
		GraphicsInstance* instance_ = static_cast<GraphicsInstance*>(instance); 
		if (result == PP_ERROR_BADRESOURCE)
			instance_->Update();
	}

	// Handle different types of messages

	// Page messages
	/*
	void HandlePageMessage(pp::Var message, pp::Var args) {
		if (message_type == "notify") {
			if (name == "decode") {
				if (args == "finished") {
					PageRender();
				} else if (args == "update") {
					//PageRender();
				}
			} else if (name == "decoded") {
				if (args.is_dictionary()) {
					imageAsDictionary = args;
					PageRender();
				} else {
					PostErrorMessage(this, "Args is not an array buffer"); 
				}
			}
		}
	}
	*/
	void PageRender() {
		if (bitmap_) {
			//Bitmap bmp(imageAsDictionary);
			//pp::ImageData image = bmp.getAsImageData(this);

			pp::ImageData image = bitmap_->getAsImageData(this);
			context_.ReplaceContents(&image);
			pp::CompletionCallback cc(FlushCallback, this);
			context_.Flush(cc);
		}
	}

public:
	explicit GraphicsInstance(PP_Instance instance)
		: pp::Instance(instance),
		isReceiving_(false),
		size_(0, 0) {}

	virtual bool Init(uint32_t argc, const char* argn[], const char* argv[]) {
		ParseArgs(argc, argn, argv);
		return true;
	}

	void Update() {
		PageRender();
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
		if (message == PPR_PAGE_READY) {
			if ( ! isReceiving_ ) {
				isReceiving_ = true;
				PostMessage(CreateDictionaryReply(PPD_GET_PAGE, dictionary_message.Get("args")));
			}
			// @TODO (ilia) create appropriative error later
		} else if (message == PPR_SEND_PAGE) {
			if ( ! dictionary_message.Get("args").is_dictionary() )
				return;
			pp::VarDictionary dictionary_bitmap(dictionary_message.Get("args"));
			bitmap_ = std::make_shared<renderer::Bitmap>(renderer::Bitmap(dictionary_bitmap));
			PageRender();
		}

	}

	// On view changed event
	virtual void DidChangeView(const pp::View& view) {
		//auto decoder = decoder_;
		//auto id = args_["id"];
		//auto getpagebmp = decoder_->getPageBmp(id);
		//auto getbmp = getpagebmp->getBmp();

		/*
		if (decoder_) {
			if (decoder_->getPageBmp(args_["id"]) != nullptr) {
				//PageRender();
				return;
			}
		}
		*/

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
		uint32_t color = 0;
		FillRect(&image, 0, 0, size_.width(), size_.height(), color);

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
