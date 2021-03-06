#ifndef _SYSTEMCLASS_H_
#define _SYSTEMCLASS_H_

// ***Quelle***: http://www.rastertek.com/tutdx11.html


#define WIN32_LEAN_AND_MEAN

#include <windows.h>
#include "inputclass.h"
#include "Scene.h"

#include "FpsClass.h"
#include "TimerClass.h"


using namespace std;

class SystemClass
{
public:
	SystemClass();
	SystemClass(const SystemClass&);
	~SystemClass();

	bool Initialize();
	void Shutdown();
	void Run();

	LRESULT CALLBACK MessageHandler(HWND, UINT, WPARAM, LPARAM);

private:
	bool Frame();
	void InitializeWindows(int&, int&);
	void ShutdownWindows();

private:
	LPCWSTR m_applicationName;
	HINSTANCE m_hinstance;
	HWND m_hwnd;

	InputClass* m_Input;
	Scene* m_Scene;

	FpsClass* m_Fps;
	TimerClass* m_Timer;
};

static LRESULT CALLBACK WndProc(HWND, UINT, WPARAM, LPARAM);

static SystemClass* ApplicationHandle = NULL;

#endif