#include "perfdata.h"

PerfData* PerfData::_instance = nullptr;

PerfData::PerfData()
{

}
PerfData* PerfData::instance()
{
    if(!_instance)
    {
        _instance = new PerfData;
    }
    return _instance;
}

unsigned long long PerfData::frameTime(int iFrame)
{
	unsigned long long t1 = _frameTimes[iFrame].t1;
	unsigned long long t2 = _frameTimes[iFrame].t2;
	unsigned long long dt = t2 - t1;
	return dt;
}
void PerfData::reset()
{
	_strMaps.clear();
	_PerfStampBuf.clear();
	_PerfStampBrackets.clear();
	_frameTimes.clear();
}
unsigned long long PerfData::frameLabelTime(int iFrame, QString strLabel,int &oCount)
{
	TimeRange_t &frame = _frameTimes[iFrame];

	int s1 = frame.sampleIndex1;
	int s2 = frame.sampleIndex2;

	int msec = 0;
	int count = 0;

	for (int j = s1; j < s2; j++)
	{

		PerfStamp_t stamp = _PerfStampBuf[j].stamp;
		SampleBracket_t bracket = _PerfStampBrackets[j];
		long long dTime = 0;

		const char* str = cstr(stamp.Label_);
		if (strcmp(str, "error") == 0)
		{

		}
		else
		{
			if (strLabel.compare(str) == 0)
			{
				if (bracket.type == 2)
				{
					int x = bracket.sampleIndex;
					dTime = _PerfStampBuf[j].stamp.Time - _PerfStampBuf[x].stamp.Time;
					msec += dTime;
					count++;
				}
			}
		}

	}

	oCount = count;

	return msec;
}