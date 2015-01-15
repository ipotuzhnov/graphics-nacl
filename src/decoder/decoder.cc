#include "decoder.h"

#include "../helpers/messages.h"
#include "../helpers/message_helper.h"

DjVuDecoder::DjVuDecoder()
	: error_("djvu:Ok") {
}

DjVuDecoder::~DjVuDecoder() {
	for (auto it = pages_.begin(); it != pages_.begin(); ++it) {
		if (it->second->decoding_thread.joinable())
			it->second->decoding_thread.join();
	}
	/*
	if (updateThread_.joinable())
		updateThread_.join();
	*/
	if (decodeThread_.joinable())
		decodeThread_.join();
}

void DjVuDecoder::startDocumentDecode(pp::Instance* instance, std::shared_ptr<UrlDownloadStream> stream) {
	stream_ = stream;
	instance_ = instance;
	delegateBmpFactory_ = std::make_shared<BmpFactoryDelegate>(instance);

	decodeThread_ = std::thread(&DjVuDecoder::decodeThreadFuntion_, this);
}

void DjVuDecoder::startPageDecode(std::string pageId, int pageNum, int width, int height) {
	//if ( pages_[pageId].first ) {
	if (pages_.find(pageId) != pages_.end())
		return; // TODO (ilia) Error: page already exists

	pages_[pageId] = std::make_shared<DjVuPage>();
	pages_[pageId]->decoding_thread = std::thread(&DjVuDecoder::decodePageThreadFunction_, this, pageId, pageNum, width, height);
}

void DjVuDecoder::sendPage(std::string pageId) {
	if (pages_.find(pageId) == pages_.end())
		return; // TODO (ilia) Error: page does not exist

	pages_[pageId]->isSending = true;
	PostBitmapMessage(instance_, pageId, pages_[pageId]->bitmap->getAsDictionary());
}

void DjVuDecoder::sendPageAsBase64(std::string pageId) {
	if (pages_.find(pageId) == pages_.end())
		return; // TODO (ilia) Error: page does not exist
	// Send image in separate thread
	pages_[pageId]->sending_thread = std::thread(&DjVuDecoder::sendPageThreadFunction_, this, pageId);

	//PostBitmapMessageAsBase64(instance_, pageId, pages_[pageId]->bitmap->getAsBase64Dictionary());
}

void DjVuDecoder::releasePage(std::string pageId) {
	if (pages_.find(pageId) == pages_.end())
		return; // TODO (ilia) Error: page does not exist

	pages_.erase(pageId);
}

std::shared_ptr<renderer::Bitmap> DjVuDecoder::getPageBmp(std::string pageId) {
	//auto first = pages_[pageId].first;
	
	if (pages_.find(pageId) != pages_.end()) {
		return pages_[pageId]->bitmap;
	} else {
		return nullptr;
	} 
}

void DjVuDecoder::decodeThreadFuntion_() {
	// TODO (ilia) report errors
	if (stream_) {
		if (stream_->getError()) {
			error_ = "djvu:Error";
			return;
		}

		document_ = std::shared_ptr<ddjvu::File<renderer::Bitmap>>(new ddjvu::File<renderer::Bitmap>(stream_->getPool(), delegateBmpFactory_));

		if (!document_->isDocumentValid()) {
			error_ = "djvu:Error";
			return;
		}
	} else {
		error_ = "djvu:Error";
		return;
	}

	int nPages = document_->getPageNum();

	pp::VarArray pageArray;
	pageArray.SetLength(nPages);

	for (int pageNum = 0; pageNum < nPages; pageNum++){
		ddjvu_pageinfo_t pageInfo =  document_->getPageInfo(pageNum);

		// Send page as dictionary
		pp::VarDictionary page;

		page.Set("width", pageInfo.width);
		page.Set("height", pageInfo.height);
		page.Set("dpi", pageInfo.dpi);

		pageArray.Set(pageNum, page);
	}

	PostMessageToInstance(instance_, CreateDictionaryReply(PPB_DECODE_FINISHED, pageArray));
	return;

	//pp::Var var_reply(error_);
	//instance_->PostMessage(var_reply);

	//updateThread_ = std::thread(&PluginWindow::updateThreadFuntion_, this);
}

void DjVuDecoder::decodePageThreadFunction_(std::string pageId, int pageNum, int width, int height) {
	pages_[pageId]->bitmap = document_->getPageBitmap(pageNum, width, height, true)->getBmp();
	//pp::VarDictionary bitmapAsDict = iBitmap->getBmp()->getAsDictionary();
	pages_[pageId]->isDecoding = false;
	PostMessageToInstance(instance_, CreateDictionaryReply(PPR_PAGE_READY, pageId));
	document_->abortPageDecode(pageNum, width, height, 0);
}

void DjVuDecoder::sendPageThreadFunction_(std::string pageId) {
	PostBitmapMessageAsBase64(instance_, pageId, pages_[pageId]->bitmap->getAsBase64Dictionary());
}

/*
void DjVuDecoder::updateThreadFuntion_() {
	auto windowNotifier = document_->getWindowNotifier();

	while (!windowNotifier->check(ddjvu::message_window::CLOSE)) {
	windowNotifier->wait();
	if (windowNotifier->check(ddjvu::message_window::UPDATE) && !windowNotifier->check(ddjvu::message_window::CLOSE)) {
	auto views = document_->getRenderedViews();
	auto it = unique (views.begin(), views.end());
	views.resize( distance(views.begin(),it) );
	for (auto it = views.begin(); it != views.end(); ++it) {
	if ((*it ) == PW_MAIN_VIEW)
	updateMainView_();
	if ((*it) == PW_PREVIEW_VIEW)
	updatePreviewView_();
	}
	windowNotifier->reset(ddjvu::message_window::UPDATE);
	}
	}
}
*/
