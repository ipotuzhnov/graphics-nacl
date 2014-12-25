#ifndef MESSAGE_HELPER_FUNCTIONS_H_
#define MESSAGE_HELPER_FUNCTIONS_H_

#include "ppapi/cpp/instance.h"
#include "ppapi/cpp/var.h"
#include "ppapi/cpp/var_array.h"
#include "ppapi/cpp/var_dictionary.h"

#ifdef WIN32
#undef PostMessage
// Allow 'this' in initializer list
#pragma warning(disable : 4355)
#endif
#undef mbstate_t

namespace {

	void PostMessageToInstance(pp::Instance *instance, pp::Var message) {
		if (instance) {
			instance->PostMessage(message);
		}
	}

	pp::VarDictionary CreateDictionaryReply(pp::Var target, pp::Var type, pp::Var name, pp::Var args) {
		pp::VarDictionary reply;
		reply.Set("target", target);
		reply.Set("type", type);
		reply.Set("name", name);
		reply.Set("args", args);
		return reply;
	}

	// Post error message to the browser
	void PostErrorMessage(pp::Instance *instance, std::string text, int code = 0, pp::Var object = "null") {
		pp::VarDictionary var_error;
		var_error.Set("text", text);
		var_error.Set("code", code);
		var_error.Set("object", object);
		pp::VarDictionary error_message(CreateDictionaryReply("browser", "notify", "error", var_error));
		PostMessageToInstance(instance, error_message); 
	}

	// Post object to the browser
	void PostObjectMessage(pp::Instance *instance, pp::Var target, pp::Var object_name, pp::Var object) {
		pp::VarDictionary object_message(CreateDictionaryReply("browser", "object", object_name, object));
		PostMessageToInstance(instance, object_message);
	}

}

#endif // MESSAGE_HELPER_FUNCTIONS_H_
