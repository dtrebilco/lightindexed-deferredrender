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

#ifndef _D3DAPP_H_
#define _D3DAPP_H_

#include "Direct3DRenderer.h"
#include "../BaseApp.h"

class D3DApp : public BaseApp {
public:
	D3DApp();

	bool initCaps();
	bool initAPI();
	void exitAPI();

	void beginFrame();
	void endFrame();

	virtual void onSize(const int w, const int h);
	virtual bool onReset(){ return true; }

	bool captureScreenshot(Image &img);

protected:
	D3DCAPS9 caps;
	LPDIRECT3D9 d3d;
	LPDIRECT3DDEVICE9 dev;
    D3DPRESENT_PARAMETERS d3dpp;
};

#endif // _D3DAPP_H_
