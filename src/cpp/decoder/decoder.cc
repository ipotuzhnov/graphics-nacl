#include "decoder.h"

#include <locale>
#include <codecvt>
#include <tuple>

#include "../helpers/messages.h"
#include "../helpers/message_helper.h"

#include <stdio.h>

DjVuDecoder::DjVuDecoder()
	: error_("djvu:Ok"), numberOfPages_(0) {
}

DjVuDecoder::~DjVuDecoder() {
	document_->stopMessageHandling();
	for (auto it = pages_.begin(); it != pages_.end(); ++it) {
		if (it->second->decoding_thread.joinable())
			it->second->decoding_thread.join();

		if (it->second->sending_thread.joinable())
			it->second->sending_thread.join();
	}

	if (decodeThread_.joinable())
		decodeThread_.join();
}

void DjVuDecoder::startDocumentDecode(std::shared_ptr<SafeInstance> safeInstance, std::shared_ptr<UrlDownloadStream> stream) {
	stream_ = stream;
	safeInstance_ = safeInstance;
	delegateBmpFactory_ = std::make_shared<BmpFactoryDelegate>();

	decodeThread_ = std::thread(&DjVuDecoder::decodeThreadFuntion_, this);
}

void DjVuDecoder::startPageDecode(std::string pageId, int pageNum, pp::Var size, pp::Var frame) {
	if (pages_.find(pageId) != pages_.end())
		return PostErrorMessage(safeInstance_, "Page with id " + pageId + " already exists.");

	if (pageNum >= numberOfPages_)
		return PostErrorMessage(safeInstance_, "Page id " + pageId + " page number is out of range.");

	pages_[pageId] = std::make_shared<DjVuPage>();
	auto page = pages_[pageId];
	if (!page)
		return PostErrorMessage(safeInstance_, "Can't create page with id " + pageId + ". Can't find page.");

	page->pageId = pageId;
	page->pageNum = pageNum;
	page->decoding_thread = std::thread(&DjVuDecoder::decodePageThreadFunction_, this, pageId, pageNum, size, frame);
}

void DjVuDecoder::sendPageAsBase64(std::string pageId) {
	if (pages_.find(pageId) == pages_.end())
		return PostErrorMessage(safeInstance_, "Can't send page with id " + pageId + ". Page does not exist.");

	// Send image in separate thread
	auto page = pages_[pageId];
	if (!page)
		return PostErrorMessage(safeInstance_, "Can't send page with id " + pageId + ". Can't find page.");

	page->sending_thread = std::thread(&DjVuDecoder::sendPageThreadFunction_, this, pageId);
}

void DjVuDecoder::releasePage(std::string pageId) {
	if (pages_.find(pageId) == pages_.end())
		return PostErrorMessage(safeInstance_, "Can't release page with id " + pageId + ". Page does not exist.");

	auto page = pages_[pageId];
	if (page->decoding_thread.joinable())
		page->decoding_thread.join();
	if (page->sending_thread.joinable())
		page->sending_thread.join();
	document_->removePage(pageId);
	pages_.erase(pageId);
}

void DjVuDecoder::getPageText(std::string pageId, int pageNum) {
	if (pageNum >= numberOfPages_)
		return PostErrorMessage(safeInstance_, "Page id " + pageId + " page number is out of range.");
	auto document = document_;
	if (!document)
		return PostErrorMessage(safeInstance_, "Can't get page's text with id " + pageId + ". Document does not exist.");

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

std::shared_ptr<decoder::Bitmap> DjVuDecoder::getPageBmp(std::string pageId) {
	if (pages_.find(pageId) != pages_.end()) {
		return pages_[pageId]->bitmap;
	} else {
		return nullptr;
	}
}

void DjVuDecoder::decodeThreadFuntion_() {
	if (!stream_)
		return PostErrorMessage(safeInstance_, "Can't start document decoding. Stream does not exist.");

	if (stream_->getError()) {
		error_ = stream_->getError();
		return PostErrorMessage(safeInstance_, "Can't start document decoding. Stream has error: " + error_);
	}

	document_ = std::shared_ptr<ddjvu::File<decoder::Bitmap>>(new ddjvu::File<decoder::Bitmap>(stream_->getPool(), delegateBmpFactory_));
	auto document = document_;
	if (!document)
		return PostErrorMessage(safeInstance_, "Can't start document decoding. Can't create document.");
	if (!document->isDocumentValid())
		return PostErrorMessage(safeInstance_, "Can't start document decoding. Document is corrupted.");

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
		return PostErrorMessage(safeInstance_, "Can't decode page with id " + pageId + ". Can't find page.");

	page->size = valid_size;
	page->frame = valid_frame;

	auto document = document_;
	if (!document) {
		return PostErrorMessage(safeInstance_, "Can't decode page with id " + pageId + ". Document does not exist.");
	}

	auto dpage = document->getPage(pageId, pageNum, valid_size->width, valid_size->height);
	dpage->start();

	if (!dpage->ready())
		return PostErrorMessage(safeInstance_, "Can't decode page with id " + pageId + ". Page was aborted.");

	auto bitmap = dpage->getBitmap();
	// TODO (ilia) page wasn't decoded. May be we should try again or whatever.
	if (!bitmap) {
		// TODO (ilia) now we will send fake image. Only for testing.
		page->bitmapStr = "iVBORw0KGgoAAAANSUhEUgAAAGQAAABkCAIAAAD/gAIDAAABDklEQVR4nO3TQQrCABAEwTHJ/7+sD1DEOkmg65CDSpCh97E9j+3c3p8fP/zylf7+dq+6diw/unb++y/cR2WBygKVBSoLVBaoLNBYoDMElQUqC1QWqCxQWaCyQGWBxgKdIagsUFmgskBlgcoCjQU6Q1BZoLJAZYHKApUFKgtUFmgs0BmCygKVBSoLVBaoLFBZoLJAY4HOEFQWqCxQWaCyQGWBxgKdIagsUFmgskBlgcoClQUqCzQW6AxBZYHKApUFKgtUFqgsUFmgsUBnCCoLVBaoLFBZoLJAY4HOEFQWqCxQWaCyQGWBygKVBRoLdIagskBlgcoClQUqC1QWqCzQWKAzBJUFKgtUFqgsUFngBaRCBIfbWmyRAAAAAElFTkSuQmCC";
	} else {
		page->bitmap = bitmap->getBmp();
	}

	PostMessageToInstance(safeInstance_, CreateDictionaryReply(PPB_PAGE_READY, pageId));
	page->isDecoding = true;
}

void DjVuDecoder::sendPageThreadFunction_(std::string pageId) {
	if (!pages_[pageId])
		return PostErrorMessage(safeInstance_, "Can't send page with id " + pageId + ". Page does not exist.");

	auto bitmap = pages_[pageId]->bitmap;
	if (bitmap) {
		std::string error;
		pp::VarDictionary bmp;
		auto error_bmp = bitmap->getAsBase64Dictionary(pages_[pageId]->frame);
		std::tie (error, bmp) = bitmap->getAsBase64Dictionary(pages_[pageId]->frame);
		if (!error.empty()) {
			return PostErrorMessage(safeInstance_, "Can't send page with id " + pageId + ". " + error);
		} else {
			return PostBitmapMessageAsBase64(safeInstance_, pageId, bmp);
		}
	} else {
		pp::VarDictionary bmp;
		bmp.Set("bitsPixel", 3);
		bmp.Set("colors", 0);
		bmp.Set("width", 100);
		bmp.Set("height", 100);
		bmp.Set("rowSize", 300);
		bmp.Set("imageData", pages_[pageId]->bitmapStr);
		PostBitmapMessageAsBase64(safeInstance_, pageId,bmp);
	}
	pages_[pageId]->isSending = true;
}
