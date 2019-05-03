#ifndef PERF_STAMP_H
#define PERF_STAMP_H

#include <stdio.h>
#include <map>
#include <vector>
#include <string>

#include "Types.h"

#ifdef ENGINE_DLL
#	if defined(_WINDOWS)
#		if defined(DLL_EXPORT)
#			define ENGINE_API     __declspec(dllexport)
#		else         /* use a DLL library */
#			define ENGINE_API     __declspec(dllimport)
#		endif  
#	endif
#else
#	define ENGINE_API
#endif

#define PERF_STAMP_VERSION 1
#define PERF_STAMP_FILE_ID 0x12345678
#define MAX_HRKS 256

struct PerfStamp_t
{
	const char* File;
	const char* Func;
	const char* Label;
	unsigned short Line;
	unsigned short Type;//1 Start,2 End,3,Tag
	unsigned long long Time;
};

struct PerfHeader_t
{
	unsigned int magic;
	unsigned int ver; 	
	unsigned int count;
};

struct PerfContext_t
{
	PerfStamp_t* _PerfStampBuf;
	int _nPerfStampBuf;
	unsigned long long StampCount;
	int StampIndex;
	FILE*_fpStream;
	int Id;
	std::map<const char*, std::string> _strMaps;
	int Signal;//-1=release;0=init;1=work;2=pause
	const char* _HrkLabels[MAX_HRKS];
	int _HrkIndex;
	bool _bFrameStart;
};

class ENGINE_API PerfSampler
{
public:
	PerfSampler();
	~PerfSampler();
	static PerfSampler* instance();
	void init(const char* Filename,int MaxStamps);
	void start();
	
	void resetContext(PerfContext_t*Ctx);
	void flushContext(PerfContext_t*Ctx);
	PerfContext_t* context();
	PerfStamp_t* alloc();
	void flush();
	void pause();
	void resume();
	int isPaused();
	FILE* stream();
	PerfStamp_t* buf();
	int max_count();
	int index();
	void close();
	void startFrame();
	void endFrame();
	bool frameStarted();
	void _pushLabel(const char* text);
	void _popLabel(const char* text);
protected:
	void _initStreamFile(PerfContext_t*Ctx);
	void _initStampBuf(PerfContext_t*Ctx);
	void _writeHeader(PerfContext_t* Ctx);
	void _writeStringTable(PerfContext_t* Ctx);
	void _releaseContext(PerfContext_t* Ctx);

	static PerfSampler _Instance;
	int _nMaxStamps;
	
	
	int  _SignalAll;
	std::string _Filename;
	std::vector<PerfContext_t*> _CtxList;

};



ENGINE_API void StartPerfSampler(const char* Filename, int MaxStamps);

#ifdef _NO_PERF_
#define PERF_STAMP_TIME(Label_,Type_,Time_)
#define PERF_STAMP_TAG_DATA(Label_,Type_,Time_,_TagData1,_TagData2)
#define PERF_STAMP_FUL(Label_,Type_,File_,Func_,Line_)
#else
#define PERF_STAMP_TIME(Label_,Type_,Time_)\
{\
	PerfStamp_t* Stamp = PerfSampler::instance()->alloc();\
	if(Stamp){\
		Stamp->File = __FILE__;\
		Stamp->Func = __FUNCTION__;\
		Stamp->Line = __LINE__;\
		Stamp->Label = Label_;\
		Stamp->Type = Type_;\
		Stamp->Time = Time_;\
	}\
}
#define PERF_STAMP_TAG_DATA(Label_,Type_,Time_,_TagData1,_TagData2)\
{\
	PerfStamp_t* Stamp = PerfSampler::instance()->alloc();\
	if(Stamp){\
		Stamp->File = (const char*)_TagData1;\
		Stamp->Func = (const char*)_TagData2;\
		Stamp->Line = __LINE__;\
		Stamp->Label = Label_;\
		Stamp->Type = Type_;\
		Stamp->Time = Time_;\
		}\
}
#define PERF_STAMP_FUL(Label_,Type_,File_,Func_,Line_)\
{\
	PerfStamp_t* Stamp = PerfSampler::instance()->alloc();\
	if(Stamp){\
		Stamp->File = File_;\
		Stamp->Func = Func_;\
		Stamp->Line = Line_;\
		Stamp->Label = Label_;\
		Stamp->Type = Type_;\
		struct timeval kNowTimeval;\
		gettimeofday(&kNowTimeval, NULL);\
		Stamp->Time = kNowTimeval.tv_sec * 1000 + kNowTimeval.tv_usec / 1000;\
	}\
}
#endif

#define PERF_TYPE_MASK 0x00ff
#define PERF_TAG_MASK 0xff00

#define PERF_TYPE_BEGIN 1
#define PERF_TYPE_END 2
#define PERF_TYPE_TAG 3

#define PERF_TAG_DATA1 1
#define PERF_TAG_DATA2 2

#define PERF_FLAGS(TYPE,TAG) (((TAG)<<8)|(TYPE))

#define PERF_STAMP(Label,Type) PERF_STAMP_FUL(Label,Type,__FILE__,__FUNCTION__,__LINE__)
#define PERF_STAMP_DATA(Label,Type,Data1,Data2) PERF_STAMP_FUL(Label,Type,(const char*)Data1,(const char*)Data2,__LINE__)

#define PERF_TAG_DATA(Label,dt,Data1,Data2) if(PerfSampler::instance()->frameStarted()){ PERF_STAMP_TAG_TIME(#Label,PERF_FLAGS(PERF_TYPE_TAG,PERF_TAG_DATA1|PERF_TAG_DATA2),dt,Data1,Data2); }
#define PERF_TAG(Label,dt) if(PerfSampler::instance()->frameStarted()){ PERF_STAMP_TIME(#Label,PERF_FLAGS(PERF_TYPE_TAG,PERF_TAG_DATA1|PERF_TAG_DATA2),dt); }

#define PERF_BEGIN(Label) { PerfSampler*s=PerfSampler::instance();if(s->frameStarted()){ PERF_STAMP(#Label,PERF_TYPE_BEGIN);s->_pushLabel(#Label); }}
#define PERF_END(Label) { PerfSampler*s=PerfSampler::instance();if(s->frameStarted()){ PERF_STAMP(#Label,PERF_TYPE_END);s->_popLabel(#Label); }}
#define PERF_END_DATA(Label,Data1,Data2) { PerfSampler*s=PerfSampler::instance();if(s->frameStarted()){ PERF_STAMP_DATA(#Label,PERF_FLAGS(PERF_TYPE_END,PERF_TAG_DATA1|PERF_TAG_DATA2),Data1,Data2);s->_popLabel(#Label); }}

#define PERF_BEGIN_FRAME() { PerfSampler*s=PerfSampler::instance();s->startFrame();PERF_STAMP("Frame",PERF_TYPE_BEGIN);s->_pushLabel("Frame"); }
#define PERF_END_FRAME() { PerfSampler*s=PerfSampler::instance();if(s->frameStarted()){ PERF_STAMP("Frame",PERF_TYPE_END);s->_popLabel("Frame");s->endFrame();} }



#endif
