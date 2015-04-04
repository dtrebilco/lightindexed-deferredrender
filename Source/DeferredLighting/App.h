/* ============================================================================
  Light Indexed Deferred Rendering Demo
  By Damian Trebilco
 
  Origional base lighting demo by "Humus"  
============================================================================ */

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

#include "../Framework3/OpenGL/OpenGLApp.h"
#include "../Framework3/Util/Model.h"
#include "../Framework3/Util/BSP.h"
#include "../Framework3/Math/Scissor.h"

#define MAX_LIGHT_TOTAL             255  // Must be a dimension supported by textures, then -1

// Helper structure to store light data
struct LightData
{
  LightData(const vec3& setColor, const vec3& setPosition, float setSize);

  bool isEnabled;     // If the particle is enabled
  int screenX;        // The screen X position
  int screenY;        // The screen Y position
  int screenWidth;    // The width on the screen  
  int screenHeight;   // The height on the screen

  vec3 color;         // The light color
  vec3 position;      // The light position
  float size;         // The light size
};

class App : public OpenGLApp {
public:
  App();
  char *getTitle() const { return "Light Indexed Deferred Lighting"; }

  void resetCamera();
  void moveCamera(const vec3 &dir);

  bool init();
  void exit();

  void onSize(const int w, const int h);
	bool onKey(const uint key, const bool pressed);
	bool onMouseMove(const int x, const int y, const int deltaX, const int deltaY);
	bool onMouseButton(const int x, const int y, const MouseButton button, const bool pressed);
	bool onMouseWheel(const int x, const int y, const int scroll);

  bool load();
  void unload();
  void createSphereModel();

  void updateLightCull();
  void updateBitMaskedLightTextures();

  void drawLightParticles(const vec3 &dx, const vec3 &dy);

  void drawLightingMPAmbient();
  void drawLightingMP();

  void drawDepthOnly();
  void drawLIDeferLight(GLubyte lightIndex, const vec3 &lightPosition, float lightSize);
  void drawLIDeferLights();
  void drawLIDeferLitObjects();

  void drawFrame();

protected:

  mat4 projectionMatrix;   // The current frame's projection matrix
  mat4 modelviewMatrix;    // The current frame's modelview matrix

  static LightData lightDataArray[MAX_LIGHT_TOTAL]; 
  bool staticLightSceneSet; // Flag indicating to set the static light scene

  Model *map;
  BSP bsp;

  ShaderID depthOnly, plainTex, lightingMP, lightingMP_ambient, lightingMP_stone, lightingMP_stone_ambient;
  TextureID base[4], bump[4], gloss[4], light, noise3D;
  SamplerStateID trilinearAniso, linearWrap, pointClamp;
  BlendStateID blendCopy, blendAdd, noColorWrite;
  DepthStateID depthNoWritePassGreater;

  float parallax[4];

  Model *sphereModel;
  Model *horseModel;

  ShaderID cmpTex;
  ShaderID plainColor;
  ShaderID lightingColorOnly;
  ShaderID lightingColorOnly_depthClamp;
  ShaderID lightingLIDefer[4];
  ShaderID lightingLIDefer_stone[4];

  TextureID lightIndexBuffer;
  TextureID depthRT;

  TextureID bitMaskLightColors;
  TextureID bitMaskLightPos;

  BlendStateID blendTwoLightRender;
  BlendStateID blendBitShift;
  BlendStateID blendMax;

  CheckBox *animateLights;
  CheckBox *staticLightScene;

  CheckBox *useStencilMasking;
  CheckBox *useDepthBoundsTest;
  DropDownList *lightCountPerFragment;
  CheckBox *useDeferedLighting;

  CheckBox *doPrecisionTest;

  // Position light editor methods
  bool GetSpherePosition(const int x, const int y);
  bool onKeyEditor(const uint key, const bool pressed);
  bool onMouseWheelEditor(const int x, const int y, const int scroll);
  bool onMouseButtonEditor(const int x, const int y, const MouseButton button, const bool pressed);
  bool onMouseMoveEditor(const int x, const int y, const int deltaX, const int deltaY);
  void drawFrameEditor();

  // Update PFX moving lights if enabled
  void updateLights(float updateTime);
  void updatePrimaryLights(float t);
  void SetStaticLightScene();

  // Precision of the graphics card methods
  void drawPrecisionTest1();
};
