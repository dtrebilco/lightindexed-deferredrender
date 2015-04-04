/***********      .---.         .-"-.      *******************\
* -------- *     /   ._.       / ´ ` \     * ---------------- *
* Author's *     \_  (__\      \_°v°_/     * humus@rogers.com *
*   note   *     //   \\       //   \\     * ICQ #47010716    *
* -------- *    ((     ))     ((     ))    * ---------------- *
*          ****--""---""-------""---""--****                  ********\
* This file is a part of the work done by Humus. You are free to use  *
* the code in any way you like, modified, unmodified or copy'n'pasted *
* into your own work. However, I expect you to respect these points:  *
*  @ If you use this file and its contents unmodified, or use a major *
*    part of this file, please credit the author and leave this note. *
*  @ For use in anything commercial, please request my approval.      *
*  @ Share your work and ideas too as much as you can.                *
\*********************************************************************/

#include "Resource.h"

#include "../CPU.h"
#include "../BaseApp.h"
#include <direct.h>

extern BaseApp *app;

#define GETX(l) (int(l & 0xFFFF))
#define GETY(l) (int(l) >> 16)

LRESULT CALLBACK WinProc(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam){
	switch (message){
	case WM_MOUSEMOVE:
		static int lastX, lastY;
		int x, y;
		x = GETX(lParam);
		y = GETY(lParam);
		app->onMouseMove(x, y, x - lastX, y - lastY);
		lastX = x;
		lastY = y;
		break;
	case WM_KEYDOWN:
		app->onKey((unsigned int) wParam, true);
		break;
	case WM_KEYUP:
		app->onKey((unsigned int) wParam, false);
		break;
	case WM_LBUTTONDOWN:
		app->onMouseButton(GETX(lParam), GETY(lParam), MOUSE_LEFT, true);
 		break;
	case WM_LBUTTONUP:
		app->onMouseButton(GETX(lParam), GETY(lParam), MOUSE_LEFT, false);
		break;
	case WM_RBUTTONDOWN:
		app->onMouseButton(GETX(lParam), GETY(lParam), MOUSE_RIGHT, true);
 		break;
	case WM_RBUTTONUP:
		app->onMouseButton(GETX(lParam), GETY(lParam), MOUSE_RIGHT, false);
		break;
	case WM_MOUSEWHEEL:
		static int scroll;
		int s;

		scroll += GET_WHEEL_DELTA_WPARAM(wParam);
		s = scroll / WHEEL_DELTA;
		scroll %= WHEEL_DELTA;

		POINT point;
		point.x = GETX(lParam);
		point.y = GETY(lParam);
		ScreenToClient(hwnd, &point);

		if (s != 0) app->onMouseWheel(point.x, point.y, s);
		break;
	case WM_SIZE:
		app->onSize(LOWORD(lParam), HIWORD(lParam));
		break;
	case WM_WINDOWPOSCHANGED:
		if ((((LPWINDOWPOS) lParam)->flags & SWP_NOSIZE) == 0){
			RECT rect;
			GetClientRect(hwnd, &rect);
			int w = rect.right - rect.left;
			int h = rect.bottom - rect.top;
			if (w > 0 && h > 0) app->onSize(w, h);
		}
		break;
	case WM_SYSKEYDOWN:
		if ((lParam & (1 << 29)) && wParam == KEY_ENTER){
			app->toggleFullscreen();
		} else {
			app->onKey((unsigned int) wParam, true);
		}
		break;
	case WM_SYSKEYUP:
			app->onKey((unsigned int) wParam, false);
		break;
	case WM_CREATE:
		ShowWindow(hwnd, SW_SHOW);
		break;
	case WM_CLOSE:
		app->closeWindow(true, true);
		break;
	case WM_DESTROY:
		PostQuitMessage(0);
		break;
	default:
		return DefWindowProc(hwnd, message, wParam, lParam);
	}
	return 0;
}

#ifdef _DEBUG
#include <crtdbg.h>
#endif

#include <stdio.h>

int WINAPI WinMain(HINSTANCE hThisInst, HINSTANCE hLastInst, LPSTR lpszCmdLine, int nCmdShow){
#ifdef _DEBUG
	int flag = _CrtSetDbgFlag(_CRTDBG_REPORT_FLAG); // Get current flag
	flag |= _CRTDBG_LEAK_CHECK_DF; // Turn on leak-checking bit
	flag |= _CRTDBG_CHECK_ALWAYS_DF; // Turn on CrtCheckMemory
//	flag |= _CRTDBG_DELAY_FREE_MEM_DF;
	_CrtSetDbgFlag(flag); // Set flag to the new value
#endif
	initCPU();

	// Make sure we're running in the exe's path
	char path[MAX_PATH];
	if (GetModuleFileName(NULL, path, sizeof(path))){
		char *slash = strrchr(path, '\\');
		if (slash) *slash = '\0';
        chdir(path);
	}

	MSG msg;
	WNDCLASS wincl;
	wincl.hInstance = hThisInst;
	wincl.lpszClassName = "Humus";
	wincl.lpfnWndProc = WinProc;
	wincl.style = 0;
	wincl.hIcon = LoadIcon(hThisInst, MAKEINTRESOURCE(IDI_MAINICON));
	wincl.hCursor = LoadCursor(NULL, IDI_APPLICATION);
	wincl.lpszMenuName = NULL;
	wincl.cbClsExtra = 0;
	wincl.cbWndExtra = 0;
	wincl.hbrBackground = NULL;
	if (!RegisterClass(&wincl)) return 0;

	app->setInstance(hThisInst);

	// Initialize timer
	app->initTime();

	app->loadConfig();
	app->initGUI();

	/*
		Force the main thread to always run on CPU 0.
		This is done because on some systems QueryPerformanceCounter returns a bit different counter values
		on the different CPUs (contrary to what it's supposed to do), which can cause negative frame times
		if the thread is scheduled on the other CPU in the next frame. This can cause very jerky behavior and
		appear as if frames return out of order.
	*/
	SetThreadAffinityMask(GetCurrentThread(), 1);

	if (app->init()){
		app->resetCamera();

		do {
			app->loadConfig();

			if (!app->initCaps()) break;
			if (!app->initAPI()) break;

			if (!app->load()){
				app->closeWindow(true, false);
			}

			while (true){
				while (PeekMessage(&msg, NULL, 0, 0, PM_REMOVE)){
					//TranslateMessage(&msg);
					DispatchMessage(&msg);
				}
				if (msg.message == WM_QUIT) break;

				app->updateTime();
				app->makeFrame();
			}
		} while (!app->isDone());

		app->exit();
	}

	delete app;

	return (int) msg.wParam;
}
