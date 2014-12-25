#ifndef DJVU_DECODER_H_
#define DJVU_DECODER_H_

#include "ppapi/cpp/var.h"
#include "ppapi/cpp/var_array.h"
#include "ppapi/cpp/var_dictionary.h"

#include "ddjvu/File.h"

#include "../loader/url_download_stream.h"

#include "bmp_factory_delegate.h"

class DjVuDecoder {
public:
	DjVuDecoder();
	~DjVuDecoder();

	void startDocumentDecode(pp::Instance* instance, std::shared_ptr<UrlDownloadStream> stream);
	void startPageDecode(std::string pageId, int pageNum, int width, int height);
	std::shared_ptr<ddjvu::IBmp<Bitmap>> getPageBmp(std::string pageId);
private:
	void decodeThreadFuntion_();
	void decodePageThreadFunction_(std::string pageId, int pageNum, int width, int height);
	//void updateThreadFuntion_();

	std::shared_ptr<UrlDownloadStream> stream_;
	std::shared_ptr<ddjvu::File<Bitmap>> document_;
	std::shared_ptr<BmpFactoryDelegate> delegateBmpFactory_;

	pp::Instance* instance_;  // Weak pointer.
	typedef std::shared_ptr<ddjvu::IBmp<Bitmap>> bitmap_mt;
	typedef std::pair<bitmap_mt, std::thread> page_mt;
	//typedef std::map<std::string, Bitmap> PagesMap;
	//std::shared_ptr<PagesMap> pages_;
	std::map<std::string, page_mt> pages_;
	//std::shared_ptr<ddjvu::IBmp<pp::ImageData>> bmp_;
	std::string error_;

	std::thread decodeThread_;
	//std::thread updateThread_;
};

#endif // DJVU_DECODER_H_
