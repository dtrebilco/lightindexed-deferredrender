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

#ifdef _WIN32
#include <shlobj.h>
#else
#include <sys/time.h>
#ifdef LINUX
#include <GL/glx.h>
#endif
#endif

#include "Config.h"
#include "GUI/Dialog.h"
#include "GUI/CheckBox.h"
#include "GUI/Slider.h"
#include "GUI/Label.h"
#include "GUI/DropDownList.h"

class BaseApp : public SliderListener, public CheckBoxListener, public DropDownListener, public PushButtonListener {
public:
	BaseApp();
	virtual ~BaseApp();

	virtual char *getTitle() const = 0;

	void loadConfig();
	void updateConfig();
	virtual void onCheckBoxClicked(CheckBox *checkBox);
	virtual void onSliderChanged(Slider *Slider);
	virtual void onDropDownChanged(DropDownList *dropDownList);
	virtual void onButtonClicked(PushButton *button);

	virtual bool init(){ return true; };
	virtual void exit(){};

	virtual bool initCaps() = 0;
	virtual bool initAPI() = 0;
	virtual void exitAPI() = 0;

	void initGUI();

	virtual bool load(){ return true; };
	virtual void unload(){};

	virtual void beginFrame() = 0;
	virtual void drawFrame() = 0;
	void drawGUI();
	virtual void endFrame() = 0;
	void initTime();
	void updateTime();
	void makeFrame();

	virtual void controls();
	virtual void resetCamera();
	virtual void moveCamera(const vec3 &dir);

	virtual bool onMouseMove(const int x, const int y, const int deltaX, const int deltaY);
	virtual bool onMouseButton(const int x, const int y, const MouseButton button, const bool pressed);
	virtual bool onMouseWheel(const int x, const int y, const int scroll);
	virtual bool onKey(const uint key, const bool pressed);
	virtual void onSize(const int w, const int h);
	virtual void onClose();

	bool isDone() const { return done; }

	void toggleFullscreen();
	void closeWindow(const bool quit, const bool callUnLoad);

	void captureMouse(const bool value);
	void setCursorPos(const int x, const int y);

#if defined(_WIN32)
	void setInstance(const HINSTANCE hInst){ hInstance = hInst; }
	HWND getWindow() const { return hwnd; }
#elif defined(LINUX)
	void setDisplay(Display *disp){ display = disp; }
#elif defined(__APPLE__)
	WindowRef getWindow() const { return window; }
#endif

	int getWidth() const { return width; }
	int getHeight() const { return height; }

	bool saveScreenshot();
	virtual bool captureScreenshot(Image &img) = 0;

protected:
	void setWindowTitle(const char *title);

	Renderer *renderer;

	vec3 camPos;
	float wx, wy;
	float speed;

	float time, frameTime;
	FontID defaultFont;
	SamplerStateID linearClamp;
	BlendStateID blendSrcAlpha;
	DepthStateID noDepthTest, noDepthWrite;
	RasterizerStateID cullNone, cullBack, cullFront;


#if defined(_WIN32)
	HINSTANCE hInstance;
	HWND hwnd;
	DISPLAY_DEVICE device;
	LARGE_INTEGER freq, prev, curr;
#else
	timeval prev, curr;

#if defined(LINUX)
	Display *display;
	Window window;
	Cursor blankCursor;
#elif defined(__APPLE__)
	WindowRef window;
#endif

#endif

	Queue <Widget *> widgets;
	Dialog *configDialog, *keysDialog;
	CheckBox *invertMouseBox, *fullscreenBox;
	Slider *mouseSensSlider;
	DropDownList *resolution, *antiAlias;
	PushButton *applyRes, *configureKeys;

	bool keys[65536];
	uint leftKey, rightKey, upKey, downKey, forwardKey, backwardKey, resetKey, fpsKey, optionsKey, screenshotKey, benchmarkKey;

	Config config;
	int width, height, fullscreenWidth, fullscreenHeight, screen;
	int colorBits, depthBits, stencilBits;

	int antiAliasSamples;

	bool fullscreen;
	bool mouseCaptured;
	bool done;

	bool invertMouse;
	float mouseSensibility;
	bool showFPS;

	// Benchmarking
	FILE *benchMarkFile;
};
