#include "decoder.h"

#include "ppapi/cpp/var.h"

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

 void DjVuDecoder::start(pp::Size size) {
	size_ = size;
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

	if (nPages > 0) {
		bmp_ = document_->getPageBitmap(0, size_.width(), size_.height(), true);
	}

	pp::Var var_reply(error_);
	instance_->PostMessage(var_reply);

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
