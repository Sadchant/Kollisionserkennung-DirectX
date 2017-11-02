// The FpsClass is simply a counter with a timer associated with it.It counts how many frames occur in a one second period and constantly
// updates that count.
#pragma once
/////////////
// LINKING //
/////////////
#pragma comment(lib, "winmm.lib")


//////////////
// INCLUDES //
//////////////
#include <windows.h>
#include <mmsystem.h>

#define AMOUNT 50


////////////////////////////////////////////////////////////////////////////////
// Class name: FpsClass
////////////////////////////////////////////////////////////////////////////////
class FpsClass
{
public:
	FpsClass();
	FpsClass(const FpsClass&);
	~FpsClass();

	void Initialize();
	void Frame();
	int GetMS();
	int GetFPS();

private:
	int m_fps, m_msFrame, m_count;
	unsigned long m_msStartTime, m_startTime;

	int m_Counter;
	unsigned long m_Last10FrameMS[AMOUNT] = {0};
};