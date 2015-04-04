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

#include "App.h"

BaseApp *app = new App();

#define SUBDIV_LEVEL 3
#define POW4(x) (1 << (2 * (x)))
#define SPHERE_SIZE (8 * 3 * POW4(SUBDIV_LEVEL))

enum LightCountPerFragment
{ 
  LCPF_One   = 0,  // Max of 1 light per fragment supported
  LCPF_Two   = 1,  // Max of 2 lights per fragment supported
  LCPF_Three = 2,  // Max of 3 lights per fragment supported
  LCPF_Four  = 3   // Max of 4 lights per fragment supported
};

LightData App::lightDataArray[MAX_LIGHT_TOTAL] = {
#include "LightPositions.h"
};

///////////////////////////////////////////////////////////////////////////////
//
LightData::LightData(const vec3& setColor, const vec3& setPosition, float setSize):
  isEnabled(false),
  screenX(0),
  screenY(0),
  screenWidth(0),
  screenHeight(0),
  color(setColor),
  position(setPosition),
  size(setSize)
{
}

///////////////////////////////////////////////////////////////////////////////
//
App::App():
  bitMaskLightColors(TEXTURE_NONE),
  bitMaskLightPos   (TEXTURE_NONE),
  staticLightSceneSet(false)
{
  lightDataArray[0].color = vec3(1, 0.7f, 0.2f);
  lightDataArray[1].color = vec3(0.8f, 1, 0.9f);
  lightDataArray[2].color = vec3(1, 0.2f, 0.1f);

  lightDataArray[0].size = 0.0f;
  lightDataArray[1].size = 0.0f;
  lightDataArray[2].size = 0.0f;
}

///////////////////////////////////////////////////////////////////////////////
//
void subDivide(vec3 *&dest, const vec3 &v0, const vec3 &v1, const vec3 &v2, int level){
	if (level){
		vec3 v3 = normalize(v0 + v1);
		vec3 v4 = normalize(v1 + v2);
		vec3 v5 = normalize(v2 + v0);

		subDivide(dest, v0, v3, v5, level - 1);
		subDivide(dest, v3, v4, v5, level - 1);
		subDivide(dest, v3, v1, v4, level - 1);
		subDivide(dest, v5, v4, v2, level - 1);
	} else {
		*dest++ = v0;
		*dest++ = v1;
		*dest++ = v2;
	}
}

///////////////////////////////////////////////////////////////////////////////
//
void App::createSphereModel(){

  // Do not like this - allocating an array of vec3's but Model class just calls delete, not delete [] (or for the right type)
	vec3 *sphere = new vec3[SPHERE_SIZE];
	vec3 *dest = sphere;

	subDivide(dest, vec3(0, 1,0), vec3( 0,0, 1), vec3( 1,0, 0), SUBDIV_LEVEL);
	subDivide(dest, vec3(0, 1,0), vec3( 1,0, 0), vec3( 0,0,-1), SUBDIV_LEVEL);
	subDivide(dest, vec3(0, 1,0), vec3( 0,0,-1), vec3(-1,0, 0), SUBDIV_LEVEL);
	subDivide(dest, vec3(0, 1,0), vec3(-1,0, 0), vec3( 0,0, 1), SUBDIV_LEVEL);

	subDivide(dest, vec3(0,-1,0), vec3( 1,0, 0), vec3( 0,0, 1), SUBDIV_LEVEL);
	subDivide(dest, vec3(0,-1,0), vec3( 0,0, 1), vec3(-1,0, 0), SUBDIV_LEVEL);
	subDivide(dest, vec3(0,-1,0), vec3(-1,0, 0), vec3( 0,0,-1), SUBDIV_LEVEL);
	subDivide(dest, vec3(0,-1,0), vec3( 0,0,-1), vec3( 1,0, 0), SUBDIV_LEVEL);

	sphereModel = new Model();

	sphereModel->addStream(TYPE_VERTEX, 3, SPHERE_SIZE, (float*)sphere, NULL, false);
  sphereModel->setIndexCount(SPHERE_SIZE);
  sphereModel->addBatch(0, SPHERE_SIZE);

  sphereModel->cleanUp();
}

///////////////////////////////////////////////////////////////////////////////
//
void App::resetCamera(){
	camPos = vec3(-557.0f, 135.0f, 5.8f);
	wx = 0.0634f;
  wy = -1.58f;
}

///////////////////////////////////////////////////////////////////////////////
//
void App::moveCamera(const vec3 &dir){
  vec3 newPos = camPos + dir * (speed * frameTime);

  vec3 point;
  const BTri *tri;
  if (bsp.intersects(camPos, newPos, &point, &tri)){
    newPos = point + tri->plane.xyz();
  }
  bsp.pushSphere(newPos, 30);

  camPos = newPos;
}

bool App::init(){
  map = new Model();
  if (!map->loadObj("../Models/Room6/Map.obj")){
    delete map;
    return false;
  }

  {
    Stream stream = map->getStream(map->findStream(TYPE_VERTEX));
    vec3 *vertices = (vec3 *) stream.vertices;
    uint *indices = stream.indices;
    for (uint i = 0; i < map->getIndexCount(); i += 3){
      bsp.addTriangle(vertices[indices[i]], vertices[indices[i + 1]], vertices[indices[i + 2]]);
    }
  }

	// Load the geometry
	horseModel = new Model();
/*	
  if (!horseModel->loadObj("../Models/Horse.obj")){
		ErrorMsg("Couldn't load model file");
		return false;
	}
	horseModel->flipComponents(0, 1, 2);
  horseModel->reverseWinding();
	horseModel->flipComponents(1, 1, 2);
  float scaleSize[3] = {3000.0f, 3000.0f, 3000.0f};
  horseModel->scale(0, scaleSize);

  horseModel->save("../Models/Horse.hmdl");
  //*/

  if (!horseModel->load("../Models/Horse.hmdl")){
		ErrorMsg("Couldn't load model file");
		return false;
	}
  //*/

  bsp.build();

  map->computeTangentSpace(true);
  map->cleanUp();
  map->changeAllGeneric(true);

  // Create the render sphere model
  createSphereModel();

  int tab = configDialog->addTab("Rendering");
  
  configDialog->addWidget(tab, animateLights = new CheckBox(0, 0, 350, 36, "Animate lights", true));
  configDialog->addWidget(tab, staticLightScene = new CheckBox(0, 30, 350, 36, "Set static light scene", false));

  configDialog->addWidget(tab, useDeferedLighting = new CheckBox(0, 60, 350, 36, "Use LI deferred lighting", true));
  configDialog->addWidget(tab, lightCountPerFragment = new DropDownList(0, 100, 350, 36));
  configDialog->addWidget(tab, useStencilMasking = new CheckBox(0, 140, 350, 36, "Use stencil volumes",  false));
  configDialog->addWidget(tab, useDepthBoundsTest = new CheckBox(0, 180, 350, 36, "Use depth bounds test",  true));

  configDialog->addWidget(tab, doPrecisionTest = new CheckBox(0, 240, 350, 36, "Precision Test",  false));

  // Select the rendering tab as the active tab
  configDialog->setCurrentTab(tab);

  // Update the PFX moving lights into a stable starting condition
  // (spool up PFX)
  for(uint i=0; i<240; i++)
  {
    updateLights(1.0f/30.0f);
  }

  return true;
}

///////////////////////////////////////////////////////////////////////////////
//
void App::exit(){
  delete map;
  delete sphereModel;
  delete horseModel;
}

///////////////////////////////////////////////////////////////////////////////
//
void App::onSize(const int w, const int h){
  OpenGLApp::onSize(w, h);

  if (renderer){
    // Make sure render targets are the size of the window
    renderer->resizeRenderTarget(lightIndexBuffer, w, h, 1, 1);
    renderer->resizeRenderTarget(depthRT, w, h, 1, 1);
  }
}

///////////////////////////////////////////////////////////////////////////////
//
bool App::onKey(const uint key, const bool pressed)
{
  // If in editor mode
  if(onKeyEditor(key, pressed)){
    return true;
  }

  // Toggle deffered lighting
  if(key == KEY_D && pressed)
  {
    useDeferedLighting->setChecked(!useDeferedLighting->isChecked());
    return true;
  }

  return OpenGLApp::onKey(key, pressed);
}

///////////////////////////////////////////////////////////////////////////////
//
bool App::onMouseMove(const int x, const int y, const int deltaX, const int deltaY){

  // If in editor mode
  if(onMouseMoveEditor(x, y, deltaX,deltaY)){
    return true;
  }

  return OpenGLApp::onMouseMove(x, y, deltaX,deltaY);
}

///////////////////////////////////////////////////////////////////////////////
//
bool App::onMouseButton(const int x, const int y, const MouseButton button, const bool pressed){

  // If in editor mode
  if(onMouseButtonEditor(x, y, button, pressed)){
    return true;
  }

  return OpenGLApp::onMouseButton(x, y, button, pressed);
}

///////////////////////////////////////////////////////////////////////////////
//
bool App::onMouseWheel(const int x, const int y, const int scroll){

  // If in editor mode
  if(onMouseWheelEditor(x, y, scroll)){
    return true;
  }

  return OpenGLApp::onMouseWheel(x, y, scroll);
}

///////////////////////////////////////////////////////////////////////////////
//
bool App::load(){

  if (!GLSL_supported){
    ErrorMsg("No GLSL support");
    return false;
  }

  if (!GL_EXT_framebuffer_object_supported){
    ErrorMsg("No FBO support");
    return false;
  }

  if (!map->makeDrawable(renderer)) return false;
  if (!sphereModel->makeDrawable(renderer)) return false;
  if (!horseModel->makeDrawable(renderer)) return false;

  // Samplerstates
  if ((trilinearAniso = renderer->addSamplerState(TRILINEAR_ANISO, WRAP, WRAP, WRAP)) == SS_NONE) return false;
  if ((linearWrap = renderer->addSamplerState(LINEAR, WRAP, WRAP, WRAP)) == SS_NONE) return false;
  
  if ((pointClamp = renderer->addSamplerState(NEAREST, CLAMP, CLAMP, CLAMP)) == SS_NONE) return false;

  // FBO
  int fboDepthBits = 24;

  // Create the light direction buffers
  if ((lightIndexBuffer = renderer->addRenderTarget(width, height, FORMAT_RGBA8, pointClamp)) == TEXTURE_NONE) return false;
  if ((depthRT = renderer->addRenderDepth(width, height, fboDepthBits)) == TEXTURE_NONE) return false;

  // Shaders
  const char *attribs[] = { NULL, "textureCoord", "tangent", "binormal", "normal" };
  if ((plainTex = renderer->addShader("plainTex.shd", attribs, elementsOf(attribs))) == SHADER_NONE) return false;
  if ((plainColor = renderer->addShader("plainColor.shd")) == SHADER_NONE) return false;
  if ((depthOnly = renderer->addShader("depthOnly.shd")) == SHADER_NONE) return false;

  if ((lightingMP = renderer->addShader("lightingMP.shd", attribs, elementsOf(attribs))) == SHADER_NONE) return false;
  if ((lightingMP_ambient = renderer->addShader("lightingMP_ambient.shd", attribs, elementsOf(attribs))) == SHADER_NONE) return false;

  if ((lightingMP_stone = renderer->addShader("lightingMP_stone.shd")) == SHADER_NONE) return false;
  if ((lightingMP_stone_ambient = renderer->addShader("lightingMP_stone_ambient.shd")) == SHADER_NONE) return false;
  
  // Depth only pass for main view
  if ((lightingColorOnly = renderer->addShader("lightingColorOnly.shd")) == SHADER_NONE) return false;
  if ((lightingColorOnly_depthClamp = renderer->addShader("lightingColorOnly.shd", "#define CLAMP_DEPTH 1\n")) == SHADER_NONE) return false;
  
  if ((lightingLIDefer[LCPF_One] = renderer->addShader("lightingLIDefer.shd", attribs, elementsOf(attribs), "#define OVERLAP_LIGHTS 1\n")) == SHADER_NONE) return false;
  if ((lightingLIDefer[LCPF_Two] = renderer->addShader("lightingLIDefer.shd", attribs, elementsOf(attribs), "#define OVERLAP_LIGHTS 2\n")) == SHADER_NONE) return false;
  if ((lightingLIDefer[LCPF_Three] = renderer->addShader("lightingLIDefer.shd", attribs, elementsOf(attribs), "#define OVERLAP_LIGHTS 3\n")) == SHADER_NONE) return false;

  // Some shader limited cards cannot compile 4 lights per fragment
  if ((lightingLIDefer[LCPF_Four] = renderer->addShader("lightingLIDefer.shd", attribs, elementsOf(attribs), "#define OVERLAP_LIGHTS 4\n", ALLOW_FAILURE)) == SHADER_NONE)
  {
    lightingLIDefer[LCPF_Four] = lightingLIDefer[LCPF_Three];
  }

  if ((lightingLIDefer_stone[LCPF_One] = renderer->addShader("lightingLIDefer_stone.shd", "#define OVERLAP_LIGHTS 1\n")) == SHADER_NONE) return false;
  if ((lightingLIDefer_stone[LCPF_Two] = renderer->addShader("lightingLIDefer_stone.shd", "#define OVERLAP_LIGHTS 2\n")) == SHADER_NONE) return false;
  if ((lightingLIDefer_stone[LCPF_Three] = renderer->addShader("lightingLIDefer_stone.shd", "#define OVERLAP_LIGHTS 3\n")) == SHADER_NONE) return false;

  // Some shader limited cards cannot compile 4 lights per fragment
  if ((lightingLIDefer_stone[LCPF_Four] = renderer->addShader("lightingLIDefer_stone.shd", "#define OVERLAP_LIGHTS 4\n", ALLOW_FAILURE)) == SHADER_NONE)
  {
    lightingLIDefer_stone[LCPF_Four] = lightingLIDefer_stone[LCPF_Three];
  }

  if ((cmpTex = renderer->addShader("compareTex.shd")) == SHADER_NONE) return false;

  // Textures
  if ((base[0] = renderer->addTexture  ("../Textures/floor_wood_3.dds",                   true, trilinearAniso)) == SHADER_NONE) return false;
  if ((bump[0] = renderer->addNormalMap("../Textures/floor_wood_3Bump.dds", FORMAT_RGBA8, true, trilinearAniso)) == SHADER_NONE) return false;
  parallax[0] = 0.0f;

  if ((base[1] = renderer->addTexture  ("../Textures/brick01.dds",                   true, trilinearAniso)) == SHADER_NONE) return false;
  if ((bump[1] = renderer->addNormalMap("../Textures/brick01Bump.dds", FORMAT_RGBA8, true, trilinearAniso)) == SHADER_NONE) return false;
  parallax[1] = 0.04f;

  if ((base[2] = renderer->addTexture  ("../Textures/stone08.dds",                   true, trilinearAniso)) == SHADER_NONE) return false;
  if ((bump[2] = renderer->addNormalMap("../Textures/stone08Bump.dds", FORMAT_RGBA8, true, trilinearAniso)) == SHADER_NONE) return false;
  parallax[2] = 0.0f;

  if ((base[3] = renderer->addTexture  ("../Textures/StoneWall_1-4.dds",                   true, trilinearAniso)) == SHADER_NONE) return false;
  if ((bump[3] = renderer->addNormalMap("../Textures/StoneWall_1-4Bump.dds", FORMAT_RGBA8, true, trilinearAniso)) == SHADER_NONE) return false;
  parallax[3] = 0.03f;

  if ((light = renderer->addTexture("../Textures/spot.dds", false, linearClamp)) == SHADER_NONE) return false;

  if ((noise3D = renderer->addTexture("../Textures/NoiseVolume.dds", true, linearWrap)) == SHADER_NONE) return false;
  
  // Reset the bitmask texture lookups
  bitMaskLightColors = TEXTURE_NONE;
  bitMaskLightPos    = TEXTURE_NONE;

  // Blendstates
  if ((blendAdd = renderer->addBlendState(ONE, ONE)) == BS_NONE) return false;
  if ((blendCopy = renderer->addBlendState(ONE, ZERO)) == BS_NONE) return false;
  if ((blendTwoLightRender = renderer->addBlendStateSeperate(DST_ALPHA, ONE_MINUS_DST_ALPHA, ONE, ZERO)) == BS_NONE) return false;
  if ((blendBitShift = renderer->addBlendState(ONE, GL_CONSTANT_COLOR)) == BS_NONE) return false;
  if ((blendMax = renderer->addBlendState(ONE, ONE, BM_MAX)) == BS_NONE) return false;

  if ((noColorWrite = renderer->addBlendState(ONE, ZERO, BM_ADD, NONE)) == BS_NONE) return false;
  if ((depthNoWritePassGreater = renderer->addDepthState(true,  false, GEQUAL)) == BS_NONE) return false;
  
  // Do not supprt bounds test option if extension is not supported
  if(!GL_EXT_depth_bounds_test_supported){

    useDepthBoundsTest->setChecked(false);
    useDepthBoundsTest->setEnabled(false);

    //Use stencil masking as a fallback
    useStencilMasking->setChecked(true);
  }
  else{

    useDepthBoundsTest->setChecked(true);
    useDepthBoundsTest->setEnabled(true);
  }

  // Set the values for lights per fragment
  lightCountPerFragment->clear();
  lightCountPerFragment->addItemUnique("1 Light per fragment");
  lightCountPerFragment->addItemUnique("2 Lights per fragment");
  lightCountPerFragment->addItemUnique("3 Lights per fragment");
  lightCountPerFragment->selectItem(LCPF_Three);

  // If 4 lights per fragment is supported
  if(lightingLIDefer[LCPF_Four] != lightingLIDefer[LCPF_Three] &&
     lightingLIDefer_stone[LCPF_Four] != lightingLIDefer_stone[LCPF_Three])
  {
    lightCountPerFragment->addItemUnique("4 Lights per fragment");
    lightCountPerFragment->selectItem(LCPF_Four);
  }

  // Huh? This is not already all 1's? (according to the spec? - Nvidia bug if main surface does not have stencil?)
  glStencilMask(0xFFFFFFFF); 

  return true;
}

///////////////////////////////////////////////////////////////////////////////
//
void App::unload(){
  
}

///////////////////////////////////////////////////////////////////////////////
//
void App::drawLightParticles(const vec3 &dx, const vec3 &dy){

  renderer->reset();
  renderer->setShader(plainTex);
  renderer->setBlendState(blendAdd);
  renderer->setTexture("Base", light);
  renderer->setDepthState(noDepthWrite);
  renderer->apply();

  glBegin(GL_QUADS);
  for (uint i = 0; i < MAX_LIGHT_TOTAL; i++){

   if(lightDataArray[i].isEnabled){

    glColor3fv(lightDataArray[i].color);

    float renderSize = lightDataArray[i].size / 10.0f;

    glTexCoord2f(0, 0);
    glVertex3fv(lightDataArray[i].position - renderSize * dx + renderSize * dy);

    glTexCoord2f(1, 0);
    glVertex3fv(lightDataArray[i].position + renderSize * dx + renderSize * dy);

    glTexCoord2f(1, 1);
    glVertex3fv(lightDataArray[i].position + renderSize * dx - renderSize * dy);

    glTexCoord2f(0, 1);
    glVertex3fv(lightDataArray[i].position - renderSize * dx - renderSize * dy);

   }
  }
  glEnd();
}

///////////////////////////////////////////////////////////////////////////////
//
void App::updateBitMaskedLightTextures()
{
  // If the color texture does not exist, create it now
  if(bitMaskLightColors == TEXTURE_NONE) {
    Image loadData;
    loadData.create(FORMAT_RGBA8, MAX_LIGHT_TOTAL + 1, 1, 1, 1);

    unsigned char *dstData = loadData.getPixels();

    // Set the zero entry to black (no light)
    dstData[0] = 0;
    dstData[1] = 0;
    dstData[2] = 0;
    dstData[3] = 0;
    dstData += 4;

    for (int i =0; i<MAX_LIGHT_TOTAL; i++){
      dstData[0] = (unsigned char)min((lightDataArray[i].color.x * 256.0f), 255.0f);
      dstData[1] = (unsigned char)min((lightDataArray[i].color.y * 256.0f), 255.0f);
      dstData[2] = (unsigned char)min((lightDataArray[i].color.z * 256.0f), 255.0f);
      dstData[3] = 0;

      dstData += 4;
    }

    // Load in the new texture
    bitMaskLightColors = renderer->addTexture(loadData, pointClamp);
  }

  // TODO: Test if doing a image-sub is faster - Use PBO's?
  // Delete the old lightPos texture
  if(bitMaskLightPos != TEXTURE_NONE){
    renderer->removeTexture(bitMaskLightPos);
    bitMaskLightPos = TEXTURE_NONE;
  }

  // Upload a new texture
  {
    Image loadPosData;
    loadPosData.create(FORMAT_RGBA32F, MAX_LIGHT_TOTAL + 1, 1, 1, 1);

    float *dstData = (float*)loadPosData.getPixels();

    // Set the zero entry to black (no light)
    dstData[0] = 0.0f;
    dstData[1] = 0.0f;
    dstData[2] = 0.0f;
    dstData[3] = 0.0f;
    dstData += 4;

    for (int i =0; i<MAX_LIGHT_TOTAL; i++){
      // Move position into view space
      vec4 viewSpace = modelviewMatrix * vec4(lightDataArray[i].position, 1.0);

      dstData[0] = viewSpace.x;
      dstData[1] = viewSpace.y;
      dstData[2] = viewSpace.z;
      dstData[3] = 1.0f / lightDataArray[i].size;

      dstData += 4;
    }

    // Load in the new texture
    bitMaskLightPos = renderer->addTexture(loadPosData, pointClamp);
  }
}

///////////////////////////////////////////////////////////////////////////////
//
void App::drawDepthOnly()
{
  renderer->reset();
  renderer->setShader(depthOnly);
  renderer->setBlendState(noColorWrite);
  renderer->setRasterizerState(cullBack);
  renderer->apply();
 
  glClearStencil(0);
  glClear(GL_DEPTH_BUFFER_BIT | GL_STENCIL_BUFFER_BIT);

  map->draw(renderer);
  horseModel->draw(renderer);
}

///////////////////////////////////////////////////////////////////////////////
//
void App::drawLIDeferLight(GLubyte lightIndex, const vec3 &lightPosition, float lightSize){

  if(useDepthBoundsTest->isChecked()){

    vec4 diffVector = vec4(0.0f, 0.0f, lightSize, 0.0f);

    vec4 viewSpaceLightPos = modelviewMatrix * vec4(lightPosition, 1.0f);
    vec4 nearVec = projectionMatrix * (viewSpaceLightPos - diffVector);
    vec4 farVec  = projectionMatrix * (viewSpaceLightPos + diffVector);

    float nearVal = clamp(nearVec.z / nearVec.w, -1.0f, 1.0f) * 0.5f + 0.5f;
    if(nearVec.w <= 0.0f){
      nearVal = 0.0f; 
    }
    float farVal = clamp(farVec.z / farVec.w, -1.0f, 1.0f) * 0.5f + 0.5f;
    if(farVec.w <= 0.0f){
      farVal = 0.0f; 
    }

    // Sanity check
    if(nearVal > farVal)
    {
      nearVal = farVal;
    }

    glDepthBoundsEXT(nearVal, farVal);
  }

  // Convert the light count into 4 2bit values
  GLubyte convertColor = lightIndex;
  GLubyte redBit   = (convertColor & (0x3 << 0)) << 6; 
  GLubyte greenBit = (convertColor & (0x3 << 2)) << 4;
  GLubyte blueBit  = (convertColor & (0x3 << 4)) << 2;
  GLubyte alphaBit = (convertColor & (0x3 << 6)) << 0;

  //Set the light index color 
  // Convert the light values to floats and set as a constant?
  //glColor4ub(redBit, greenBit, blueBit, alphaBit);
  vec4 outColor( (float)redBit, (float)greenBit, (float)blueBit, (float)alphaBit);
  static float divisor = 255.0f;
  outColor = outColor / divisor;

  // Setup lightIndex, 1-lightIndex when not using bit packing
  if(lightCountPerFragment->getSelectedItem() < LCPF_Three){
    outColor =  vec4((float)lightIndex, (float)(255 - lightIndex), (float)lightIndex, (float)lightIndex);  
    outColor = outColor / divisor;
  }

  // Note: Should use a infinite view projection matrix and cull front faces
  renderer->setShaderConstant3f("lightPos", lightPosition);
  renderer->setShaderConstant1f("lightRadius", lightSize);
  renderer->setShaderConstant4f("outColor", outColor);
  renderer->applyConstants();

  renderer->changeRasterizerState(cullFront);

  if(useStencilMasking->isChecked()){

    // Set the stencil state to set the value on fail
    glStencilFunc(GL_ALWAYS, lightIndex, 0xFFFFFFFF);
    glStencilOp(GL_KEEP, GL_REPLACE, GL_KEEP); 

    // Disable color writes
    renderer->changeBlendState(noColorWrite);
    renderer->changeDepthState(noDepthWrite);

    // Draw a sphere the radius of the light
    sphereModel->draw(renderer);

    // Set the stencil to only pass on equal value
    glStencilFunc(GL_EQUAL, lightIndex, 0xFFFFFFFF);
    glStencilOp(GL_KEEP, GL_KEEP, GL_KEEP); 
    renderer->changeRasterizerState(cullBack);
  }

  // Set the bitshift blend state
  if(lightCountPerFragment->getSelectedItem() == LCPF_One){
    renderer->changeBlendState(blendCopy);
  }
  else if(lightCountPerFragment->getSelectedItem() == LCPF_Two){
    renderer->changeBlendState(blendMax);
  }
  else{
    renderer->changeBlendState(blendBitShift);
  }

  //TODO: Render lowers detail spheres when light is far away?
  // Draw a sphere the radius of the light
  sphereModel->draw(renderer);

}

///////////////////////////////////////////////////////////////////////////////
//
void App::drawLIDeferLights(){

  // Set target to render depth only 
  // (ATI does not sopport only rendering to a depth buffer? Depth buffer seems inverted when bound to another FBO)
  renderer->changeRenderTarget(lightIndexBuffer, depthRT);
  drawDepthOnly();

  //Clear the output color
  float clearColor[4] = {0.0f, 0.0f, 0.0f, 0.0f};
  renderer->clear(true, false, clearColor);

  ShaderID colorShaderID = lightingColorOnly;

  // Set the constant blend color to bit shift 2 bits down on each call
  glBlendColor(0.251f, 0.251f, 0.251f, 0.251f); 

  if(useStencilMasking->isChecked()){
    glEnable(GL_STENCIL_TEST);

    // Use a depth clamping shader if useing stencil masking
    colorShaderID = lightingColorOnly_depthClamp;
  }

  if(useDepthBoundsTest->isChecked()){
    glEnable(GL_DEPTH_BOUNDS_TEST_EXT);
  }

  renderer->reset();
  renderer->setShader(colorShaderID);
  renderer->setRasterizerState(cullFront);
  renderer->setDepthState(depthNoWritePassGreater);
  renderer->apply();
  
  // Loop for each light color to give each color an even chance of been visible
  //  Draw the primary lights last by iterating through the loop backwards
  for (int i = MAX_LIGHT_TOTAL - 1; i >= 0; i--){
    if(lightDataArray[i].isEnabled){
      drawLIDeferLight(i + 1, lightDataArray[i].position, lightDataArray[i].size);
    }
  }

  if(useDepthBoundsTest->isChecked()){
    glDisable(GL_DEPTH_BOUNDS_TEST_EXT);
  }
  if(useStencilMasking->isChecked()){
    glDisable(GL_STENCIL_TEST);
  }

  renderer->changeToMainFramebuffer();
}

///////////////////////////////////////////////////////////////////////////////
//
void App::drawLIDeferLitObjects()
{
  // Do a z-pre pass (this is wasteful as we already have a render target with the depth)
  renderer->changeDepthState(DS_NONE);
  drawDepthOnly();
  //glClearColor(1.0f, 1.0f, 1.0f, 1.0f);
  //glClear(GL_COLOR_BUFFER_BIT);  

  // Setup render states
  renderer->reset();
  renderer->setShader(lightingLIDefer[lightCountPerFragment->getSelectedItem()]);
  renderer->setRasterizerState(cullBack);
  renderer->setBlendState(blendCopy);
  renderer->setDepthState(noDepthWrite);
  renderer->setShaderConstant3f("camPos", camPos);
  renderer->apply();

  renderer->setTexture("BitPlane", lightIndexBuffer);
  renderer->setTexture("LightColorTex", bitMaskLightColors);
  renderer->setTexture("LightPosTex", bitMaskLightPos);

  //Loop for all pieces of geometry
  for (uint k = 0; k < 4; k++){
    renderer->setTexture("Base", base[k]);
    renderer->setTexture("Bump", bump[k]);
    renderer->applyTextures();

    renderer->setShaderConstant1i("hasParallax", int(parallax[k] > 0.0f));
    renderer->setShaderConstant2f("plxCoeffs", vec2(2, -1) * parallax[k]);
    renderer->applyConstants();

    map->drawBatch(renderer, k);
  }

  renderer->reset();
  renderer->setShader(lightingLIDefer_stone[lightCountPerFragment->getSelectedItem()]);
  renderer->setRasterizerState(cullBack);
  renderer->setBlendState(blendCopy);
  renderer->setDepthState(noDepthWrite);

  renderer->setTexture("Noise", noise3D);
  renderer->setTexture("BitPlane", lightIndexBuffer);
  renderer->setTexture("LightColorTex", bitMaskLightColors);
  renderer->setTexture("LightPosTex", bitMaskLightPos);
  renderer->apply();

  horseModel->draw(renderer);

}

///////////////////////////////////////////////////////////////////////////////
//
void App::drawLightingMPAmbient(){

  // Make sure depth writes are on
  renderer->changeDepthState(DS_NONE);
  glClear(GL_DEPTH_BUFFER_BIT | GL_STENCIL_BUFFER_BIT);

  renderer->reset();
  renderer->setShader(lightingMP_ambient);
  renderer->setRasterizerState(cullBack);
  renderer->apply();

  for (uint i = 0; i < 4; i++){
    renderer->setTexture("Base", base[i]);
    renderer->setTexture("Bump", bump[i]);
    renderer->applyTextures();

    renderer->setShaderConstant1i("hasParallax", int(parallax[i] > 0.0f));
    renderer->setShaderConstant2f("plxCoeffs", vec2(2, -1) * parallax[i]);
    renderer->setShaderConstant3f("camPos", camPos);
    renderer->applyConstants();

    map->drawBatch(renderer, i);
  }

  renderer->reset();
  renderer->setShader(lightingMP_stone_ambient);
  renderer->setRasterizerState(cullBack);
  renderer->setTexture("Noise", noise3D);
  renderer->apply();

  horseModel->draw(renderer);
}

///////////////////////////////////////////////////////////////////////////////
//
void App::drawLightingMP(){

  renderer->reset();
  renderer->setShader(lightingMP);
  renderer->setShaderConstant3f("camPos", camPos);
  renderer->setRasterizerState(cullBack);
  renderer->setBlendState(blendAdd);
  renderer->setDepthState(noDepthWrite);
  renderer->apply();

  for (uint k = 0; k < 4; k++){
    renderer->setTexture("Base", base[k]);
    renderer->setTexture("Bump", bump[k]);
    renderer->applyTextures();

    renderer->setShaderConstant1i("hasParallax", int(parallax[k] > 0.0f));
    renderer->setShaderConstant2f("plxCoeffs", vec2(2, -1) * parallax[k]);

    glEnable(GL_SCISSOR_TEST);

    for(uint i=0; i<MAX_LIGHT_TOTAL; i++){

      if(lightDataArray[i].isEnabled){
        glScissor(lightDataArray[i].screenX, lightDataArray[i].screenY, lightDataArray[i].screenWidth, lightDataArray[i].screenHeight);

        renderer->setShaderConstant3f("lightColor", lightDataArray[i].color);
        renderer->setShaderConstant3f("lightPos", lightDataArray[i].position);
        renderer->setShaderConstant1f("invRadius", 1.0f / lightDataArray[i].size);
        renderer->applyConstants();
    
        map->drawBatch(renderer, k);
      }
    }
    glDisable(GL_SCISSOR_TEST);
  }

  renderer->reset();
  renderer->setShader(lightingMP_stone);
  renderer->setRasterizerState(cullBack);
  renderer->setBlendState(blendAdd);
  renderer->setDepthState(noDepthWrite);
  renderer->setShaderConstant3f("camPos", camPos);
  renderer->setTexture("Noise", noise3D);
  renderer->apply();

  glEnable(GL_SCISSOR_TEST);
  for(uint i=0; i<MAX_LIGHT_TOTAL; i++)
  {
    if(lightDataArray[i].isEnabled)
    {
      glScissor(lightDataArray[i].screenX, lightDataArray[i].screenY, lightDataArray[i].screenWidth, lightDataArray[i].screenHeight);
    
      renderer->setShaderConstant3f("lightColor", lightDataArray[i].color);
      renderer->setShaderConstant3f("lightPos", lightDataArray[i].position);
      renderer->setShaderConstant1f("invRadius", 1.0f/lightDataArray[i].size);
      renderer->applyConstants();

      horseModel->draw(renderer);
    }
  }
  glDisable(GL_SCISSOR_TEST);

}

///////////////////////////////////////////////////////////////////////////////
//
void App::updateLightCull()
{
  // Update the PFX light culling
  for(uint i=0; i<MAX_LIGHT_TOTAL; i++)
  {
    if(lightDataArray[i].size > 0.0f){

      // Get if the light is visible on screen
      lightDataArray[i].isEnabled = getScissorRectangle(modelviewMatrix, lightDataArray[i].position, lightDataArray[i].size,
        1.5f, width, height, 
        &lightDataArray[i].screenX, 
        &lightDataArray[i].screenY, 
        &lightDataArray[i].screenWidth, 
        &lightDataArray[i].screenHeight);
    }
    else{
      lightDataArray[i].isEnabled = false;
    }
  }
}

///////////////////////////////////////////////////////////////////////////////
//
void App::drawFrame(){
  
  // Update and load the modelview and projection matrices
  projectionMatrix = perspectiveMatrixX(1.5f, width, height, 5, 4000);
  modelviewMatrix = rotateXY(-wx, -wy);
  modelviewMatrix.translate(-camPos);

  glMatrixMode(GL_PROJECTION);
  glLoadTransposeMatrixfARB(projectionMatrix);

  glMatrixMode(GL_MODELVIEW);
  glLoadTransposeMatrixfARB(modelviewMatrix);

  // If switching to the static light scene
  if(staticLightScene->isChecked() != staticLightSceneSet){

    staticLightSceneSet = staticLightScene->isChecked();
    if(staticLightSceneSet){
      SetStaticLightScene();
    }
  }

  // Update light positions if necessary
  if(animateLights->isChecked() && !staticLightScene->isChecked())
  {
    updateLights(frameTime);
  }

  // Cull the lights to the bounds of the screen
  updateLightCull();

  // Test if performing a precision test
  if(doPrecisionTest->isChecked())
  {
    drawPrecisionTest1();
  }
  else if(useDeferedLighting->isChecked())
  {
    // Update the light textures
    // TODO: Don't update if camera pos/rotation shas not changed 
    // (possibly use world space lights instead of camera space if lights do not move?)
    updateBitMaskedLightTextures();

    // Add light volumes
    drawLIDeferLights();

    // TODO: Find out why looking at the back wall 
    // is faster than looking across whole scene - even with no light - should be same fragment work...Fast depth Z not working?
    
    // Draw the lit objects - lighting using the deferred light indexes
    drawLIDeferLitObjects();

    drawLightParticles(modelviewMatrix.rows[0].xyz(), modelviewMatrix.rows[1].xyz());
  }
  else
  {
    // Render the lights using a forward render pass
    drawLightingMPAmbient();
    drawLightingMP();

    drawLightParticles(modelviewMatrix.rows[0].xyz(), modelviewMatrix.rows[1].xyz());
  }

  // Draw the editor data (if in editor mode)
  drawFrameEditor();
}

