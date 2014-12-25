#include "decoder.h"

#include "../helpers/message_helper.h"

DjVuDecoder::DjVuDecoder()
	: error_("djvu:Ok") {
}

DjVuDecoder::~DjVuDecoder() {
	for (auto it = pages_.begin(); it != pages_.begin(); ++it) {
		if (it->second.second.joinable())
			it->second.second.join();
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
	if (pages_.find(pageId) != pages_.end()) {
		//PostMessageToInstance(instance_, CreateDictionaryReply(pageId, "notify", "decoded", 0));
		return;
	}
	if ( ! pages_[pageId].second.joinable() )
		pages_[pageId].second = std::thread(&DjVuDecoder::decodePageThreadFunction_, this, pageId, pageNum, width, height);
}

std::shared_ptr<ddjvu::IBmp<Bitmap>> DjVuDecoder::getPageBmp(std::string pageId) {
	//auto first = pages_[pageId].first;
	
	if (pages_.find(pageId) != pages_.end()) {
		return pages_[pageId].first;
	} else {
		return nullptr;
	} 
}

void DjVuDecoder::decodeThreadFuntion_() {
	if (stream_) {
		if (stream_->getError()) {
			error_ = "djvu:Error";
			return;
		}

		document_ = std::shared_ptr<ddjvu::File<Bitmap>>(new ddjvu::File<Bitmap>(stream_->getPool(), delegateBmpFactory_));

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

	PostObjectMessage(instance_, "browser", "pages", pageArray);
	return;

	//pp::Var var_reply(error_);
	//instance_->PostMessage(var_reply);

	//updateThread_ = std::thread(&PluginWindow::updateThreadFuntion_, this);
}

void DjVuDecoder::decodePageThreadFunction_(std::string pageId, int pageNum, int width, int height) {
	auto first = document_->getPageBitmap(pageNum, width, height, true);
	pages_[pageId].first = first;
	int y = 2;
	PostMessageToInstance(instance_, CreateDictionaryReply(pageId, "notify", "decoded", 0));
	//pages_[pageId].first = document_->getPageBitmap(pageNum, width, height, true);
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
