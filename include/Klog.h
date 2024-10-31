#ifndef KLOG_H
#define KLOG_H

#include "magic_enum.hpp"
#include "KException.h"
#include <iostream>

class Klog {

public:
	static void exception(const KException& exception);
	static void hint(const KHintException& exception);
};

#endif