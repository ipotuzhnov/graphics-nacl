#ifndef DJVU_DECODER_H_
#define DJVU_DECODER_H_

#include "ppapi/cpp/var.h"
#include "ppapi/cpp/var_array.h"
#include "ppapi/cpp/var_dictionary.h"
#include "ppapi/cpp/var_array_buffer.h"

#include "ddjvu/File.h"

#include "../helpers/safe_instance.h"
#include "../loader/url_download_stream.h"
#include "bitmap.h"
#include "page.h"

#include "bmp_factory_delegate.h"

class DjVuDecoder {
public:
	DjVuDecoder();
	~DjVuDecoder();

	void startDocumentDecode(std::shared_ptr<SafeInstance> safeInstance, std::shared_ptr<UrlDownloadStream> stream);
	void startPageDecode(std::string pageId, int pageNum,  pp::Var size, pp::Var frame);
	void sendPageAsBase64(std::string pageId);
	void releasePage(std::string pageId);
	void getPageText(std::string pageId, int pageNum);
private:
	void decodeThreadFuntion_();
	void decodePageThreadFunction_(std::string pageId, int pageNum,  pp::Var size, pp::Var frame);
	void sendPageThreadFunction_(std::string pageId);

	std::shared_ptr<SafeInstance> safeInstance_; // Shared pointer

	std::shared_ptr<UrlDownloadStream> stream_;
	std::shared_ptr<ddjvu::File<decoder::Bitmap>> document_;
	std::shared_ptr<BmpFactoryDelegate> delegateBmpFactory_;
	int numberOfPages_;

	std::map<std::string, std::shared_ptr<DjVuPage>> pages_;
	std::string error_;

	std::thread decodeThread_;
};

#endif // DJVU_DECODER_H_
