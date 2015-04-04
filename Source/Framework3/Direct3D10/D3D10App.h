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

#ifndef _D3D10APP_H_
#define _D3D10APP_H_

#include "Direct3D10Renderer.h"
#include "../BaseApp.h"

#define NO_SETTING_CHANGE 0x1

class D3D10App : public BaseApp {
public:
	D3D10App();

	virtual bool initCaps();
	virtual bool initAPI();
	virtual void exitAPI();

	void beginFrame();
	void endFrame();

	virtual void onSize(const int w, const int h);

	bool captureScreenshot(Image &img);

protected:
	bool initAPI(const DXGI_FORMAT backBufferFmt, const DXGI_FORMAT depthBufferFmt, const int samples, const uint flags);
	bool createBuffers();
	bool deleteBuffers();

	ID3D10Device *device;
	IDXGISwapChain *swapChain;

	ID3D10Texture2D *backBuffer;
	ID3D10Texture2D *depthBuffer;
	ID3D10RenderTargetView *backBufferRTV;
	ID3D10DepthStencilView *depthBufferDSV;
	DXGI_FORMAT backBufferFormat;
	DXGI_FORMAT depthBufferFormat;
	int msaaSamples;
};

#endif // _D3D10APP_H_
