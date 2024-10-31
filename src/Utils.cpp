#include "Utils.h"

void SplitString(const std::string& s, std::vector<std::string>& tokens, const std::string& delimiters = ",")
{
	std::string::size_type lastPos = s.find_first_not_of(delimiters, 0);
	std::string::size_type pos = s.find_first_of(delimiters, lastPos);
	while (std::string::npos != pos || std::string::npos != lastPos)
	{
		tokens.push_back(s.substr(lastPos, pos - lastPos));
		lastPos = s.find_first_not_of(delimiters, pos);
		pos = s.find_first_of(delimiters, lastPos);
	}
}

std::pair<std::string, std::string> SplitPairStrByComma(const std::string& s)
{
	const std::string& delimiter = ",";
	std::vector<std::string> tokens;
	std::pair<std::string, std::string> returnPair = std::pair<std::string, std::string>();

	SplitString(s, tokens, delimiter);

	if (tokens.size() >= 2)
	{
		returnPair.first = tokens[0];
		returnPair.second = tokens[1];
	}

	return returnPair;
}

std::string removeSuffix(const std::string& str) {
	size_t found = str.rfind('-'); // 查找最后一个'-'字符的位置  
	if (found == std::string::npos) {
		// 如果没有找到'-'，则原样返回字符串  
		return str;
	}

	// 检查'-'后是否全为数字  
	bool allDigits = true;
	for (size_t i = found + 1; i < str.size(); ++i) {
		if (!std::isdigit(str[i])) {
			allDigits = false;
			break;
		}
	}

	if (allDigits) {
		// 如果'-'后全是数字，则裁剪掉这部分  
		return str.substr(0, found);
	}

	// 否则，返回原字符串  
	return str;
}

time_t FloorDuration(const timespec& end, const timespec& start)
{
	time_t sec, nsec;
	sec = end.tv_sec - start.tv_sec;

	nsec = end.tv_nsec - start.tv_nsec;
	if (nsec < 0)
	{
		return sec - 1;
	}

	return sec;
}

std::string GetTimeSec(time_t sec)
{
	struct tm time;
	localtime_s(&time, &sec);

	std::stringstream ss;

	ss << time.tm_year << "-"
		<< time.tm_mon << "-"
		<< time.tm_mday << "-"
		<< time.tm_hour << "-"
		<< time.tm_min << "-"
		<< time.tm_sec;

	return ss.str();
}