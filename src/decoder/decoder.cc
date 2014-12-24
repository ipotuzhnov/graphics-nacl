#include "decoder.h"

#include "ppapi/cpp/var.h"
#include "ppapi/cpp/var_array.h"
#include "ppapi/cpp/var_dictionary.h"

#include "../helpers/message_helper.h"

DjVuDecoder::DjVuDecoder(pp::Instance* instance, std::shared_ptr<UrlDownloadStream> stream)
	: stream_(stream)
	, delegateBmpFactory_(std::make_shared<BmpFactoryDelegate>(instance))
	, error_("djvu:Ok")
	, instance_(instance) {
}

DjVuDecoder::~DjVuDecoder() {
	if (updateThread_.joinable())
		updateThread_.join();
	if (decodeThread_.joinable())
		decodeThread_.join();
}

 void DjVuDecoder::start() {
	decodeThread_ = std::thread(&DjVuDecoder::decodeThreadFuntion_, this);
}

std::shared_ptr<ddjvu::IBmp<pp::ImageData>> DjVuDecoder::getBmp() {
	return bmp_;
}

void DjVuDecoder::decodeThreadFuntion_() {
	if (stream_) {
		if (stream_->getError()) {
			error_ = "djvu:Error";
			return;
		}

		document_ = std::shared_ptr<ddjvu::File<pp::ImageData>>(new ddjvu::File<pp::ImageData>(stream_->getPool(), delegateBmpFactory_));


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
	//array.Set(0, x_angle_);
	//array.Set(1, y_angle_);
	//PostMessage(array);
	for (int pageNum = 0; pageNum < nPages; pageNum++){
		ddjvu_pageinfo_t pageInfo =  document_->getPageInfo(pageNum);

		// Send page as array
		/* 
		pp::VarArray page;
		
		page.SetLength(3);
		page.Set(0, pp::Var(pageInfo.width));
		page.Set(1, pp::Var(pageInfo.height));
		page.Set(2, pp::Var(pageInfo.dpi));

		pageArray.Set(pageNum, page);
		*/

		// Send page as dictionary
		pp::VarDictionary page;

		page.Set("width", pageInfo.width);
		page.Set("height", pageInfo.height);
		page.Set("dpi", pageInfo.dpi);

		pageArray.Set(pageNum, page);


		/*
		Page p;
		p.w = pageInfo.width;
		p.h = pageInfo.height;
		p.w_screen = MulDiv(p.w, dpiX, pageInfo.dpi);
		p.h_screen = MulDiv(p.h, dpiY, pageInfo.dpi);
		pages_.push_back(p);
		if (p.w_screen > pageWidth_)
		pageWidth_ = p.w_screen;
		if (p.h_screen > pageHeight_)
		pageHeight_ = p.h_screen;
		*/
	}

	PostObjectMessage(instance_, "browser", "pages", pageArray);
	return;

	//pp::Var var_reply(error_);
	//instance_->PostMessage(var_reply);

	//updateThread_ = std::thread(&PluginWindow::updateThreadFuntion_, this);
}

void DjVuDecoder::updateThreadFuntion_() {
	/*
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
	*/
}
