#ifndef __KTIMER_H__
#define __KTIMER_H__

#include <chrono>
#include <iostream>
#include <algorithm>
#include <string>
#include <fstream>
#include <filesystem>
#include "AConfig.h"
#include <thread>
#include <mutex>

struct ProfileResult
{
	std::string Name;
	long long Start, End;
	std::thread::id ThreadID;
};

struct InstrumentationSession
{
	std::string Name;
};

class Instrumentor
{
private:
	InstrumentationSession* m_CurrentSession;
	std::ofstream m_OutputStream;
	int m_ProfileCount = 0;
	std::mutex mtx;
	std::mutex profileCountMtx;

public:
	Instrumentor()
		: m_CurrentSession(nullptr), m_ProfileCount(0)
	{
	}
	
	void BeginSession(const std::string& name, const std::string& filepath)
	{
		if (m_ProfileCount != 0)
		{
			std::cout << "something wrong m_profile count is not 0";
		}
		m_OutputStream.open(filepath);
		WriteHeader();
		m_CurrentSession = new InstrumentationSession{ name };
	}

	void EndSession()
	{
		WriteFooter();
		m_OutputStream.close();
		delete m_CurrentSession;
		m_CurrentSession = nullptr;			
		m_ProfileCount = 0;
	}

	void WriteProfile(const ProfileResult& result)
	{
		std::unique_lock<std::mutex> lock(mtx);
	
		if (m_ProfileCount++ > 0)
		{
			m_OutputStream << ",";
		}
		
		std::string name = result.Name;
		std::replace(name.begin(), name.end(), '"', '\'');

		m_OutputStream << "{";
		m_OutputStream << "\"cat\":\"function\",";
		m_OutputStream << "\"dur\":" << (result.End - result.Start) << ',';
		m_OutputStream << "\"name\":\"" << name << "\",";
		m_OutputStream << "\"ph\":\"X\",";
		m_OutputStream << "\"pid\":0,";
		m_OutputStream << "\"tid\":" << result.ThreadID << ",";
		m_OutputStream << "\"ts\":" << result.Start;
		m_OutputStream << "}";

		m_OutputStream.flush();
	}

	void WriteHeader()
	{
		m_OutputStream << "{\"otherData\": {},\"traceEvents\":[";
		m_OutputStream.flush();
	}

	void WriteFooter()
	{
		m_OutputStream << "]}";
		m_OutputStream.flush();
	}

	static Instrumentor& Get()
	{
		static Instrumentor* instance = new Instrumentor();
		return *instance;
	}

};

class InstrumentationTimer
{
public:
	InstrumentationTimer(const char* name)
		:m_Name(name), m_Stopped(false)
	{
		m_StartTimePoint = std::chrono::high_resolution_clock::now();
	}

	~InstrumentationTimer()
	{
		Stop();
	}

	void Stop()
	{
		auto endTimepoint = std::chrono::high_resolution_clock::now();

		auto start = std::chrono::time_point_cast<std::chrono::microseconds>(m_StartTimePoint).time_since_epoch().count();
		auto stop = std::chrono::time_point_cast<std::chrono::microseconds>(endTimepoint).time_since_epoch().count();
		
		std::thread::id threadID = std::this_thread::get_id();
		Instrumentor::Get().WriteProfile({ m_Name, start, stop, threadID});
	}
private:
	const char* m_Name;
	std::chrono::time_point< std::chrono::high_resolution_clock> m_StartTimePoint;
	bool m_Stopped;
};

#define PROFILING 1
#if PROFILING
#define PROFILE_SCOPE(name) InstrumentationTimer time##__LINE__(name)
#define PROFILE_FUNCTION() PROFILE_SCOPE(__FUNCSIG__)
#else
#define PROFILE_SCOPE(name)
#endif

#endif