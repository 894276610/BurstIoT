#ifndef _KEXCEPTION_H_
#define _KEXCEPTION_H_

#include <exception> 
#include <string>
#include <iostream>
#include "magic_enum.hpp"

class KException : public std::exception
{
public:

	enum class Code
	{
		DEVICE_MAPPING_OPEN_ERROR = 1,
		DEVICE_MAPPING_TYPE_ERROR,
		PCAP_FORMAT_ERROR,
		PCAP_CANNOT_OPEN,
		USER_SETTING_OPEN_ERROR,
		OUTPUT_ERROR,
		FILTER_ERROR,
		WRITE_PCAP_ERROR,
		EMPTY_REORGANIZER_BUFFER_ERROR,
		DEVICE_THREADPOOL_ERROR,
		NOT_IPV4_PACKET,
		OTHER_DEVICE_PACKET,
		BURST_NULLPTR,
		BURST_STRANGE,
		BURST_NUMBER_MISMATCH,
		BURST_UNI_NUMBER_MISMATCH,
		OPEN_ERROR,
		UNKNOWN_BURST_DEVICE,
		BURST_UNI_NUMBER_OVERFLOW,
		DEVICE_NOT_FOUND,
		SHOULD_NOT_EMPTY,
		NOT_ETHERNET_PACKET,
		DATASET_PATH_NOT_EXIST,
		PCAP_TIME_MESS,
		CLEAR_ERROR,
		INVALID_PKT
	};

	KException() {}
	KException(KException::Code code) : code(code) {}
	
	virtual void print() const {
		std::cout << "Exception:" << magic_enum::enum_name(code) << std::endl;
	}

private:
	std::string detail;
	KException::Code code;
};

class KHintException : public KException
{
public:
	enum class Code
	{
		DATASET_INACTIVE_HINT = 1,
		USER_SETTING_FORMAT_ERROR
	};

	KHintException(KHintException::Code code) : code(code) {}

	virtual void print() const {
		std::cout << "Hint:" << magic_enum::enum_name(code) << std::endl;
	}

private:
	KHintException::Code code;
};

#endif
