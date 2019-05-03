
#include "PerfStamp.h"
#include <map>
#include <string>
#include <assert.h>
#include <time.h>
#include <string.h>
#include "PathUtil.h"

PerfSampler PerfSampler::_Instance;



#if defined(_WINDOWS)
_declspec(thread) PerfContext_t* _PerfContex = nullptr;
#elif defined(ANDROID)
__thread PerfContext_t* _PerfContex = nullptr;
#elif defined(__APPLE__)
PerfContext_t* _PerfContex = nullptr;
#endif

ENGINE_API void StartPerfSampler(const char* Filename,int MaxStamps)
{
	PerfSampler::instance()->init(Filename,MaxStamps);
}

PerfSampler::PerfSampler()
{
	_nMaxStamps = 0;
	_SignalAll = -1;
}

void PerfSampler::_pushLabel(const char* text)
{
	PerfContext_t* Ctx = context();

	if (!Ctx)
		return ;

	if (Ctx->_HrkIndex < MAX_HRKS-1)
	{
		Ctx->_HrkIndex++;

		Ctx->_HrkLabels[Ctx->_HrkIndex] = text;
	}

}

void PerfSampler::_popLabel(const char* text)
{
	PerfContext_t* Ctx = context();

	if (!Ctx)
		return ;
	
	if (Ctx->_HrkIndex >= 0)
	{
		if (strcmp(Ctx->_HrkLabels[Ctx->_HrkIndex] ,text))
		{
			assert(0);
		}

		Ctx->_HrkIndex--;
	}
}
PerfContext_t* PerfSampler::context()
{
	if (_SignalAll == 0)
	{
		if (_PerfContex&&_PerfContex->Signal != -1)
		{
			_releaseContext(_PerfContex);		
		}
		return nullptr;
	}
	else if (_SignalAll == 1)
	{
		if (!_PerfContex)
		{
			_PerfContex = new PerfContext_t;
			_PerfContex->_PerfStampBuf = nullptr;
			_PerfContex->StampIndex = 0;
			_PerfContex->StampCount = 0;
			_PerfContex->_HrkIndex =  -1;
			_PerfContex->_nPerfStampBuf = _nMaxStamps;
			_PerfContex->Id = _CtxList.size();
			_PerfContex->_fpStream = nullptr;
			_PerfContex->Signal = 0;
			_PerfContex->_bFrameStart = false;
			_CtxList.push_back(_PerfContex);
		}

		if (_PerfContex->Signal == 0 || _PerfContex->Signal == -1)
		{
			_initStreamFile(_PerfContex);
			_writeHeader(_PerfContex);
			_initStampBuf(_PerfContex);
			_PerfContex->Signal = 1;
		}
	}
	else if (_SignalAll == 2)
	{
		if(_PerfContex)
			_PerfContex->Signal = 2;
	}

	return _PerfContex;
}



int PerfSampler::max_count()
{
	return _nMaxStamps;
}

int PerfSampler::index()
{
	PerfContext_t* ctx = context();
	if(ctx)
		return ctx->StampIndex;
	return -1;
}

PerfSampler::~PerfSampler()
{
	flush();
	close();
}

PerfSampler* PerfSampler::instance()
{
	return &_Instance;
}

PerfStamp_t* PerfSampler::alloc()
{
	PerfContext_t* Ctx = context();

	if (!Ctx)
		return nullptr;

	if (Ctx->Signal != 1)
		return nullptr;

	if (!Ctx->_fpStream)
	{
		return nullptr;
	}

	PerfStamp_t* Buf = Ctx->_PerfStampBuf;
	if (!Buf)
	{
		return nullptr;
	}

	int nBuf = Ctx->_nPerfStampBuf;
	int Index = Ctx->StampIndex;
	if (Buf && nBuf>0)
	{
		if (Index >= nBuf)
		{
			flushContext(Ctx);
		}

		PerfStamp_t* Stamp = Buf + Ctx->StampIndex;
		Ctx->StampIndex++;
		return Stamp;

	}
	return nullptr;
}

void PerfSampler::init(const char* Filename,int MaxStamps)
{
	_nMaxStamps = MaxStamps;
	_Filename = Filename;
}

void PerfSampler::_initStreamFile(PerfContext_t*Ctx)
{
	if (Ctx&&Ctx->_fpStream == nullptr)
	{
		time_t t;
		time(&t);
		const char* stm = ctime(&t);
		struct tm * tt = localtime(&t);
		char name[512] = "";
		sprintf_safe(name,512,"%s-(%d)-%d-%02d-%02d-%02d-%02d-%02d", _Filename.c_str(), Ctx->Id, 1900 + tt->tm_year, tt->tm_mon + 1, tt->tm_mday, tt->tm_hour, tt->tm_min, tt->tm_sec);
	
		Ctx->_fpStream = fopen(name, "wb");
	}
}

void PerfSampler::_initStampBuf(PerfContext_t*Ctx)
{
	if (Ctx&&!Ctx->_PerfStampBuf)
	{
		Ctx->_PerfStampBuf = new PerfStamp_t[_nMaxStamps];
		Ctx->_nPerfStampBuf = _nMaxStamps;
	}
}

void PerfSampler::resetContext(PerfContext_t*Ctx)
{
}

void PerfSampler::start()
{
	_SignalAll = 1;
}

void PerfSampler::pause()
{
	_SignalAll = 2;
}

void PerfSampler::resume()
{
	_SignalAll = 1;
}

int PerfSampler::isPaused()
{
	return _SignalAll != 1;
}

void PerfSampler::_writeHeader(PerfContext_t* Ctx)
{
	if (!Ctx)
		return;
	long long StampCount = Ctx->StampCount;

	if (Ctx->_fpStream)
	{
		long fl = ftell(Ctx->_fpStream);
		PerfHeader_t hdr;
		hdr.count = 0;
		hdr.magic = PERF_STAMP_FILE_ID;
		hdr.ver = PERF_STAMP_VERSION;
		int ret = fseek(Ctx->_fpStream, 0, SEEK_SET);
		int hdr_offset = sizeof(PerfHeader_t);

		if(fl == 0)
		{
			hdr.count = 0;
			fl += hdr_offset;
		}
		else
		{
			hdr.count = StampCount;
		}
		fwrite(&hdr, sizeof(PerfHeader_t), 1, Ctx->_fpStream);
		fflush(Ctx->_fpStream);
		fseek(Ctx->_fpStream,fl,SEEK_SET);
	}
}

void PerfSampler::flushContext(PerfContext_t*Ctx)
{
	if (Ctx&&Ctx->_fpStream )
	{
		for (int i = 0; i<Ctx->StampIndex; i++)
		{
			unsigned short Type = PERF_TYPE_MASK&Ctx->_PerfStampBuf[i].Type;
			unsigned short Tag = PERF_TAG_MASK&Ctx->_PerfStampBuf[i].Type;
			const char* s = nullptr;
			if ( Tag == 0)
			{
				s = Ctx->_PerfStampBuf[i].File;
				Ctx->_strMaps[s] = s;

			    s = Ctx->_PerfStampBuf[i].Func;
				if (s)
				{
					Ctx->_strMaps[s] = s;
				}			
			}

			s = Ctx->_PerfStampBuf[i].Label;
			if (s)
			{
				assert(strlen(s) > 0);
				Ctx->_strMaps[s] = s;
			}
		}
		fwrite(Ctx->_PerfStampBuf, sizeof(PerfStamp_t), Ctx->StampIndex, Ctx->_fpStream);
		fflush(Ctx->_fpStream);
		Ctx->StampCount += Ctx->StampIndex;
		Ctx->StampIndex = 0;
		_writeHeader(Ctx);
	}
}

void PerfSampler::flush()
{
	PerfContext_t * Ctx = context();
	flushContext(Ctx);
}

void PerfSampler::_writeStringTable(PerfContext_t* Ctx)
{
	if (!Ctx)
		return;
	if (!Ctx->_fpStream)
		return;

	int cStrings = Ctx->_strMaps.size();

	long fl = ftell(Ctx->_fpStream);
	int c = fwrite(&cStrings, sizeof(cStrings), 1, Ctx->_fpStream);
	auto kItor = Ctx->_strMaps.begin();
	while (kItor != Ctx->_strMaps.end())
	{
		unsigned int addr = reinterpret_cast<unsigned long>(kItor->first);
		const char* str = kItor->second.c_str();
		assert(strlen(str)>0);
		int len = strlen(str)+1;
		fwrite(&addr, sizeof(addr), 1, Ctx->_fpStream);
		fwrite(&len, sizeof(len), 1, Ctx->_fpStream);
		fwrite(str, len, 1, Ctx->_fpStream);
		kItor++;
	}
}

void PerfSampler::startFrame()
{
	PerfContext_t * Ctx = context();
	if(Ctx)
		Ctx->_bFrameStart = true;
}

void PerfSampler::endFrame()
{
	PerfContext_t * Ctx = context();
	if (Ctx)
		Ctx->_bFrameStart = false;
}

bool PerfSampler::frameStarted()
{
	PerfContext_t * Ctx = context();
	if(Ctx)
		return Ctx->_bFrameStart;
	return false;
}

void PerfSampler::close()
{
	_SignalAll = 0;
}

void PerfSampler::_releaseContext(PerfContext_t* Ctx)
{
	if (Ctx)
	{
		if (Ctx->_fpStream)
		{
			flushContext(Ctx);
			_writeHeader(Ctx);
			_writeStringTable(Ctx);
			fclose(Ctx->_fpStream);
			Ctx->_fpStream = nullptr;
		}

		if (Ctx->_PerfStampBuf)
		{
			delete[] Ctx->_PerfStampBuf;
		}
		Ctx->StampCount = 0;
		Ctx->_PerfStampBuf = nullptr;
		Ctx->_nPerfStampBuf = 0;
		Ctx->Signal = -1;
	}
}