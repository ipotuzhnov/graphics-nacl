#ifndef DJVU_DECODER_H_
#define DJVU_DECODER_H_

#include "../ddjvu/File.h"
#include "../loader//url_download_stream.h"
#include "bmp_factory_delegate.h"

class DjVuDecoder {
public:
	DjVuDecoder(pp::Instance* instance, std::shared_ptr<UrlDownloadStream> stream);
	~DjVuDecoder();

	void start(pp::Size size);
	std::shared_ptr<ddjvu::IBmp<pp::ImageData>> getBmp();
private:
	void decodeThreadFuntion_();
	void updateThreadFuntion_();

	std::shared_ptr<UrlDownloadStream> stream_;
	std::shared_ptr<ddjvu::File<pp::ImageData>> document_;
	std::shared_ptr<BmpFactoryDelegate> delegateBmpFactory_;

	pp::Instance* instance_;  // Weak pointer.
	std::shared_ptr<ddjvu::IBmp<pp::ImageData>> bmp_;
	std::string error_;
	pp::Size size_;

	std::thread decodeThread_;
	std::thread updateThread_;
};

#endif // DJVU_DECODER_H_
