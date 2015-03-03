/* From http://stackoverflow.com/questions/1821806/how-to-encode-png-to-buffer-using-libpng
*/

#ifndef __PNG_VIRTUAL_FILE_H__
#define __PNG_VIRTUAL_FILE_H__

#include <memory>

#include <png.h>

namespace {

	/* structure to store PNG image bytes */
	struct mem_encode {
		char *buffer;
		size_t size;
	};

	void png_write_data_callback(png_structp png_ptr, png_bytep data, png_size_t length) {
		/* with libpng15 next line causes pointer deference error; use libpng12 */
		struct mem_encode* p=(struct mem_encode*)png_get_io_ptr(png_ptr); /* was png_ptr->io_ptr */
		size_t nsize = p->size + length;

		/* allocate or grow buffer */
		if(p->buffer)
			p->buffer = (char *)realloc(p->buffer, nsize);
		else
			p->buffer = (char *)malloc(nsize);

		// TODO (ilia) propper error handling
		if(!p->buffer)
			png_error(png_ptr, "Write Error");

		/* copy new bytes to end of buffer */
		memcpy(p->buffer + p->size, data, length);
		p->size += length;
	}

	/* This is optional but included to show how png_set_write_fn() is called */
	void png_flush_callback(png_structp png_ptr) {}

}

#endif // __PNG_VIRTUAL_FILE_H__
