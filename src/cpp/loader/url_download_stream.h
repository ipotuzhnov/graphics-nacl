#ifndef __URL_DOWNLOAD_STREAM_H__
#define __URL_DOWNLOAD_STREAM_H__

#include <string>

#include "DataPool.h"

class UrlDownloadStream
{
public:
	UrlDownloadStream();

	void addData(const void *data, int size);
	void close();
	GP<DataPool> getPool();
	std::string getError();
private:
	GP<DataPool> pool_;
	std::string error_;
};

#endif // __URL_DOWNLOAD_STREAM_H__
