#include "Klog.h"

void Klog::exception(const KException& exception)
{
	exception.print();
}

void Klog::hint(const KHintException& exception)
{
	std::cout << "Hint: " << exception.what() << std::endl;
}

