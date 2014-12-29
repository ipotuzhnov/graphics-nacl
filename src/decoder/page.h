#ifndef DJVU_DECODER_PAGE_H_
#define DJVU_DECODER_PAGE_H_

#include <memory>
#include <thread>

#include "../renderer/bitmap.h"

class DjVuPage {
public:
	std::shared_ptr<renderer::Bitmap> bitmap;
	bool isSending;

	std::thread thread;
	bool isDecoding;

	DjVuPage() : 
		isSending(false),
		isDecoding(true) {}

	~DjVuPage() {
		if (thread.joinable())
			thread.join();
	}
		/*
	BITMAP: renderer::Bitmap
+++isSending: bool

+THREAD: std::thread
+++isDecoding: bool
*/
};

#endif // DJVU_DECODER_PAGE_H_
