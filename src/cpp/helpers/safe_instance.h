#ifndef __SAFE_INSTANCE_H__
#define __SAFE_INSTANCE_H__

#include <atomic>

#include "ppapi/cpp/instance.h"

class SafeInstance {
public:
	pp::Instance *instance; // Weak pointer.
	std::atomic<bool> isValid;
	SafeInstance(pp::Instance *instance) : instance(instance) {
		if (instance != nullptr)
			isValid = true;
		else
			isValid = false;
	}
};

#endif // __SAFE_INSTANCE_H__
