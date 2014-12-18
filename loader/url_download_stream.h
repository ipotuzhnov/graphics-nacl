#ifndef URL_DOWNLOAD_STREAM_H_
#define URL_DOWNLOAD_STREAM_H_

#include "DataPool.h"

class UrlDownloadStream
{
public:
	UrlDownloadStream();

	void addData(const void *data, int size);
	void close();
	GP<DataPool> getPool();
	/* 
	return error code:
	0: Error: No, success
	1: Error: No data was delivered
	2: Error: Not all data was delivered
	*/
	int getError();
private:
	GP<DataPool> pool_;
	int error_;
};

#endif // URL_DOWNLOAD_STREAM_H_
