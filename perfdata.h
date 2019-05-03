#ifndef PERFDATA_H
#define PERFDATA_H

#define PERF_STAMP_VERSION 1
#define PERF_STAMP_FILE_ID 0x12345678
#include <map>
#include <vector>
#include <qstring.h>


struct PerfStamp_t
{
    unsigned int File;
	unsigned int Func;
	unsigned int Label_;
    unsigned short Line;
    unsigned short Type;//1 Start,2 End
    unsigned long long Time;
};

struct PerfStampData_t
{
	PerfStamp_t stamp;
	const char* name;
};

struct PerfHeader_t
{
    unsigned int magic;
    unsigned int ver;
    unsigned int count;
};
struct TimeRange_t
{
    unsigned long long t1,t2;
    int frameIndex;
	int sampleIndex1,sampleIndex2;
};

struct SampleBracket_t
{
	int type;
	int sampleIndex;
};


class PerfData
{
public:
    std::map<unsigned int,std::string> _strMaps;
	std::vector<PerfStampData_t> _PerfStampBuf;
	std::vector<SampleBracket_t> _PerfStampBrackets;

    std::vector<TimeRange_t> _frameTimes;
	

    PerfData();
	std::vector<TimeRange_t>& frameTimes(){ return _frameTimes; }
	std::vector<SampleBracket_t>& sampleBrackets(){ return _PerfStampBrackets; }
	std::map<unsigned int, std::string>& strMaps(){ return _strMaps; }
	std::vector<PerfStampData_t>& samples(){ return _PerfStampBuf; }
	void reset();
	unsigned long long frameTime(int iFrame);
	unsigned long long frameLabelTime(int iFrame, QString strLabel, int &oCount);
	const char* cstr(unsigned int key)
	{  
		auto kItor = _strMaps.find(key);
		if (kItor != _strMaps.end())
		{
			
			const char* s = kItor->second.c_str();
			if (strlen(s) > 0)
				return s;
			else
				return nullptr;
		}

		return "(error)";
	}
    static PerfData* instance();
    static PerfData* _instance;
};

#endif // PERFDATA_H
