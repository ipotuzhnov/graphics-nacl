#ifndef MESSAGE_HELPER_FUNCTIONS_H_
#define MESSAGE_HELPER_FUNCTIONS_H_

#include <string>
#include <memory>

#include "ppapi/cpp/instance.h"
#include "ppapi/cpp/var.h"
#include "ppapi/cpp/var_array.h"
#include "ppapi/cpp/var_dictionary.h"

#include "safe_instance.h"
#include "messages.h"

#ifdef WIN32
#undef PostMessage
// Allow 'this' in initializer list
#pragma warning(disable : 4355)
#endif
#undef mbstate_t

namespace {

	void PostMessageToInstance(std::shared_ptr<SafeInstance> safeInstance, pp::Var message) {
		if (safeInstance->isValid) {
			safeInstance->instance->PostMessage(message);
		}
	}

	pp::VarDictionary CreateDictionaryReply(pp::Var message, pp::Var args) {
		pp::VarDictionary reply;
		reply.Set("message", message);
		reply.Set("args", args);
		return reply;
	}

	// Post error message to the browser
	void PostErrorMessage(std::shared_ptr<SafeInstance> safeInstance, std::string text, std::string code = "0", pp::Var object = "null") {
		pp::VarDictionary var_error;
		var_error.Set("text", text);
		var_error.Set("code", code);
		var_error.Set("object", object);
		pp::VarDictionary error_message(CreateDictionaryReply(PPB_PLUGIN_ERROR, var_error));
		PostMessageToInstance(safeInstance, error_message); 
	}

	// Post bitmap to the browser
	void PostBitmapMessage(std::shared_ptr<SafeInstance> safeInstance, std::string pageId, pp::VarDictionary bitmapAsDict) {
		pp::VarDictionary var_bitmap;
		var_bitmap.Set("pageId", pageId);
		var_bitmap.Set("page", bitmapAsDict);
		PostMessageToInstance(safeInstance, CreateDictionaryReply(PPR_SEND_PAGE, var_bitmap));
	}

	// Post bitmap as base64 to the browser
	void PostBitmapMessageAsBase64(std::shared_ptr<SafeInstance> safeInstance, std::string pageId, pp::VarDictionary bitmapAsDict) {
		pp::VarDictionary var_bitmap;
		var_bitmap.Set("pageId", pageId);
		var_bitmap.Set("page", bitmapAsDict);
		PostMessageToInstance(safeInstance, CreateDictionaryReply(PPB_SEND_PAGE_AS_BASE64, var_bitmap));
	}

	// Post page's text to the browser
	void PostPageTextMessage(std::shared_ptr<SafeInstance> safeInstance, std::string pageId, pp::VarArray pageText) {
		pp::VarDictionary var_page_text;
		var_page_text.Set("pageId", pageId);
		var_page_text.Set("pageText", pageText);
		PostMessageToInstance(safeInstance, CreateDictionaryReply(PPB_SEND_PAGE_TEXT, var_page_text));
	}

	/*
	// Post object to the browser
	void PostObjectMessage(pp::Instance *instance, pp::Var target, pp::Var object_name, pp::Var object) {
		pp::VarDictionary var_object;
		var_object.Set("");
		pp::VarDictionary object_message(CreateDictionaryReply("PPB_OBJECT", object));
		PostMessageToInstance(instance, object_message);
	}
	*/

}

#endif // MESSAGE_HELPER_FUNCTIONS_H_
