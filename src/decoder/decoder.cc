#include "decoder.h"

#include <locale>
#include <codecvt>

#include "../helpers/messages.h"
#include "../helpers/message_helper.h"

DjVuDecoder::DjVuDecoder()
	: error_("djvu:Ok"), numberOfPages_(0) {
}

DjVuDecoder::~DjVuDecoder() {
	document_->getWindowNotifier()->set(ddjvu::message_window::CLOSE);
	for (auto it = pages_.begin(); it != pages_.end(); ++it) {
		//if (it->second->isDecoding) {
		if (it->second->decoding_thread.joinable()) {
			//document_->abortPageDecode(it->second->pageNum, it->second->size->width, it->second->size->height, 0);
			document_->abortPageDecode(it->second->pageId);
			it->second->decoding_thread.join();
		}
		//}
		//if (it->second->isSending) {
		if (it->second->sending_thread.joinable())
			it->second->sending_thread.join();
		//}
	}
	/*
	if (updateThread_.joinable())
		updateThread_.join();
	*/
	if (decodeThread_.joinable())
		decodeThread_.join();
}

void DjVuDecoder::startDocumentDecode(std::shared_ptr<SafeInstance> safeInstance, std::shared_ptr<UrlDownloadStream> stream) {
	stream_ = stream;
	safeInstance_ = safeInstance;
	delegateBmpFactory_ = std::make_shared<BmpFactoryDelegate>(safeInstance_);

	decodeThread_ = std::thread(&DjVuDecoder::decodeThreadFuntion_, this);
}

void DjVuDecoder::startPageDecode(std::string pageId, int pageNum, pp::Var size, pp::Var frame) {
	//if ( pages_[pageId].first ) {
	if (pages_.find(pageId) != pages_.end())
		return; // TODO (ilia) Error: page already exists

	if (pageNum >= numberOfPages_)
		return; // TODO (ilia) Error: page number is too big
	pages_[pageId] = std::make_shared<DjVuPage>();
	auto page = pages_[pageId];
	if (!page)
		return; // TODO (ilia) page does not exist	
	page->pageId = pageId;
	page->pageNum = pageNum;
	page->decoding_thread = std::thread(&DjVuDecoder::decodePageThreadFunction_, this, pageId, pageNum, size, frame);
}

void DjVuDecoder::sendPage(std::string pageId) {
	if (pages_.find(pageId) == pages_.end())
		return; // TODO (ilia) Error: page does not exist
	auto page = pages_[pageId];
	if (!page)
		return; // TODO (ilia) page does not exist
	PostBitmapMessage(safeInstance_, pageId, page->bitmap->getAsDictionary());
}

void DjVuDecoder::sendPageAsBase64(std::string pageId) {
	if (pages_.find(pageId) == pages_.end())
		return; // TODO (ilia) Error: page does not exist
	// Send image in separate thread
	auto page = pages_[pageId];
	if (!page)
		return; // TODO (ilia) page does not exist
	page->sending_thread = std::thread(&DjVuDecoder::sendPageThreadFunction_, this, pageId);

	//PostBitmapMessageAsBase64(instance_, pageId, pages_[pageId]->bitmap->getAsBase64Dictionary());
}

void DjVuDecoder::releasePage(std::string pageId) {
	if (pages_.find(pageId) == pages_.end())
		return; // TODO (ilia) Error: page does not exist

	pages_.erase(pageId);
}

void DjVuDecoder::getPageText(std::string pageId, int pageNum) {
	if (pageNum >= numberOfPages_)
		return; // TODO (ilia) Error: page number is too big
	auto document = document_;
	if (!document)
		return; // TODO (ilia) document does not exist
	std::vector<ddjvu::Text> pageText = document->getPageText(pageNum);
	pp::VarArray var_page_text;
	var_page_text.SetLength(pageText.size());
	for (int i = 0; i < pageText.size(); i++) {		
		// Word
		std::wstring w_word = pageText[i].getWord();
		std::wstring_convert<std::codecvt_utf8<wchar_t>> converter;
		std::string word = converter.to_bytes(w_word);
		pp::Var var_word(word);
		// Rect
		pp::VarDictionary var_rect;
		ddjvu::Rectangle rect = pageText[i].getRect();
		var_rect.Set("left", rect.left);
		var_rect.Set("top", rect.top);
		var_rect.Set("right", rect.right);
		var_rect.Set("bottom", rect.bottom);
		// Word and rect to be set
		pp::VarDictionary var_word_rect;
		var_word_rect.Set("word", var_word);
		var_word_rect.Set("rect", var_rect);
		var_page_text.Set(i, var_word_rect);
	}
	PostPageTextMessage(safeInstance_, pageId, var_page_text);
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
	if (!stream_)
		return; // TODO (ilia) stream does not exist
	if (stream_->getError()) {
		error_ = "djvu:Error";
		return; // TODO (ilia) error in the stream
	}

	document_ = std::shared_ptr<ddjvu::File<renderer::Bitmap>>(new ddjvu::File<renderer::Bitmap>(stream_->getPool(), delegateBmpFactory_));
	auto document = document_;
	if (!document)
		return; // TODO (ilia) document does not exist
	if (!document->isDocumentValid()) {
		error_ = "djvu:Error";
		return;
	}

	int nPages = document->getPageNum();
	numberOfPages_ = nPages;

	pp::VarArray pageArray;
	pageArray.SetLength(nPages);

	for (int pageNum = 0; pageNum < nPages; pageNum++){
		ddjvu_pageinfo_t pageInfo =  document->getPageInfo(pageNum);

		// Send page as dictionary
		pp::VarDictionary page;

		page.Set("width", pageInfo.width);
		page.Set("height", pageInfo.height);
		page.Set("dpi", pageInfo.dpi);

		pageArray.Set(pageNum, page);
	}

	PostMessageToInstance(safeInstance_, CreateDictionaryReply(PPB_DECODE_FINISHED, pageArray));
	return;

	//pp::Var var_reply(error_);
	//instance_->PostMessage(var_reply);

	//updateThread_ = std::thread(&PluginWindow::updateThreadFuntion_, this);
}

void DjVuDecoder::decodePageThreadFunction_(std::string pageId, int pageNum,  pp::Var size_var, pp::Var frame_var) {
	std::string error;
	// Check if size is valid
	if (!size_var.is_dictionary()) {
		error = pageId + "Error: size is not a dictionary";
		PostErrorMessage(safeInstance_, error);
		return;
	}
	pp::VarDictionary size(size_var);
	std::shared_ptr<DjVuSize> valid_size = std::make_shared<DjVuSize>();
	if (!size.Get("width").is_int())
		error += " Error: size.width is not an integer value.";
	if (!size.Get("height").is_int())
		error += " Error: size.height is not an integer value.";
	if (!error.empty()) {
		error = pageId + error;
		PostErrorMessage(safeInstance_, error);
		return;
	}
	valid_size->width = size.Get("width").AsInt();
	valid_size->height = size.Get("height").AsInt();
	
	// Check if frame is null
	std::shared_ptr<DjVuFrame> valid_frame = std::make_shared<DjVuFrame>();
	if (frame_var.is_null()) {
		valid_frame->left = 0;
		valid_frame->top = 0;
		valid_frame->right = valid_size->width;
		valid_frame->bottom = valid_size->height;
	} else {
		if (!frame_var.is_dictionary()) {
			error = pageId + " Error: frame is not a dictionary";
			PostErrorMessage(safeInstance_, error);
			return;
		}
		pp::VarDictionary frame(frame_var);
		// Check if frame is valid
		int left, top, right, bottom, width, height;
		if (!frame.Get("left").is_int())
			error += " Error: frame.lift is not an integer value.";
		if (!frame.Get("top").is_int())
			error += " Error: frame.top is not an integer value.";
		if (!frame.Get("right").is_int())
			error += " Error: frame.right is not an integer value.";
		if (!frame.Get("bottom").is_int())
			error += " Error: frame.bottom is not an integer value.";
		left = frame.Get("left").AsInt();
		top = frame.Get("top").AsInt();
		right = frame.Get("right").AsInt();
		bottom = frame.Get("bottom").AsInt();
		if (!error.empty()) {
			error = pageId + error;
			PostErrorMessage(safeInstance_, error);
			return;
		}
		width = right - left;
		height = bottom - top;
		if (left >= right)
			error += " Error: frame.left is greater than frame.left.";
		if (top >= bottom)
			error += " Error: frame.top is greater than frame.bottom.";
		if (width > valid_size->width)
			error += " Error: frame width is greater than size.width.";
		if (height > valid_size->height)
			error += " Error: frame height is greater than size.height.";
		if (!error.empty()) {
			error = pageId + error;
			PostErrorMessage(safeInstance_, error);
			return;
		}
		valid_frame->left = left;
		valid_frame->top = top;
		valid_frame->right = right;
		valid_frame->bottom = bottom;
	}
	auto page = pages_[pageId];
	if (!page)
		return; // TODO (ilia) page does not exist
	page->size = valid_size;
	page->frame = valid_frame;
	
	auto document = document_;
	if (!document)
		return; // TODO (ilia) document does not exist
	document->getPageBitmap(pageNum, valid_size->width, valid_size->height, true, pageId);
	if (!document->isBitmapReady(pageId))
		return; // TODO (ilia) page was aborted
	auto bitmap = document->getPageBitmap(pageNum, valid_size->width, valid_size->height, true, pageId);
	// TODO (ilia) page wasn't decoded. May be we should try again or whatever
	if (!bitmap) {
		// TODO (ilia) now we will send fake image. Only for testing.
		page->bitmapStr = "iVBORw0KGgoAAAANSUhEUgAAAGQAAABkCAIAAAD/gAIDAAABDklEQVR4nO3TQQrCABAEwTHJ/7+sD1DEOkmg65CDSpCh97E9j+3c3p8fP/zylf7+dq+6diw/unb++y/cR2WBygKVBSoLVBaoLNBYoDMElQUqC1QWqCxQWaCyQGWBxgKdIagsUFmgskBlgcoCjQU6Q1BZoLJAZYHKApUFKgtUFmgs0BmCygKVBSoLVBaoLFBZoLJAY4HOEFQWqCxQWaCyQGWBxgKdIagsUFmgskBlgcoClQUqCzQW6AxBZYHKApUFKgtUFqgsUFmgsUBnCCoLVBaoLFBZoLJAY4HOEFQWqCxQWaCyQGWBygKVBRoLdIagskBlgcoClQUqC1QWqCzQWKAzBJUFKgtUFqgsUFngBaRCBIfbWmyRAAAAAElFTkSuQmCC";
	} else {
		page->bitmap = bitmap->getBmp();
	}
	//pp::VarDictionary bitmapAsDict = iBitmap->getBmp()->getAsDictionary();
	PostMessageToInstance(safeInstance_, CreateDictionaryReply(PPR_PAGE_READY, pageId));
	document->abortPageDecode(pageId);
	page->isDecoding = true;
}

void DjVuDecoder::sendPageThreadFunction_(std::string pageId) {
	auto page = pages_[pageId];
	if (!page)
		return; // TODO (ilia) page does not exist
	if (page->bitmap) {
		PostBitmapMessageAsBase64(safeInstance_, pageId, page->bitmap->getAsBase64Dictionary(page->frame));
	} else {
		pp::VarDictionary bmp;
		bmp.Set("bitsPixel", 3);
		bmp.Set("colors", 0);
		bmp.Set("width", 100);
		bmp.Set("height", 100);
		bmp.Set("rowSize", 300);
		bmp.Set("imageData", page->bitmapStr);
		PostBitmapMessageAsBase64(safeInstance_, pageId,bmp);
	}
	page->isSending = true;
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
