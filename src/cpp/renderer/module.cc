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

#include "../helpers/messages.h"
#include "../helpers/message_helper.h"
#include "bitmap.h"

#ifdef WIN32
#undef PostMessage
// Allow 'this' in initializer list
#pragma warning(disable : 4355)
#endif
#undef mbstate_t

namespace {

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
	bool isFlushed_;
	bool isDrawn_;

	bool isReceiving_;
	std::shared_ptr<renderer::Bitmap> bitmap_;

	// Arguments
	std::map<std::string, std::string> args_;

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
		instance_->Flushed();
		instance_->Drawn();
		if (result == PP_ERROR_BADRESOURCE)
			instance_->Update();
	}

	void PageRender() {
		pp::ImageData image;
		isFlushed_ = false;
		if (bitmap_) {
			//Bitmap bmp(imageAsDictionary);
			//pp::ImageData image = bmp.getAsImageData(this);
			image = bitmap_->getAsImageData(this);
		} else {
			PP_ImageDataFormat format = pp::ImageData::GetNativeImageDataFormat();
			const bool kDontInitToZero = false;
			image = pp::ImageData(this, format, size_, kDontInitToZero);
			if(image.is_null())
				return;
			uint32_t color = 0;
			FillRect(&image, 0, 0, size_.width(), size_.height(), color);
		}
		context_.ReplaceContents(&image);
		pp::CompletionCallback cc(FlushCallback, this);
		context_.Flush(cc);
	}

public:
	explicit GraphicsInstance(PP_Instance instance)
		: pp::Instance(instance),
		isReceiving_(false),
		isFlushed_(true),
		isDrawn_(false),
		size_(0, 0) {}

	virtual bool Init(uint32_t argc, const char* argn[], const char* argv[]) {
		ParseArgs(argc, argn, argv);
		return true;
	}

	void Update() {
		if (isFlushed_)
			PageRender();
	}

	void Flushed() {
		isFlushed_ = true;
	}

	void Drawn() {
		if (isFlushed_ && bitmap_)
			isDrawn_ = true;
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

		// TODO (ilia) check if null by the way
		if (!dictionary_message.Get("message").is_string()) {
			PostErrorMessage(this, "Message is not a string");
			return;
		}

		std::string message = dictionary_message.Get("message").AsString();
		if (message == PPB_PAGE_READY) {
			if ( ! isReceiving_ ) {
				isReceiving_ = true;
				PostMessage(CreateDictionaryReply(PPD_GET_PAGE, dictionary_message.Get("args")));
			}
			// @TODO (ilia) create appropriative error later
		} else if (message == PPR_SEND_PAGE) {
			if ( ! dictionary_message.Get("args").is_dictionary() )
				return;
			//Sleep(5000);
			pp::VarDictionary dictionary_bitmap(dictionary_message.Get("args"));
			bitmap_ = std::shared_ptr<renderer::Bitmap>(new renderer::Bitmap(dictionary_bitmap));

			PostMessage(CreateDictionaryReply(PPB_PAGE_RECEIVED, args_["id"]));
			/*
			if ( isFlushed_ ) {
				PageRender();
			}
			*/
		} else if (message == PPR_PAGE_RENDER) {
			if (bitmap_)
				PageRender();
			else
				return; // TODO (ilia) error
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
				//if ( isFlushed_ && ! isDrawn_ )
				//	PageRender();
				return;
		}
		

		context_ = pp::Graphics2D(this, new_size, false);
		if (!BindGraphics(context_))
			return;
		size_ = new_size;
		//PageRender();
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
