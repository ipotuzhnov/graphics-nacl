#ifndef DJVU_DECODER_PAGE_H_
#define DJVU_DECODER_PAGE_H_

#include <memory>
#include <thread>

#include "../renderer/bitmap.h"

class DjVuPage {
public:
	std::shared_ptr<renderer::Bitmap> bitmap;
	bool isSending;

	std::thread decoding_thread;
	std::thread sending_thread;
	bool isDecoding;

	std::shared_ptr<DjVuSize> size;
	std::shared_ptr<DjVuFrame> frame;

	// TODO (ilia) remove it later
	int pageNum;

	DjVuPage() : 
		isSending(false),
		isDecoding(false)
	{}

	~DjVuPage() {
		if (decoding_thread.joinable())
			decoding_thread.join();
		if (sending_thread.joinable())
			sending_thread.join();
	}
		/*
	BITMAP: renderer::Bitmap
+++isSending: bool

+THREAD: std::thread
+++isDecoding: bool
*/
};

#endif // DJVU_DECODER_PAGE_H_
