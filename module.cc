#include <algorithm>
#include <cmath>
#include <limits>
#include <string>
#include <vector>
#include <utility>

#include "ppapi/cpp/graphics_2d.h"
#include "ppapi/cpp/image_data.h"
#include "ppapi/cpp/input_event.h"
#include "ppapi/cpp/instance.h"
#include "ppapi/cpp/module.h"
#include "ppapi/cpp/point.h"
#include "ppapi/cpp/var.h"
#include "ppapi/utility/completion_callback_factory.h"

#include "loader/url_loader_handler.h"
#include "decoder/decoder.h"

#ifdef WIN32
#undef PostMessage
// Allow 'this' in initializer list
#pragma warning(disable : 4355)
#endif
#undef mbstate_t

namespace {

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

} // namespace

class GraphicsInstance: public pp::Instance {
private:
	uint32_t color_;
	pp::Size size_;
	pp::Graphics2D context_;
	std::shared_ptr<UrlDownloadStream> stream_;
	std::shared_ptr<DjVuDecoder> decoder_;

	// Arguments
	std::vector<std::pair<std::string, std::string>> args_;

	void ParseArgs(uint32_t argc, const char* argn[], const char* argv[]) {
		for (uint32_t i = 0; i < argc; ++i) {
			std::string tag = std::string(argn[i]);
			std::string value = std::string(argv[i]);
			std::pair<std::string, std::string> arg(tag, value); 
			args_.push_back(arg);
		}
	}

	static void FlushCallback(void* instance, int32_t result) {
		// Here we can get instance
		// GraphicsInstance* instance1 = static_cast<GraphicsInstance*>(instance); 
	}

public:
	explicit GraphicsInstance(PP_Instance instance)
		: pp::Instance(instance), 
		color_(kDefaultColor), 
		size_(0, 0) {}

	virtual bool Init(uint32_t argc, const char* argn[], const char* argv[]) {
		ParseArgs(argc, argn, argv);
		return true;
	}

	// Handle message from JavaScript
	virtual void HandleMessage(const pp::Var& var_message) {
		if (!var_message.is_string())
			return;

		std::string message = var_message.AsString();
		if (message == kDownloadUrl) {
			std::string url;
			for (auto it = args_.begin(); it != args_.end(); ++it) {
				if (it->first == "docsrc") {
					url = it->second;
				}
			}

			if (url.empty()) {
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
			decoder_ = std::make_shared<DjVuDecoder>(this, stream_);
			decoder_->start(size_);
		} else if (message == kDjVuOk) {
			// Redraw
			
			pp::ImageData image = *decoder_->getBmp()->getBmp();
			context_.ReplaceContents(&image);
			pp::CompletionCallback cc(FlushCallback, this);
			context_.Flush(cc);
			
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
