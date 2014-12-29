#ifndef GRAPHICS_NACL_MESSAGES_H_
#define GRAPHICS_NACL_MESSAGES_H_

namespace {
	
	// @TODO (ilia)
	// Message format <target>:<message_type>:<command>:<args>
	// Instead of string messages we'll use Arrays or Dictionaries
	// So new message format is Dictionary { message: pp::Var.AsString(), args: <string, array or dictionary> } 

	// Messages
	// ppb - Pepper Browser
	// ppp - Pepper Plugin
	// ppd - Graphics NaCl Decoder
	// ppr - Graphics NaCl Renderer

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
	// Decode page
	// args = pp::Dictionary <pp::VarDictionary<pageId, pageNum, width, height>>
	const char* const PPD_DECODE_PAGE = "PPD_DECODE_PAGE";

	// Page is decoded
	// args = pageId
	const char* const PPR_PAGE_READY = "PPR_PAGE_READY";
	// Receiving page from JS
	// args = pp::Dictionary
	const char* const PPR_SEND_PAGE = "PPR_SEND_PAGE";

	// Request page pp::Dictionary
	const char* const PPD_GET_PAGE = "PPD_GET_PAGE";

	// Release page
	// args = ppVar.AsString() <pageId>
	const char* const PPD_RELEASE_PAGE = "PPD_RELEASE_PAGE";

	// Error
	// args = pp::Dictionary
	const char* const PPB_PLUGIN_ERROR = "PPB_PLUGIN_ERROR";

} // namespace

#endif GRAPHICS_NACL_MESSAGES_H_
