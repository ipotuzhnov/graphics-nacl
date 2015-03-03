#include "url_download_stream.h"

// TODO: remove this header

// Event class

// Public

// Constructor

UrlDownloadStream::UrlDownloadStream()
{
	error_ = 0;
	pool_ = DataPool::create();
}

// Destructor

// Functions

void UrlDownloadStream::addData(const void *data, int size)
{
	pool_->add_data(data, size);
}

void UrlDownloadStream::close()
{
	// Check file size
	if (pool_->get_length() <= 0) {
		error_ = 1; // Error: No data was delivered
		return;
	} else {
		if (!pool_->has_data(0, pool_->get_length())) {
			error_ = 2; // Error: Not all data was delivered
			return;
		}
	}
	error_ = 0;
	pool_->set_eof();
}

GP<DataPool> UrlDownloadStream::getPool()
{
	return pool_;
}

int UrlDownloadStream::getError()
{
	return error_;
}

// Private

// Functions
