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

#ifndef _GLAPP_H_
#define _GLAPP_H_

#include "OpenGLRenderer.h"
#include "../BaseApp.h"

#ifdef LINUX
#include <X11/extensions/xf86vmode.h>
#endif

class OpenGLApp : public BaseApp {
public:
	OpenGLApp();

	virtual bool initCaps();

	virtual bool initAPI();
	virtual void exitAPI();

	void beginFrame();
	void endFrame();

	virtual void onSize(const int w, const int h);

	bool captureScreenshot(Image &img);

protected:

#if defined(_WIN32)
	HDC hdc;
	HGLRC glContext;
#elif defined(LINUX)
	GLXContext glContext;
    XF86VidModeModeInfo **dmodes;
#elif defined(__APPLE__)
	AGLContext glContext;
	CFArrayRef dmodes;
	CFDictionaryRef initialMode;
#endif
};

#endif // _GLAPP_H_
