#ifndef __DICHHDEFS
#define __DICHHDEFS
#include <assert.h>
#include <stdlib.h>
#include <string.h>
#ifndef WIN32
#include <unistd.h>
#endif
#ifdef __VMS
#include <starlet.h>
#endif
#include "dim_core.hxx"
#include "dim.hxx"
#include "tokenstring.hxx"

#if __cplusplus <= 201103L
#define DIMXX_OVERRIDE
#else
#define DIMXX_OVERRIDE override
#endif

enum DimServiceType {DimSERVICE=1, DimCOMMAND, DimRPC};

class DimClient;
class DimInfo;
class DimCurrentInfo;
class DimRpcInfo;

class DllExp DimInfoHandler{
public:
	DimInfo *itsService;
    DimInfo *getInfo() { return itsService; }; 
	virtual void infoHandler() = 0;
	virtual ~DimInfoHandler() {};
};

class DllExp DimInfo : public DimInfoHandler, public DimTimer{

public :
	DimInfoHandler *itsHandler;

	DimInfo()
		{ subscribe((char *)0, 0, (void *)0, 0, 0, 0); };
	DimInfo(const char *name, int nolink) 
		{ subscribe((char *)name, 0, &nolink, sizeof(int), 0, 0); };
	DimInfo(const char *name, int time, int nolink) 
		{ subscribe((char *)name, time, &nolink, sizeof(int), 0, 0); };
	DimInfo(const char *name, float nolink) 
		{ subscribe((char *)name, 0, &nolink, sizeof(float), 0, 0); };
	DimInfo(const char *name, int time, float nolink) 
		{ subscribe((char *)name, time, &nolink, sizeof(float), 0, 0); };
	DimInfo(const char *name, double nolink) 
		{ subscribe((char *)name, 0, &nolink, sizeof(double), 0, 0); };
	DimInfo(const char *name, int time, double nolink) 
		{ subscribe((char *)name, time, &nolink, sizeof(double), 0, 0); };
	DimInfo(const char *name, longlong nolink) 
		{ subscribe((char *)name, 0, &nolink, sizeof(longlong), 0, 0); };
	DimInfo(const char *name, int time, longlong nolink) 
		{ subscribe((char *)name, time, &nolink, sizeof(longlong), 0, 0); };
	DimInfo(const char *name, short nolink) 
		{ subscribe((char *)name, 0, &nolink, sizeof(short), 0, 0); };
	DimInfo(const char *name, int time, short nolink) 
		{ subscribe((char *)name, time, &nolink, sizeof(short), 0, 0); };
	DimInfo(const char *name, char *nolink) 
		{ subscribe((char *)name, 0, nolink, (int)strlen(nolink)+1, 0, 0); };
	DimInfo(const char *name, int time, char *nolink) 
		{ subscribe((char *)name, time, nolink, (int)strlen(nolink)+1, 0, 0); };
	DimInfo(const char *name, void *nolink, int nolinksize) 
		{ subscribe((char *)name, 0, nolink, nolinksize, 0, 0); };
	DimInfo(const char *name, int time, void *nolink, int nolinksize) 
		{ subscribe((char *)name, time, nolink, nolinksize, 0, 0); };

	DimInfo(const char *name, int nolink, DimInfoHandler *handler) 
		{ subscribe((char *)name, 0, &nolink, sizeof(int), handler, 0); };
	DimInfo(const char *name, int time, int nolink, DimInfoHandler *handler) 
		{ subscribe((char *)name, time, &nolink, sizeof(int), handler, 0); };
	DimInfo(const char *name, float nolink, DimInfoHandler *handler) 
		{ subscribe((char *)name, 0, &nolink, sizeof(float), handler, 0); };
	DimInfo(const char *name, int time, float nolink, DimInfoHandler *handler) 
		{ subscribe((char *)name, time, &nolink, sizeof(float), handler, 0); };
	DimInfo(const char *name, double nolink, DimInfoHandler *handler) 
		{ subscribe((char *)name, 0, &nolink, sizeof(double), handler, 0); };
	DimInfo(const char *name, int time, double nolink, DimInfoHandler *handler) 
		{ subscribe((char *)name, time, &nolink, sizeof(double), handler, 0); };
	DimInfo(const char *name, longlong nolink, DimInfoHandler *handler) 
		{ subscribe((char *)name, 0, &nolink, sizeof(longlong), handler, 0); };
	DimInfo(const char *name, int time, longlong nolink, DimInfoHandler *handler) 
		{ subscribe((char *)name, time, &nolink, sizeof(longlong), handler, 0); };
	DimInfo(const char *name, short nolink, DimInfoHandler *handler)
		{ subscribe((char *)name, 0, &nolink, sizeof(short), handler, 0); };
	DimInfo(const char *name, int time, short nolink, DimInfoHandler *handler) 
		{ subscribe((char *)name, time, &nolink, sizeof(short), handler, 0); };
	DimInfo(const char *name, char *nolink, DimInfoHandler *handler) 
		{ subscribe((char *)name, 0, nolink, (int)strlen(nolink)+1, handler, 0); };
	DimInfo(const char *name, int time, char *nolink, DimInfoHandler *handler) 
		{ subscribe((char *)name, time, nolink, (int)strlen(nolink)+1, handler, 0); };
	DimInfo(const char *name, void *nolink, int nolinksize, DimInfoHandler *handler) 
		{ subscribe((char *)name, 0, nolink, nolinksize, handler, 0); };
	DimInfo(const char *name, int time, void *nolink, int nolinksize, DimInfoHandler *handler) 
		{ subscribe((char *)name, time, nolink, nolinksize, handler, 0); };

	DimInfo(dim_long dnsid, const char *name, int nolink)
	{
		subscribe((char *)name, 0, &nolink, sizeof(int), 0, dnsid);
	};
	DimInfo(dim_long dnsid, const char *name, int time, int nolink)
	{
		subscribe((char *)name, time, &nolink, sizeof(int), 0, dnsid);
	};
	DimInfo(dim_long dnsid, const char *name, float nolink)
	{
		subscribe((char *)name, 0, &nolink, sizeof(float), 0, dnsid);
	};
	DimInfo(dim_long dnsid, const char *name, int time, float nolink)
	{
		subscribe((char *)name, time, &nolink, sizeof(float), 0, dnsid);
	};
	DimInfo(dim_long dnsid, const char *name, double nolink)
	{
		subscribe((char *)name, 0, &nolink, sizeof(double), 0, dnsid);
	};
	DimInfo(dim_long dnsid, const char *name, int time, double nolink)
	{
		subscribe((char *)name, time, &nolink, sizeof(double), 0, dnsid);
	};
	DimInfo(dim_long dnsid, const char *name, longlong nolink)
	{
		subscribe((char *)name, 0, &nolink, sizeof(longlong), 0, dnsid);
	};
	DimInfo(dim_long dnsid, const char *name, int time, longlong nolink)
	{
		subscribe((char *)name, time, &nolink, sizeof(longlong), 0, dnsid);
	};
	DimInfo(dim_long dnsid, const char *name, short nolink)
	{
		subscribe((char *)name, 0, &nolink, sizeof(short), 0, dnsid);
	};
	DimInfo(dim_long dnsid, const char *name, int time, short nolink)
	{
		subscribe((char *)name, time, &nolink, sizeof(short), 0, dnsid);
	};
	DimInfo(dim_long dnsid, const char *name, char *nolink)
	{
		subscribe((char *)name, 0, nolink, (int)strlen(nolink) + 1, 0, dnsid);
	};
	DimInfo(dim_long dnsid, const char *name, int time, char *nolink)
	{
		subscribe((char *)name, time, nolink, (int)strlen(nolink) + 1, 0, dnsid);
	};
	DimInfo(dim_long dnsid, const char *name, void *nolink, int nolinksize)
	{
		subscribe((char *)name, 0, nolink, nolinksize, 0, dnsid);
	};
	DimInfo(dim_long dnsid, const char *name, int time, void *nolink, int nolinksize)
	{
		subscribe((char *)name, time, nolink, nolinksize, 0, dnsid);
	};

	DimInfo(dim_long dnsid, const char *name, int nolink, DimInfoHandler *handler)
	{
		subscribe((char *)name, 0, &nolink, sizeof(int), handler, dnsid);
	};
	DimInfo(dim_long dnsid, const char *name, int time, int nolink, DimInfoHandler *handler)
	{
		subscribe((char *)name, time, &nolink, sizeof(int), handler, dnsid);
	};
	DimInfo(dim_long dnsid, const char *name, float nolink, DimInfoHandler *handler)
	{
		subscribe((char *)name, 0, &nolink, sizeof(float), handler, dnsid);
	};
	DimInfo(dim_long dnsid, const char *name, int time, float nolink, DimInfoHandler *handler)
	{
		subscribe((char *)name, time, &nolink, sizeof(float), handler, dnsid);
	};
	DimInfo(dim_long dnsid, const char *name, double nolink, DimInfoHandler *handler)
	{
		subscribe((char *)name, 0, &nolink, sizeof(double), handler, dnsid);
	};
	DimInfo(dim_long dnsid, const char *name, int time, double nolink, DimInfoHandler *handler)
	{
		subscribe((char *)name, time, &nolink, sizeof(double), handler, dnsid);
	};
	DimInfo(dim_long dnsid, const char *name, longlong nolink, DimInfoHandler *handler)
	{
		subscribe((char *)name, 0, &nolink, sizeof(longlong), handler, dnsid);
	};
	DimInfo(dim_long dnsid, const char *name, int time, longlong nolink, DimInfoHandler *handler)
	{
		subscribe((char *)name, time, &nolink, sizeof(longlong), handler, dnsid);
	};
	DimInfo(dim_long dnsid, const char *name, short nolink, DimInfoHandler *handler)
	{
		subscribe((char *)name, 0, &nolink, sizeof(short), handler, dnsid);
	};
	DimInfo(dim_long dnsid, const char *name, int time, short nolink, DimInfoHandler *handler)
	{
		subscribe((char *)name, time, &nolink, sizeof(short), handler, dnsid);
	};
	DimInfo(dim_long dnsid, const char *name, char *nolink, DimInfoHandler *handler)
	{
		subscribe((char *)name, 0, nolink, (int)strlen(nolink) + 1, handler, dnsid);
	};
	DimInfo(dim_long dnsid, const char *name, int time, char *nolink, DimInfoHandler *handler)
	{
		subscribe((char *)name, time, nolink, (int)strlen(nolink) + 1, handler, dnsid);
	};
	DimInfo(dim_long dnsid, const char *name, void *nolink, int nolinksize, DimInfoHandler *handler)
	{
		subscribe((char *)name, 0, nolink, nolinksize, handler, dnsid);
	};
	DimInfo(dim_long dnsid, const char *name, int time, void *nolink, int nolinksize, DimInfoHandler *handler)
	{
		subscribe((char *)name, time, nolink, nolinksize, handler, dnsid);
	};

	virtual ~DimInfo();
	void *itsData;
	int itsDataSize;
	int itsSize;
	int getSize() {return itsSize; };
	char *getName()  { return itsName; } ;
	void *getData();
	int getInt() { return *(int *)getData(); } ;
	float getFloat() { return *(float *)getData(); } ;
	double getDouble() { return *(double *)getData(); } ;
	longlong getLonglong() { return *(longlong *)getData(); } ;
	short getShort() { return *(short *)getData(); } ;
	char *getString()  { return (char *)getData(); } ;

	virtual void infoHandler() DIMXX_OVERRIDE;
	void timerHandler() DIMXX_OVERRIDE;
	virtual void subscribe(char *name, int time, void *nolink, int nolinksize,
		DimInfoHandler *handler, dim_long dnsid);
	virtual void doIt();
	int getQuality();
	int getTimestamp();
	int getTimestampMillisecs();
	char *getFormat();
	void subscribe(char *name, void *nolink, int nolinksize, int time, 
		DimInfoHandler *handler, dim_long dnsid) 
		{ subscribe((char *)name, time, nolink, nolinksize, handler, dnsid); };

protected :
	char *itsName;
	int itsId;
	int itsTime;
	int itsType;
//	int itsTagId;
	char *itsFormat;
	void *itsNolinkBuf;
	int itsNolinkSize;
	int secs, millisecs;
	dim_long itsDnsId;
};

class DllExp DimStampedInfo : public DimInfo{

public :
	DimStampedInfo(){};
	DimStampedInfo(const char *name, int nolink) 
	{ subscribe((char *)name, 0, &nolink, sizeof(int), 0, 0); };
	DimStampedInfo(const char *name, int time, int nolink) 
	{ subscribe((char *)name, time, &nolink, sizeof(int), 0, 0); };
	DimStampedInfo(const char *name, float nolink) 
	{ subscribe((char *)name, 0, &nolink, sizeof(float), 0, 0); };
	DimStampedInfo(const char *name, int time, float nolink) 
	{ subscribe((char *)name, time, &nolink, sizeof(float), 0, 0); };
	DimStampedInfo(const char *name, double nolink) 
	{ subscribe((char *)name, 0, &nolink, sizeof(double), 0, 0); };
	DimStampedInfo(const char *name, int time, double nolink) 
	{ subscribe((char *)name, time, &nolink, sizeof(double), 0, 0); };
	DimStampedInfo(const char *name, longlong nolink) 
	{ subscribe((char *)name, 0, &nolink, sizeof(longlong), 0, 0); };
	DimStampedInfo(const char *name, int time, longlong nolink) 
	{ subscribe((char *)name, time, &nolink, sizeof(longlong), 0, 0); };
	DimStampedInfo(const char *name, short nolink) 
	{ subscribe((char *)name, 0, &nolink, sizeof(short), 0, 0); };
	DimStampedInfo(const char *name, int time, short nolink) 
	{ subscribe((char *)name, time, &nolink, sizeof(short), 0, 0); };
	DimStampedInfo(const char *name, char *nolink) 
	{ subscribe((char *)name, 0, nolink, (int)strlen(nolink)+1, 0, 0); };
	DimStampedInfo(const char *name, int time, char *nolink) 
	{ subscribe((char *)name, time, nolink, (int)strlen(nolink)+1, 0, 0); };
	DimStampedInfo(const char *name, void *nolink, int nolinksize) 
	{ subscribe((char *)name, 0, nolink, nolinksize, 0, 0); };
	DimStampedInfo(const char *name, int time, void *nolink, int nolinksize) 
	{ subscribe((char *)name, time, nolink, nolinksize, 0, 0); };

	DimStampedInfo(const char *name, int nolink, DimInfoHandler *handler) 
	{ subscribe((char *)name, 0, &nolink, sizeof(int), handler, 0); };
	DimStampedInfo(const char *name, int time, int nolink, DimInfoHandler *handler) 
	{ subscribe((char *)name, time, &nolink, sizeof(int), handler, 0); };
	DimStampedInfo(const char *name, float nolink, DimInfoHandler *handler) 
	{ subscribe((char *)name, 0, &nolink, sizeof(float), handler, 0); };
	DimStampedInfo(const char *name, int time, float nolink, DimInfoHandler *handler) 
	{ subscribe((char *)name, time, &nolink, sizeof(float), handler, 0); };
	DimStampedInfo(const char *name, double nolink, DimInfoHandler *handler) 
	{ subscribe((char *)name, 0, &nolink, sizeof(double), handler, 0); };
	DimStampedInfo(const char *name, int time, double nolink, DimInfoHandler *handler) 
	{ subscribe((char *)name, time, &nolink, sizeof(double), handler, 0); };
	DimStampedInfo(const char *name, longlong nolink, DimInfoHandler *handler) 
	{ subscribe((char *)name, 0, &nolink, sizeof(longlong), handler, 0); };
	DimStampedInfo(const char *name, int time, longlong nolink, DimInfoHandler *handler) 
	{ subscribe((char *)name, time, &nolink, sizeof(longlong), handler, 0); };
	DimStampedInfo(const char *name, short nolink, DimInfoHandler *handler) 
	{ subscribe((char *)name, 0, &nolink, sizeof(short), handler, 0); };
	DimStampedInfo(const char *name, int time, short nolink, DimInfoHandler *handler) 
	{ subscribe((char *)name, time, &nolink, sizeof(short), handler, 0); };
		DimStampedInfo(const char *name, char *nolink, DimInfoHandler *handler) 
	{ subscribe((char *)name, 0, nolink, (int)strlen(nolink)+1, handler, 0); };
	DimStampedInfo(const char *name, int time, char *nolink, DimInfoHandler *handler) 
	{ subscribe((char *)name, time, nolink, (int)strlen(nolink)+1, handler, 0); };
	DimStampedInfo(const char *name, void *nolink, int nolinksize, DimInfoHandler *handler) 
	{ subscribe((char *)name, 0, nolink, nolinksize, handler, 0); };
	DimStampedInfo(const char *name, int time, void *nolink, int nolinksize, DimInfoHandler *handler) 
	{ subscribe((char *)name, time, nolink, nolinksize, handler, 0); };

	DimStampedInfo(dim_long dnsid, const char *name, int nolink)
	{
		subscribe((char *)name, 0, &nolink, sizeof(int), 0, dnsid);
	};
	DimStampedInfo(dim_long dnsid, const char *name, int time, int nolink)
	{
		subscribe((char *)name, time, &nolink, sizeof(int), 0, dnsid);
	};
	DimStampedInfo(dim_long dnsid, const char *name, float nolink)
	{
		subscribe((char *)name, 0, &nolink, sizeof(float), 0, dnsid);
	};
	DimStampedInfo(dim_long dnsid, const char *name, int time, float nolink)
	{
		subscribe((char *)name, time, &nolink, sizeof(float), 0, dnsid);
	};
	DimStampedInfo(dim_long dnsid, const char *name, double nolink)
	{
		subscribe((char *)name, 0, &nolink, sizeof(double), 0, dnsid);
	};
	DimStampedInfo(dim_long dnsid, const char *name, int time, double nolink)
	{
		subscribe((char *)name, time, &nolink, sizeof(double), 0, dnsid);
	};
	DimStampedInfo(dim_long dnsid, const char *name, longlong nolink)
	{
		subscribe((char *)name, 0, &nolink, sizeof(longlong), 0, dnsid);
	};
	DimStampedInfo(dim_long dnsid, const char *name, int time, longlong nolink)
	{
		subscribe((char *)name, time, &nolink, sizeof(longlong), 0, dnsid);
	};
	DimStampedInfo(dim_long dnsid, const char *name, short nolink)
	{
		subscribe((char *)name, 0, &nolink, sizeof(short), 0, dnsid);
	};
	DimStampedInfo(dim_long dnsid, const char *name, int time, short nolink)
	{
		subscribe((char *)name, time, &nolink, sizeof(short), 0, dnsid);
	};
	DimStampedInfo(dim_long dnsid, const char *name, char *nolink)
	{
		subscribe((char *)name, 0, nolink, (int)strlen(nolink) + 1, 0, dnsid);
	};
	DimStampedInfo(dim_long dnsid, const char *name, int time, char *nolink)
	{
		subscribe((char *)name, time, nolink, (int)strlen(nolink) + 1, 0, dnsid);
	};
	DimStampedInfo(dim_long dnsid, const char *name, void *nolink, int nolinksize)
	{
		subscribe((char *)name, 0, nolink, nolinksize, 0, dnsid);
	};
	DimStampedInfo(dim_long dnsid, const char *name, int time, void *nolink, int nolinksize)
	{
		subscribe((char *)name, time, nolink, nolinksize, 0, dnsid);
	};

	DimStampedInfo(dim_long dnsid, const char *name, int nolink, DimInfoHandler *handler)
	{
		subscribe((char *)name, 0, &nolink, sizeof(int), handler, dnsid);
	};
	DimStampedInfo(dim_long dnsid, const char *name, int time, int nolink, DimInfoHandler *handler)
	{
		subscribe((char *)name, time, &nolink, sizeof(int), handler, dnsid);
	};
	DimStampedInfo(dim_long dnsid, const char *name, float nolink, DimInfoHandler *handler)
	{
		subscribe((char *)name, 0, &nolink, sizeof(float), handler, dnsid);
	};
	DimStampedInfo(dim_long dnsid, const char *name, int time, float nolink, DimInfoHandler *handler)
	{
		subscribe((char *)name, time, &nolink, sizeof(float), handler, dnsid);
	};
	DimStampedInfo(dim_long dnsid, const char *name, double nolink, DimInfoHandler *handler)
	{
		subscribe((char *)name, 0, &nolink, sizeof(double), handler, dnsid);
	};
	DimStampedInfo(dim_long dnsid, const char *name, int time, double nolink, DimInfoHandler *handler)
	{
		subscribe((char *)name, time, &nolink, sizeof(double), handler, dnsid);
	};
	DimStampedInfo(dim_long dnsid, const char *name, longlong nolink, DimInfoHandler *handler)
	{
		subscribe((char *)name, 0, &nolink, sizeof(longlong), handler, dnsid);
	};
	DimStampedInfo(dim_long dnsid, const char *name, int time, longlong nolink, DimInfoHandler *handler)
	{
		subscribe((char *)name, time, &nolink, sizeof(longlong), handler, dnsid);
	};
	DimStampedInfo(dim_long dnsid, const char *name, short nolink, DimInfoHandler *handler)
	{
		subscribe((char *)name, 0, &nolink, sizeof(short), handler, dnsid);
	};
	DimStampedInfo(dim_long dnsid, const char *name, int time, short nolink, DimInfoHandler *handler)
	{
		subscribe((char *)name, time, &nolink, sizeof(short), handler, dnsid);
	};
	DimStampedInfo(dim_long dnsid, const char *name, char *nolink, DimInfoHandler *handler)
	{
		subscribe((char *)name, 0, nolink, (int)strlen(nolink) + 1, handler, dnsid);
	};
	DimStampedInfo(dim_long dnsid, const char *name, int time, char *nolink, DimInfoHandler *handler)
	{
		subscribe((char *)name, time, nolink, (int)strlen(nolink) + 1, handler, dnsid);
	};
	DimStampedInfo(dim_long dnsid, const char *name, void *nolink, int nolinksize, DimInfoHandler *handler)
	{
		subscribe((char *)name, 0, nolink, nolinksize, handler, dnsid);
	};
	DimStampedInfo(dim_long dnsid, const char *name, int time, void *nolink, int nolinksize, DimInfoHandler *handler)
	{
		subscribe((char *)name, time, nolink, nolinksize, handler, dnsid);
	};
	virtual ~DimStampedInfo();
	void subscribe(char *name, void *nolink, int nolinksize, int time, 
		DimInfoHandler *handler, dim_long dnsid)
		{ subscribe((char *)name, time, nolink, nolinksize, handler, dnsid); };
private :
	void doIt() DIMXX_OVERRIDE;
	void subscribe(char *name, int time, void *nolink, int nolinksize,
		DimInfoHandler *handler, dim_long dnsid) DIMXX_OVERRIDE;
};

class DllExp DimUpdatedInfo : public DimInfo{

public :
	DimUpdatedInfo(){};
	DimUpdatedInfo(const char *name, int nolink) 
	{ subscribe((char *)name, 0, &nolink, sizeof(int), 0, 0); };
	DimUpdatedInfo(const char *name, int time, int nolink) 
	{ subscribe((char *)name, time, &nolink, sizeof(int), 0, 0); };
	DimUpdatedInfo(const char *name, float nolink) 
	{ subscribe((char *)name, 0, &nolink, sizeof(float), 0, 0); };
	DimUpdatedInfo(const char *name, int time, float nolink) 
	{ subscribe((char *)name, time, &nolink, sizeof(float), 0, 0); };
	DimUpdatedInfo(const char *name, double nolink) 
	{ subscribe((char *)name, 0, &nolink, sizeof(double), 0, 0); };
	DimUpdatedInfo(const char *name, int time, double nolink) 
	{ subscribe((char *)name, time, &nolink, sizeof(double), 0, 0); };
	DimUpdatedInfo(const char *name, longlong nolink) 
	{ subscribe((char *)name, 0, &nolink, sizeof(longlong), 0, 0); };
	DimUpdatedInfo(const char *name, int time, longlong nolink) 
	{ subscribe((char *)name, time, &nolink, sizeof(longlong), 0, 0); };
	DimUpdatedInfo(const char *name, short nolink) 
	{ subscribe((char *)name, 0, &nolink, sizeof(short), 0, 0); };
	DimUpdatedInfo(const char *name, int time, short nolink) 
	{ subscribe((char *)name, time, &nolink, sizeof(short), 0, 0); };
	DimUpdatedInfo(const char *name, char *nolink) 
	{ subscribe((char *)name, 0, nolink, (int)strlen(nolink)+1, 0, 0); };
	DimUpdatedInfo(const char *name, int time, char *nolink) 
	{ subscribe((char *)name, time, nolink, (int)strlen(nolink)+1, 0, 0); };
	DimUpdatedInfo(const char *name, void *nolink, int nolinksize) 
	{ subscribe((char *)name, 0, nolink, nolinksize, 0, 0); };
	DimUpdatedInfo(const char *name, int time, void *nolink, int nolinksize) 
	{ subscribe((char *)name, time, nolink, nolinksize, 0, 0); };

	DimUpdatedInfo(const char *name, int nolink, DimInfoHandler *handler) 
	{ subscribe((char *)name, 0, &nolink, sizeof(int), handler, 0); };
	DimUpdatedInfo(const char *name, int time, int nolink, DimInfoHandler *handler) 
	{ subscribe((char *)name, time, &nolink, sizeof(int), handler, 0); };
	DimUpdatedInfo(const char *name, float nolink, DimInfoHandler *handler) 
	{ subscribe((char *)name, 0, &nolink, sizeof(float), handler, 0); };
	DimUpdatedInfo(const char *name, int time, float nolink, DimInfoHandler *handler) 
	{ subscribe((char *)name, time, &nolink, sizeof(float), handler, 0); };
	DimUpdatedInfo(const char *name, double nolink, DimInfoHandler *handler) 
	{ subscribe((char *)name, 0, &nolink, sizeof(double), handler, 0); };
	DimUpdatedInfo(const char *name, int time, double nolink, DimInfoHandler *handler) 
	{ subscribe((char *)name, time, &nolink, sizeof(double), handler, 0); };
	DimUpdatedInfo(const char *name, longlong nolink, DimInfoHandler *handler) 
	{ subscribe((char *)name, 0, &nolink, sizeof(longlong), handler, 0); };
	DimUpdatedInfo(const char *name, int time, longlong nolink, DimInfoHandler *handler) 
	{ subscribe((char *)name, time, &nolink, sizeof(longlong), handler, 0); };
	DimUpdatedInfo(const char *name, short nolink, DimInfoHandler *handler) 
	{ subscribe((char *)name, 0, &nolink, sizeof(short), handler, 0); };
	DimUpdatedInfo(const char *name, int time, short nolink, DimInfoHandler *handler) 
	{ subscribe((char *)name, time, &nolink, sizeof(short), handler, 0); };
	DimUpdatedInfo(const char *name, char *nolink, DimInfoHandler *handler) 
	{ subscribe((char *)name, 0, nolink, (int)strlen(nolink)+1, handler, 0); };
	DimUpdatedInfo(const char *name, int time, char *nolink, DimInfoHandler *handler) 
	{ subscribe((char *)name, time, nolink, (int)strlen(nolink)+1, handler, 0); };
	DimUpdatedInfo(const char *name, void *nolink, int nolinksize, DimInfoHandler *handler) 
	{ subscribe((char *)name, 0, nolink, nolinksize, handler, 0); };
	DimUpdatedInfo(const char *name, int time, void *nolink, int nolinksize, DimInfoHandler *handler) 
	{ subscribe((char *)name, time, nolink, nolinksize, handler, 0); };

	DimUpdatedInfo(dim_long dnsid, const char *name, int nolink)
	{
		subscribe((char *)name, 0, &nolink, sizeof(int), 0, dnsid);
	};
	DimUpdatedInfo(dim_long dnsid, const char *name, int time, int nolink)
	{
		subscribe((char *)name, time, &nolink, sizeof(int), 0, dnsid);
	};
	DimUpdatedInfo(dim_long dnsid, const char *name, float nolink)
	{
		subscribe((char *)name, 0, &nolink, sizeof(float), 0, dnsid);
	};
	DimUpdatedInfo(dim_long dnsid, const char *name, int time, float nolink)
	{
		subscribe((char *)name, time, &nolink, sizeof(float), 0, dnsid);
	};
	DimUpdatedInfo(dim_long dnsid, const char *name, double nolink)
	{
		subscribe((char *)name, 0, &nolink, sizeof(double), 0, dnsid);
	};
	DimUpdatedInfo(dim_long dnsid, const char *name, int time, double nolink)
	{
		subscribe((char *)name, time, &nolink, sizeof(double), 0, dnsid);
	};
	DimUpdatedInfo(dim_long dnsid, const char *name, longlong nolink)
	{
		subscribe((char *)name, 0, &nolink, sizeof(longlong), 0, dnsid);
	};
	DimUpdatedInfo(dim_long dnsid, const char *name, int time, longlong nolink)
	{
		subscribe((char *)name, time, &nolink, sizeof(longlong), 0, dnsid);
	};
	DimUpdatedInfo(dim_long dnsid, const char *name, short nolink)
	{
		subscribe((char *)name, 0, &nolink, sizeof(short), 0, dnsid);
	};
	DimUpdatedInfo(dim_long dnsid, const char *name, int time, short nolink)
	{
		subscribe((char *)name, time, &nolink, sizeof(short), 0, dnsid);
	};
	DimUpdatedInfo(dim_long dnsid, const char *name, char *nolink)
	{
		subscribe((char *)name, 0, nolink, (int)strlen(nolink) + 1, 0, dnsid);
	};
	DimUpdatedInfo(dim_long dnsid, const char *name, int time, char *nolink)
	{
		subscribe((char *)name, time, nolink, (int)strlen(nolink) + 1, 0, dnsid);
	};
	DimUpdatedInfo(dim_long dnsid, const char *name, void *nolink, int nolinksize)
	{
		subscribe((char *)name, 0, nolink, nolinksize, 0, dnsid);
	};
	DimUpdatedInfo(dim_long dnsid, const char *name, int time, void *nolink, int nolinksize)
	{
		subscribe((char *)name, time, nolink, nolinksize, 0, dnsid);
	};

	DimUpdatedInfo(dim_long dnsid, const char *name, int nolink, DimInfoHandler *handler)
	{
		subscribe((char *)name, 0, &nolink, sizeof(int), handler, dnsid);
	};
	DimUpdatedInfo(dim_long dnsid, const char *name, int time, int nolink, DimInfoHandler *handler)
	{
		subscribe((char *)name, time, &nolink, sizeof(int), handler, dnsid);
	};
	DimUpdatedInfo(dim_long dnsid, const char *name, float nolink, DimInfoHandler *handler)
	{
		subscribe((char *)name, 0, &nolink, sizeof(float), handler, dnsid);
	};
	DimUpdatedInfo(dim_long dnsid, const char *name, int time, float nolink, DimInfoHandler *handler)
	{
		subscribe((char *)name, time, &nolink, sizeof(float), handler, dnsid);
	};
	DimUpdatedInfo(dim_long dnsid, const char *name, double nolink, DimInfoHandler *handler)
	{
		subscribe((char *)name, 0, &nolink, sizeof(double), handler, dnsid);
	};
	DimUpdatedInfo(dim_long dnsid, const char *name, int time, double nolink, DimInfoHandler *handler)
	{
		subscribe((char *)name, time, &nolink, sizeof(double), handler, dnsid);
	};
	DimUpdatedInfo(dim_long dnsid, const char *name, longlong nolink, DimInfoHandler *handler)
	{
		subscribe((char *)name, 0, &nolink, sizeof(longlong), handler, dnsid);
	};
	DimUpdatedInfo(dim_long dnsid, const char *name, int time, longlong nolink, DimInfoHandler *handler)
	{
		subscribe((char *)name, time, &nolink, sizeof(longlong), handler, dnsid);
	};
	DimUpdatedInfo(dim_long dnsid, const char *name, short nolink, DimInfoHandler *handler)
	{
		subscribe((char *)name, 0, &nolink, sizeof(short), handler, dnsid);
	};
	DimUpdatedInfo(dim_long dnsid, const char *name, int time, short nolink, DimInfoHandler *handler)
	{
		subscribe((char *)name, time, &nolink, sizeof(short), handler, dnsid);
	};
	DimUpdatedInfo(dim_long dnsid, const char *name, char *nolink, DimInfoHandler *handler)
	{
		subscribe((char *)name, 0, nolink, (int)strlen(nolink) + 1, handler, dnsid);
	};
	DimUpdatedInfo(dim_long dnsid, const char *name, int time, char *nolink, DimInfoHandler *handler)
	{
		subscribe((char *)name, time, nolink, (int)strlen(nolink) + 1, handler, dnsid);
	};
	DimUpdatedInfo(dim_long dnsid, const char *name, void *nolink, int nolinksize, DimInfoHandler *handler)
	{
		subscribe((char *)name, 0, nolink, nolinksize, handler, dnsid);
	};
	DimUpdatedInfo(dim_long dnsid, const char *name, int time, void *nolink, int nolinksize, DimInfoHandler *handler)
	{
		subscribe((char *)name, time, nolink, nolinksize, handler, dnsid);
	};

	virtual ~DimUpdatedInfo();
	void subscribe(char *name, void *nolink, int nolinksize, int time, 
		DimInfoHandler *handler, dim_long dnsid)
		{ subscribe((char *)name, time, nolink, nolinksize, handler, dnsid); };

private :
	void doIt() DIMXX_OVERRIDE;
	void subscribe(char *name, int time, void *nolink, int nolinksize,
		DimInfoHandler *handler, dim_long dnsid) DIMXX_OVERRIDE;
};

class DllExp DimCmnd {
public :

	int wakeUp;
	int result;
	int send(char *name, void *data, int datasize);
	void sendNB(char *name, void *data, int datasize);
	int send(dim_long dnsid, char *name, void *data, int datasize);
	void sendNB(dim_long dnsid, char *name, void *data, int datasize);
	DimCmnd(){};
};

class DllExp DimCurrentInfo {

public :
	void *itsData;
	int itsDataSize;
	int itsSize;
//	int itsTagId;
	int wakeUp;

	DimCurrentInfo(){
		subscribe((char *)0, 0, (void *)0, 0, 0); };
	DimCurrentInfo(const char *name, int nolink) { 
		subscribe((char *)name, 0, &nolink, sizeof(int), 0); };
	DimCurrentInfo(const char *name, float nolink) { 
		subscribe((char *)name, 0, &nolink, sizeof(float), 0); };
	DimCurrentInfo(const char *name, double nolink) { 
		subscribe((char *)name, 0, &nolink, sizeof(double), 0); };
	DimCurrentInfo(const char *name, longlong nolink) { 
		subscribe((char *)name, 0, &nolink, sizeof(longlong), 0); };
	DimCurrentInfo(const char *name, short nolink) { 
		subscribe((char *)name, 0, &nolink, sizeof(short), 0); };
	DimCurrentInfo(const char *name, char *nolink) { 
		subscribe((char *)name, 0, nolink, (int)strlen(nolink)+1, 0); };
	DimCurrentInfo(const char *name, void *nolink, int nolinksize) { 
		subscribe((char *)name, 0, nolink, nolinksize, 0); };
	DimCurrentInfo(const char *name, int time, int nolink) { 
		subscribe((char *)name, time, &nolink, sizeof(int), 0); };
	DimCurrentInfo(const char *name, int time, float nolink) { 
		subscribe((char *)name, time, &nolink, sizeof(float), 0); };
	DimCurrentInfo(const char *name, int time, double nolink) { 
		subscribe((char *)name, time, &nolink, sizeof(double), 0); };
	DimCurrentInfo(const char *name, int time, longlong nolink) { 
		subscribe((char *)name, time, &nolink, sizeof(longlong), 0); };
	DimCurrentInfo(const char *name, int time, short nolink) { 
		subscribe((char *)name, time, &nolink, sizeof(short), 0); };
	DimCurrentInfo(const char *name, int time, char *nolink) { 
		subscribe((char *)name, time, nolink, (int)strlen(nolink)+1, 0); };
	DimCurrentInfo(const char *name, int time, void *nolink, int nolinksize) { 
		subscribe((char *)name, time, nolink, nolinksize, 0); };

	DimCurrentInfo(dim_long dnsid, const char *name, int nolink) {
		subscribe((char *)name, 0, &nolink, sizeof(int), dnsid);
	};
	DimCurrentInfo(dim_long dnsid, const char *name, float nolink) {
		subscribe((char *)name, 0, &nolink, sizeof(float), dnsid);
	};
	DimCurrentInfo(dim_long dnsid, const char *name, double nolink) {
		subscribe((char *)name, 0, &nolink, sizeof(double), dnsid);
	};
	DimCurrentInfo(dim_long dnsid, const char *name, longlong nolink) {
		subscribe((char *)name, 0, &nolink, sizeof(longlong), dnsid);
	};
	DimCurrentInfo(dim_long dnsid, const char *name, short nolink) {
		subscribe((char *)name, 0, &nolink, sizeof(short), dnsid);
	};
	DimCurrentInfo(dim_long dnsid, const char *name, char *nolink) {
		subscribe((char *)name, 0, nolink, (int)strlen(nolink) + 1, dnsid);
	};
	DimCurrentInfo(dim_long dnsid, const char *name, void *nolink, int nolinksize) {
		subscribe((char *)name, 0, nolink, nolinksize, dnsid);
	};
	DimCurrentInfo(dim_long dnsid, const char *name, int time, int nolink) {
		subscribe((char *)name, time, &nolink, sizeof(int), dnsid);
	};
	DimCurrentInfo(dim_long dnsid, const char *name, int time, float nolink) {
		subscribe((char *)name, time, &nolink, sizeof(float), dnsid);
	};
	DimCurrentInfo(dim_long dnsid, const char *name, int time, double nolink) {
		subscribe((char *)name, time, &nolink, sizeof(double), dnsid);
	};
	DimCurrentInfo(dim_long dnsid, const char *name, int time, longlong nolink) {
		subscribe((char *)name, time, &nolink, sizeof(longlong), dnsid);
	};
	DimCurrentInfo(dim_long dnsid, const char *name, int time, short nolink) {
		subscribe((char *)name, time, &nolink, sizeof(short), dnsid);
	};
	DimCurrentInfo(dim_long dnsid, const char *name, int time, char *nolink) {
		subscribe((char *)name, time, nolink, (int)strlen(nolink) + 1, dnsid);
	};
	DimCurrentInfo(dim_long dnsid, const char *name, int time, void *nolink, int nolinksize) {
		subscribe((char *)name, time, nolink, nolinksize, dnsid);
	};


	virtual ~DimCurrentInfo();
	char *getName()  { return itsName; } ;
	void *getData();
	int getInt() { return *(int *)getData(); } ;
	float getFloat() { return *(float *)getData(); } ;
	double getDouble() { return *(double *)getData(); } ;
	longlong getLonglong() { return *(longlong *)getData(); } ;
	short getShort() { return *(short *)getData(); } ;
	char *getString()  { return (char *)getData(); } ;
	int getSize()  { getData(); return itsSize; } ;
	void subscribe(char *name, void *nolink, int nolinksize, int time, dim_long dnsid)
		{ subscribe((char *)name, time, nolink, nolinksize, dnsid); };

private :
	char *itsName;
	void *itsNolinkBuf;
	int itsNolinkSize;
	dim_long itsDnsId;
	void subscribe(char *name, int time, void *nolink, int nolinksize, dim_long dnsid);
};

class DllExp DimRpcInfo : public DimTimer {
public :
	int itsId;
//	int itsTagId;
	int itsInit;
	void *itsData;
	int itsDataSize;
	void *itsDataOut;
	int itsDataOutSize;
	int itsSize;
	int wakeUp;
	int itsWaiting;
	int itsConnected;
	void *itsNolinkBuf;
	int itsNolinkSize;
	DimRpcInfo *itsHandler;

	DimRpcInfo(const char *name, int nolink) { 
		subscribe((char *)name, 0, 0, &nolink, sizeof(int), 0, 0); };
	DimRpcInfo(const char *name, float nolink) { 
		subscribe((char *)name, 0, 0, &nolink, sizeof(float), 0, 0); };
	DimRpcInfo(const char *name, double nolink) { 
		subscribe((char *)name, 0, 0, &nolink, sizeof(double), 0, 0); };
	DimRpcInfo(const char *name, longlong nolink) { 
		subscribe((char *)name, 0, 0, &nolink, sizeof(longlong), 0, 0); };
	DimRpcInfo(const char *name, short nolink) { 
		subscribe((char *)name, 0, 0, &nolink, sizeof(short), 0, 0); };
	DimRpcInfo(const char *name, char *nolink) { 
		subscribe((char *)name, 0, 0, nolink, (int)strlen(nolink)+1, 0, 0); };
	DimRpcInfo(const char *name, void *nolink, int nolinksize) { 
		subscribe((char *)name, 0, 0, nolink, nolinksize, 0, 0); };

	DimRpcInfo(const char *name, int time, int nolink) { 
		subscribe((char *)name, 0, 0, &nolink, sizeof(int), time, 0); };
	DimRpcInfo(const char *name, int time, float nolink) { 
		subscribe((char *)name, 0, 0, &nolink, sizeof(float), time, 0); };
	DimRpcInfo(const char *name, int time, double nolink) { 
		subscribe((char *)name, 0, 0, &nolink, sizeof(double), time, 0); };
	DimRpcInfo(const char *name, int time, longlong nolink) { 
		subscribe((char *)name, 0, 0, &nolink, sizeof(longlong), time, 0); };
	DimRpcInfo(const char *name, int time, short nolink) { 
		subscribe((char *)name, 0, 0, &nolink, sizeof(short), time, 0); };
	DimRpcInfo(const char *name, int time, char *nolink) { 
		subscribe((char *)name, 0, 0, nolink, (int)strlen(nolink)+1, time, 0); };
	DimRpcInfo(const char *name, int time, void *nolink, int nolinksize) { 
		subscribe((char *)name, 0, 0, nolink, nolinksize, time, 0); };
	
	DimRpcInfo(dim_long dnsid, const char *name, int nolink) {
		subscribe((char *)name, 0, 0, &nolink, sizeof(int), 0, dnsid);
	};
	DimRpcInfo(dim_long dnsid, const char *name, float nolink) {
		subscribe((char *)name, 0, 0, &nolink, sizeof(float), 0, dnsid);
	};
	DimRpcInfo(dim_long dnsid, const char *name, double nolink) {
		subscribe((char *)name, 0, 0, &nolink, sizeof(double), 0, dnsid);
	};
	DimRpcInfo(dim_long dnsid, const char *name, longlong nolink) {
		subscribe((char *)name, 0, 0, &nolink, sizeof(longlong), 0, dnsid);
	};
	DimRpcInfo(dim_long dnsid, const char *name, short nolink) {
		subscribe((char *)name, 0, 0, &nolink, sizeof(short), 0, dnsid);
	};
	DimRpcInfo(dim_long dnsid, const char *name, char *nolink) {
		subscribe((char *)name, 0, 0, nolink, (int)strlen(nolink) + 1, 0, dnsid);
	};
	DimRpcInfo(dim_long dnsid, const char *name, void *nolink, int nolinksize) {
		subscribe((char *)name, 0, 0, nolink, nolinksize, 0, dnsid);
	};

	DimRpcInfo(dim_long dnsid, const char *name, int time, int nolink) {
		subscribe((char *)name, 0, 0, &nolink, sizeof(int), time, dnsid);
	};
	DimRpcInfo(dim_long dnsid, const char *name, int time, float nolink) {
		subscribe((char *)name, 0, 0, &nolink, sizeof(float), time, dnsid);
	};
	DimRpcInfo(dim_long dnsid, const char *name, int time, double nolink) {
		subscribe((char *)name, 0, 0, &nolink, sizeof(double), time, dnsid);
	};
	DimRpcInfo(dim_long dnsid, const char *name, int time, longlong nolink) {
		subscribe((char *)name, 0, 0, &nolink, sizeof(longlong), time, dnsid);
	};
	DimRpcInfo(dim_long dnsid, const char *name, int time, short nolink) {
		subscribe((char *)name, 0, 0, &nolink, sizeof(short), time, dnsid);
	};
	DimRpcInfo(dim_long dnsid, const char *name, int time, char *nolink) {
		subscribe((char *)name, 0, 0, nolink, (int)strlen(nolink) + 1, time, dnsid);
	};
	DimRpcInfo(dim_long dnsid, const char *name, int time, void *nolink, int nolinksize) {
		subscribe((char *)name, 0, 0, nolink, nolinksize, time, dnsid);
	};

	virtual void rpcInfoHandler();

	virtual ~DimRpcInfo();
	int getId() {return itsId;};
	void keepWaiting() {itsWaiting = 2;};
	char *getName()  { return itsName; } ;
	void *getData();
	int getInt() { return *(int *)getData(); } ;
	float getFloat() { return *(float *)getData(); } ;
	double getDouble() { return *(double *)getData(); } ;
	longlong getLonglong() { return *(longlong *)getData(); } ;
	short getShort() { return *(short *)getData(); } ;
	char *getString()  { return (char *)getData(); } ;
	int getSize()  { getData(); return itsSize; } ;

	void setData(void *data, int size) { doIt(data, size); };
	void setData(int &data) { doIt(&data, sizeof(int)); } ;
	void setData(float &data) { doIt(&data, sizeof(float)); } ;
	void setData(double &data) { doIt(&data, sizeof(double)); } ;
	void setData(longlong &data) { doIt(&data, sizeof(longlong)); } ;
	void setData(short &data) { doIt(&data, sizeof(short)); } ;
	void setData(char *data)  { doIt(data, (int)strlen(data)+1); } ;

private :
	char *itsName;
	char *itsNameIn;
	char *itsNameOut;
	int itsTimeout;
	dim_long itsDnsId;
	void subscribe(char *name, void *data, int size,
		void *nolink, int nolinksize, int timeout, dim_long dnsid);
	void doIt(void *data, int size);
	void timerHandler() DIMXX_OVERRIDE;
};

class DllExp DimClient : public DimInfoHandler, public DimErrorHandler
{
public:

	static char *dimDnsNode;
	static DimErrorHandler *itsCltError;

	DimClient();
	virtual ~DimClient();
	static int sendCommand(const char *name, int data);
	static int sendCommand(const char *name, float data);
	static int sendCommand(const char *name, double data);
	static int sendCommand(const char *name, longlong data);
	static int sendCommand(const char *name, short data);
	static int sendCommand(const char *name, const char *data);
	static int sendCommand(const char *name, void *data, int datasize);
	static void sendCommandNB(const char *name, int data);
	static void sendCommandNB(const char *name, float data);
	static void sendCommandNB(const char *name, double data);
	static void sendCommandNB(const char *name, longlong data);
	static void sendCommandNB(const char *name, short data);
	static void sendCommandNB(const char *name, char *data);
	static void sendCommandNB(const char *name, void *data, int datasize);

	static int sendCommand(dim_long dnsid, const char *name, int data);
	static int sendCommand(dim_long dnsid, const char *name, float data);
	static int sendCommand(dim_long dnsid, const char *name, double data);
	static int sendCommand(dim_long dnsid, const char *name, longlong data);
	static int sendCommand(dim_long dnsid, const char *name, short data);
	static int sendCommand(dim_long dnsid, const char *name, const char *data);
	static int sendCommand(dim_long dnsid, const char *name, void *data, int datasize);
	static void sendCommandNB(dim_long dnsid, const char *name, int data);
	static void sendCommandNB(dim_long dnsid, const char *name, float data);
	static void sendCommandNB(dim_long dnsid, const char *name, double data);
	static void sendCommandNB(dim_long dnsid, const char *name, longlong data);
	static void sendCommandNB(dim_long dnsid, const char *name, short data);
	static void sendCommandNB(dim_long dnsid, const char *name, char *data);
	static void sendCommandNB(dim_long dnsid, const char *name, void *data, int datasize);

	static int setExitHandler(const char *serverName);
	static int killServer(const char *serverName);
	static int setDnsNode(const char *node);
	static int setDnsNode(const char *node, int port);
	static char *getDnsNode();
	static int getDnsPort();
	static dim_long addDns(const char *node);
	static dim_long addDns(const char *node, int port);
	static void stopDns(dim_long dnsid);
	static void addErrorHandler(DimErrorHandler *handler);
	void addErrorHandler();
	virtual void errorHandler(int /* severity */, int /* code */, char* /* msg */)  DIMXX_OVERRIDE {};
	static char *serverName;
	// Get Current Server Identifier	
	static int getServerId();
	// Get Current Server Process Identifier	
	static int getServerPid();
	// Get Current Server Name	
	static char *getServerName();
	static char **getServerServices();
//	static char *getServerServices(int serverId);

	virtual void infoHandler()  DIMXX_OVERRIDE {};

	static int dicNoCopy;
	static void setNoDataCopy();
	static int getNoDataCopy();
	static int inCallback();
};

class DllExp DimBrowser
{
public :

	DimBrowser();
	DimBrowser(dim_long dnsid);

	~DimBrowser();

	int getServices(const char *serviceName);
	int getServices(const char *serviceName, int timeout);
	int getServers();
	int getServers(int timeout);
	int getServerServices(const char *serverName);
	int getServerServices(const char *serverName, int timeout);
	int getServerClients(const char *serverName);
	int getServerClients(const char *serverName, int timeout);
/*
	int getServices(dim_long dnsid, const char *serviceName);
	int getServices(dim_long dnsid, const char *serviceName, int timeout);
	int getServers(dim_long dnsid);
	int getServers(dim_long dnsid, int timeout);
	int getServerServices(dim_long dnsid, const char *serverName);
	int getServerServices(dim_long dnsid, const char *serverName, int timeout);
	int getServerClients(dim_long dnsid, const char *serverName);
	int getServerClients(dim_long dnsid, const char *serverName, int timeout);
*/
	int getNextService(char *&service, char *&format);
	int getNextServer(char *&server, char *&node);
	int getNextServer(char *&server, char *&node, int &pid);
	int getNextServerService(char *&service, char *&format);
	int getNextServerClient(char *&client, char *&node);

private:

	TokenString *itsData[5];
	int currIndex; 
	char *currToken;
	char none;
	DimRpcInfo *browserRpc;
	dim_long itsDnsId;
	int doGetServices(const char *serviceName, int timeout);
	int doGetServers(int timeout);
	int doGetServerServices(const char *serviceName, int timeout);
	int doGetServerClients(const char *serviceName, int timeout);
};

#undef DIMXX_OVERRIDE

#endif
