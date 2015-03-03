#ifndef DJVU_DECODER_PAGE_H_
#define DJVU_DECODER_PAGE_H_

#include <memory>
#include <thread>

#include "bitmap.h"

struct DjVuPage {
	std::shared_ptr<decoder::Bitmap> bitmap;

	std::thread decoding_thread;
	std::thread sending_thread;

	std::shared_ptr<DjVuSize> size;
	std::shared_ptr<DjVuFrame> frame;
};

#endif // DJVU_DECODER_PAGE_H_
