#ifndef __GRAPHICS_NACL_MESSAGES_H__
#define __GRAPHICS_NACL_MESSAGES_H__

namespace {

	// @TODO (ilia)
	// Message format <target>:<message_type>:<command>:<args>
	// Instead of string messages we'll use Arrays or Dictionaries
	// So new message format is Dictionary { message: pp::Var.AsString(), args: <string, array or dictionary> }

	// Messages
	// ppb - Pepper Browser
	// ppp - Pepper Plugin
	// ppd - Graphics NaCl Decoder

	// Download document
	const char* const PPD_DOWNLOAD_START = "PPD_DOWNLOAD_START";
	// Update downloading progress
	// args = pp::Var.AsInt()
	const char* const PPB_DOWNLOAD_PROGRESS = "PPB_DOWNLOAD_PROGRESS";
	// Notify browser that download is finished
	const char* const PPB_DOWNLOAD_FINISHED = "PPB_DOWNLOAD_FINISHED";

	// Start decoding document
	const char* const PPD_DECODE_START = "PPD_DECODE_START";
	// Notify browser that decode is finished
	// args = pp::VarArray <pp::VarDictionary<width, height, dpi>>
	const char* const PPB_DECODE_FINISHED = "PPB_DECODE_FINISHED";

	// Tell plugin to decode page
	// args = pp::VarDictionary <pp::VarDictionary<pageId, pageNum, width, height>>
	const char* const PPD_DECODE_PAGE = "PPD_DECODE_PAGE";
	// Page is decoded
	// args = pageId
	const char* const PPB_PAGE_READY = "PPB_PAGE_READY";

	// Request page as base64
	// args = pp::Var.AsString() <pageId>
	const char* const PPD_GET_PAGE_AS_BASE64 = "PPD_GET_PAGE_AS_BASE64";
	// Send page to the browser as base64
	// args = pp::VarDictionary
	const char* const PPB_SEND_PAGE_AS_BASE64 = "PPB_SEND_PAGE_AS_BASE64";

	// Request page's text
	// args = pp::Var.AsString() <pageId, pageNum>
	const char* const PPD_GET_PAGE_TEXT = "PPD_GET_PAGE_TEXT";
	// Send page's text to the browser
	// args = pp::VarDictionary <pageId, pp::VarArray<pp::VarDictionary<rect, word>>>
	const char* const PPB_SEND_PAGE_TEXT = "PPB_SEND_PAGE_TEXT";

	// Release page
	// args = pp::Var.AsString() <pageId>
	const char* const PPD_RELEASE_PAGE = "PPD_RELEASE_PAGE";

	// Error
	// args = pp::VarDictionary
	const char* const PPB_PLUGIN_ERROR = "PPB_PLUGIN_ERROR";

} // namespace

#endif // __GRAPHICS_NACL_MESSAGES_H__
