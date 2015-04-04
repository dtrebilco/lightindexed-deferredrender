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

#ifndef _BUTTON_H_
#define _BUTTON_H_

#include "Widget.h"

class PushButton;

class PushButtonListener {
public:
	virtual ~PushButtonListener(){}

	virtual void onButtonClicked(PushButton *button) = 0;
};


class PushButton : public Widget {
public:
	PushButton(const float x, const float y, const float w, const float h, const char *txt);
	virtual ~PushButton();

	void setListener(PushButtonListener *listener){ buttonListener = listener; }

	virtual bool onMouseButton(const int x, const int y, const MouseButton button, const bool pressed);
	virtual bool onKey(const unsigned int key, const bool pressed);

	virtual void draw(Renderer *renderer, const FontID defaultFont, const SamplerStateID linearClamp, const BlendStateID blendSrcAlpha, const DepthStateID depthState);

protected:
	void drawButton(Renderer *renderer, const char *text, const FontID defaultFont, const SamplerStateID linearClamp, const BlendStateID blendSrcAlpha, const DepthStateID depthState);

	char *text;

	PushButtonListener *buttonListener;

	bool pushed;
};

class KeyWaiterButton : public PushButton {
public:
	KeyWaiterButton(const float x, const float y, const float w, const float h, const char *txt, uint *key);
	virtual ~KeyWaiterButton();

	bool onMouseButton(const int x, const int y, const MouseButton button, const bool pressed);
	bool onKey(const unsigned int key, const bool pressed);

	void draw(Renderer *renderer, const FontID defaultFont, const SamplerStateID linearClamp, const BlendStateID blendSrcAlpha, const DepthStateID depthState);
protected:
	uint *targetKey;

	bool waitingForKey;

};

#endif // _BUTTON_H_
