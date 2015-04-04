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

#define PRIMARY_LIGHT_COUNT         3
#define SECONDARY_LIGHT_COUNT       (MAX_LIGHT_TOTAL - PRIMARY_LIGHT_COUNT)
#define SECONDARY_LIGHT_LIFETIME    8.0f
#define SECONDARY_LIGHT_SPAWNRATE   (SECONDARY_LIGHT_LIFETIME / (float)SECONDARY_LIGHT_COUNT)


GLint 
gluUnProject(GLdouble winx, GLdouble winy, GLdouble winz,
	     const GLdouble model[16], const GLdouble proj[16],
	     const GLint viewport[4],
	     GLdouble * objx, GLdouble * objy, GLdouble * objz);

// Structure containing all the editor only data
struct EditorData
{
  EditorData();

  bool isEditorMode;
  bool allowOverlappingLights;

  int  lightIndex;
  vec3 lightColor;
  float lightSize;
  vec3 lightPosition;
};
EditorData editorData;


//Structure to hold PFX light data
struct PPFXLightData
{
  PPFXLightData();
  void Spawn(const vec3 &spawnPos, float intialAge, LightData &setLight);
  void Update(float updateTime, const BSP &collideBSP, LightData &setLight);

  vec3 spawnPosition; // The spawn/intersection position
  vec3 direction;     // The current direction

  float lifeTime;     // The lifetime of the particle
  float dirLifeTime;  // The life of the particle in the current direction
};
uint nextPFXLightEnable = 0;
PPFXLightData pfxLights[SECONDARY_LIGHT_COUNT];

// Define the arry of static light data positions
LightData staticLightDataArray[MAX_LIGHT_TOTAL] = { 
#include "LightPositions.h"
};

///////////////////////////////////////////////////////////////////////////////
//
EditorData::EditorData() :
  isEditorMode(false),
  allowOverlappingLights(true),
  lightIndex(0),
  lightSize(50.0f)
{
  lightColor = vec3(1.0f, 1.0f, 0.0f);
  lightPosition = vec3(0.0f);
}

///////////////////////////////////////////////////////////////////////////////
//
void ConvertMatrix(const mat4 &srcMat, GLdouble outMat[16])
{
  mat4 srcTranspose = transpose(srcMat);

  // Loop and convert to double format
  const float * srcData = (const float*)srcTranspose.rows;
  for(int i=0; i< 16; i++){
    outMat[i] = srcData[i];
  }
}

///////////////////////////////////////////////////////////////////////////////
//
void App::SetStaticLightScene()
{
  // Copy over the fixed light positions
  for(uint i =0; i<MAX_LIGHT_TOTAL; i++)
  {
    lightDataArray[i] = staticLightDataArray[i];
  }
}

///////////////////////////////////////////////////////////////////////////////
//
bool App::GetSpherePosition(const int x, const int y){

  vec3 newPos;
  vec3 targetPos;

  // Calculate the screen target position
  {
    GLdouble modelview[16];
    GLdouble projection[16];
    GLint viewport[4] ={0 , 0, width, height};

    GLdouble posX, posY, posZ;

    ConvertMatrix(projectionMatrix, projection);
    ConvertMatrix(modelviewMatrix, modelview);

    float srcZ = 1.0f;
	  glReadPixels( x, height - y, 1, 1, GL_DEPTH_COMPONENT, GL_FLOAT, &srcZ );

    gluUnProject((GLdouble)x, (GLdouble)height - (GLdouble)y, srcZ, //1.0,
         modelview, projection,
	       viewport,
	       &posX, &posY, &posZ);

    targetPos = vec3((float)posX, (float)posY, (float)posZ);
  }

  // Get the new position as slightly offset from the surface intersection
  newPos = targetPos - (normalize(targetPos - camPos) * editorData.lightSize * 0.3f);

  /*
  // Check for a collision between the last and new position
  vec3 colPoint;
  const BTri *colTriangle;
  if(bsp.intersects(camPos, targetPos, &colPoint, &colTriangle)){

    // Don't land exactly on the plane, 
    newPos = colPoint + 10.0f * colTriangle->plane.xyz();
  }
  else{
    return false;
  }
*/

  // If no allowing overlapping spheres
  if(!editorData.allowOverlappingLights){

    vec3 dirVector = normalize(newPos - camPos);
    float newPosDist = dot(newPos - camPos, dirVector);

    // Get the collision against all the current spheres (except the current light index)
    for(uint i=0; i<MAX_LIGHT_TOTAL; i++){

      if(i != editorData.lightIndex){

        // This is a lazy approximation of sphere line intersection
        float projDist = dot(lightDataArray[i].position - camPos, dirVector);
        if(length((dirVector * projDist) - (lightDataArray[i].position - camPos)) < lightDataArray[i].size){

          newPosDist = min(projDist - lightDataArray[i].size - editorData.lightSize, newPosDist);
        }
      }
    }

    // If the sphere cannot be placed, return false
    if(newPosDist < 0.0f){
      return false;
    }

    // Assign the new position
    newPos = (dirVector * newPosDist) + camPos;
  }

  // Assign the editor data new position
  editorData.lightPosition = newPos;

  return true;
}

///////////////////////////////////////////////////////////////////////////////
//
bool App::onKeyEditor(const uint key, const bool pressed) {

  // Toggle in/out of editor mode
  if(key == KEY_E && pressed){
    editorData.isEditorMode = !editorData.isEditorMode;

    // If in editor mode, disable animating lights
    if(editorData.isEditorMode)
    {
      animateLights->setChecked(false);
    }
  }

  // If not in editor mode, return now
  if(!editorData.isEditorMode){
    return false;
  }

  // Toggle if overlapping lights are allowed
  if(key == KEY_O && pressed){
    editorData.allowOverlappingLights = !editorData.allowOverlappingLights;
  }

  // Dump out all the current light data
  if(key == KEY_D && pressed){
    for(uint i=0; i<MAX_LIGHT_TOTAL; i++){
      printf("  LightData(vec3(%ff,%ff,%ff), vec3(%ff,%ff,%ff), %ff), \n", 
            lightDataArray[i].color.x, lightDataArray[i].color.y, lightDataArray[i].color.z,
            lightDataArray[i].position.x, lightDataArray[i].position.y, lightDataArray[i].position.z,
            lightDataArray[i].size);
    }
  }

  // Change the placement light index 
  if(key == KEY_ADD && pressed){
    editorData.lightIndex = (editorData.lightIndex + 1) % MAX_LIGHT_TOTAL;
  }
  if(key == KEY_SUBTRACT && pressed){
    editorData.lightIndex--;
    if(editorData.lightIndex < 0)
    {
      editorData.lightIndex = MAX_LIGHT_TOTAL - 1;
    }
  }

  // Change the placement light color
  if(key == KEY_NUMPAD7 && pressed){
    editorData.lightColor.x += 0.1f;
  }
  if(key == KEY_NUMPAD4 && pressed){
    editorData.lightColor.x -= 0.1f;
  }

  if(key == KEY_NUMPAD8 && pressed){
    editorData.lightColor.y += 0.1f;
  }
  if(key == KEY_NUMPAD5 && pressed){
    editorData.lightColor.y -= 0.1f;
  }

  if(key == KEY_NUMPAD9 && pressed){
    editorData.lightColor.z += 0.1f;
  }
  if(key == KEY_NUMPAD6 && pressed){
    editorData.lightColor.z -= 0.1f;
  }
  editorData.lightColor = clamp(editorData.lightColor, 0.0f, 1.0f);

  return true;
}

///////////////////////////////////////////////////////////////////////////////
//
bool App::onMouseWheelEditor(const int x, const int y, const int scroll){
  // If not in editor mode, return now
  if(!editorData.isEditorMode){
    return false;
  }

  // Adjust the sphere placement size
  editorData.lightSize = clamp(editorData.lightSize + (float)scroll, 0.0f, 700.0f);

  return true;
}


///////////////////////////////////////////////////////////////////////////////
//
bool App::onMouseButtonEditor(const int x, const int y, const MouseButton button, const bool pressed){
  // If not in editor mode, return now
  if(!editorData.isEditorMode || !pressed || button != MOUSE_LEFT){
    return false;
  }

  // Place the light sphere
  if(GetSpherePosition(x,y)){
    lightDataArray[editorData.lightIndex].position = editorData.lightPosition;
    lightDataArray[editorData.lightIndex].color = editorData.lightColor;
    lightDataArray[editorData.lightIndex].size = editorData.lightSize;
  }

  // Reset the deferred rendering light color texture to force a re-generation
  if(bitMaskLightColors != TEXTURE_NONE){
    renderer->removeTexture(bitMaskLightColors);
    bitMaskLightColors = TEXTURE_NONE;
  }

  return true;
}

///////////////////////////////////////////////////////////////////////////////
//
bool App::onMouseMoveEditor(const int x, const int y, const int deltaX, const int deltaY){

  // If not in editor mode, return now
  if(!editorData.isEditorMode){
    return false;
  }

  // Get the new sphere position
  GetSpherePosition(x,y);

  return true;
}

///////////////////////////////////////////////////////////////////////////////
//
void App::drawFrameEditor(){

  // If not in editor mode, return now
  if(!editorData.isEditorMode){
    return;
  }

  // Draw the light sphere volumes into the scene
  renderer->reset();
  renderer->setShader(lightingColorOnly);
  renderer->setBlendState(blendAdd);
  renderer->setRasterizerState(cullNone);
  renderer->setDepthState(noDepthWrite);
  renderer->apply();

  renderer->setShaderConstant3f("lightPos", editorData.lightPosition);
  renderer->setShaderConstant1f("lightRadius", editorData.lightSize);
  renderer->setShaderConstant4f("outColor", vec4(editorData.lightColor * 0.5f, 0.5f));
  renderer->applyConstants();

  // Draw a sphere the radius of the light
  sphereModel->draw(renderer);

  // Draw text data to the screen 
	renderer->setup2DMode(0, (float) width, 0, (float) height);

  // Draw the current light index to the screen
	char str[512];
  int offset = 0;

  sprintf(str, "Allow overlapping %d", (int)editorData.allowOverlappingLights);
	renderer->drawText(str, (float)width - (14 * 30) - 8, 8.0f + offset, 30, 38, defaultFont, linearClamp, blendSrcAlpha, noDepthTest);

  offset += 38;
  sprintf(str, "LightIndex %d", editorData.lightIndex);
	renderer->drawText(str, (float)width - (14 * 30) - 8, 8.0f + offset, 30, 38, defaultFont, linearClamp, blendSrcAlpha, noDepthTest);

  offset += 38;
  sprintf(str, "LightSize %f", editorData.lightSize);
	renderer->drawText(str, (float)width - (14 * 30) - 8, 8.0f + offset, 30, 38, defaultFont, linearClamp, blendSrcAlpha, noDepthTest);

  offset += 38;
  sprintf(str, "LightColor (%.2f, %.2f, %.2f)", editorData.lightColor.x, editorData.lightColor.y, editorData.lightColor.z);
	renderer->drawText(str, (float)width - (14 * 30) - 8, 8.0f + offset, 30, 38, defaultFont, linearClamp, blendSrcAlpha, noDepthTest);
}

///////////////////////////////////////////////////////////////////////////////
//
PPFXLightData::PPFXLightData():
  lifeTime(0.0f),
  dirLifeTime(0.0f)
{
  spawnPosition = vec3(0.0f);
  direction = vec3(0.0f);
}

///////////////////////////////////////////////////////////////////////////////
//
void PPFXLightData::Spawn(const vec3 &spawnPos, float intialAge, LightData &setLight){

  spawnPosition = spawnPos;
  setLight.position = spawnPosition;

  // Spawn in a random directoin
  direction = vec3(2.0f * float(rand()) / RAND_MAX - 1.0f,
                   2.0f * float(rand()) / RAND_MAX - 1.0f,
                   2.0f * float(rand()) / RAND_MAX - 1.0f);
 
  direction = normalize(direction) *  500.0f;
  
  setLight.size = 75.0f;
  lifeTime    = intialAge;
  dirLifeTime = intialAge;
}

///////////////////////////////////////////////////////////////////////////////
//
void PPFXLightData::Update(float updateTime, const BSP &collideBSP, LightData &setLight){

  // Update particle lifetimes
  lifeTime    += updateTime;
  dirLifeTime += updateTime;

/*
  // Attenuate size over time
  if(lifeTime > (SECONDARY_LIGHT_LIFETIME*0.75f)){
    setLight.size = 75.0f * (1.0f - (lifeTime - (SECONDARY_LIGHT_LIFETIME*0.75f)) / (SECONDARY_LIGHT_LIFETIME*0.25f));
  }
  else{
    setLight.size = 75.0f; 
  }
*/

  // Calculate the new position using s = ut + 0.5at^2
  vec3 newPosition = spawnPosition + (direction * dirLifeTime) + (vec3(0.0f, -100.0f, 0.0f) * dirLifeTime * dirLifeTime * 0.5f);

  // Check for a collision between the last and new position
  vec3 colPoint;
  const BTri *colTriangle;
  if(collideBSP.intersects(setLight.position, newPosition, &colPoint, &colTriangle))
  {
    // Don't land exactly on the plane, 
    spawnPosition = colPoint + 10.0f * colTriangle->plane.xyz();

    // Calculate the new direction, with 95% of the origional velocity
    direction = reflect(direction, colTriangle->plane.xyz()); 
    direction *= 0.95f;
    dirLifeTime = 0.0f;

    // Don't worry about calculating the directional 
    newPosition = spawnPosition;
  }

  // Assign the new position
  setLight.position = newPosition;
}

///////////////////////////////////////////////////////////////////////////////
//
void App::updatePrimaryLights(float t){

  float c = 0.5f + 0.5f * sinf(t * 0.723f);

  lightDataArray[0].size = 600.0f;
  lightDataArray[1].size = 600.0f;
  lightDataArray[2].size = 600.0f;

  // Set primary light positions along a pre-programmed track
  lightDataArray[1].position = vec3(350 * cosf(1.82345f * t), 300 * cosf(1.252f * t), 180 * sinf(2.451f * t) - 1300);
  lightDataArray[2].position = vec3(85 - 250 * c * sinf(t * 2 * 0.723f), 400 * sinf(t * 0.723f) - 320, 150 * c * sinf(t * 3 * 0.723f) - 115);

  float f = fmodf(0.7f * t, 4.0f);
  float cf = cosf(PI * f);
  if (f < 2){
    if (f < 1){
      lightDataArray[0].position = float3(720 * cf, 0, 720);
    } else {
      lightDataArray[0].position = float3(-720, 0, -720 * cf);
    }
  } else {
    if (f < 3){
      lightDataArray[0].position = float3(-720 * cf, 0, -720);
    } else {
      lightDataArray[0].position = float3(720, 0, 720 * cf);
    }
  }

}

///////////////////////////////////////////////////////////////////////////////
//
void App::updateLights(float updateTime){

  static float animateTime = 0.0f;
  static float spawnTime = 0.0f;
  static float spawnDelta = 0.0f;

  spawnDelta += updateTime;
  animateTime += updateTime;

  // Loop for each spawn step
  while (spawnDelta > SECONDARY_LIGHT_SPAWNRATE)
  {
    spawnDelta -= SECONDARY_LIGHT_SPAWNRATE;
    spawnTime += SECONDARY_LIGHT_SPAWNRATE;

    // Update the spawn point positions
    updatePrimaryLights(spawnTime);

    // Spawn a particle at the new position of the specified age
    pfxLights[nextPFXLightEnable].Spawn(
        lightDataArray[nextPFXLightEnable%3].position,
         animateTime - spawnTime, 
         lightDataArray[nextPFXLightEnable + PRIMARY_LIGHT_COUNT]);

    // Get the next available PFX spawn light position
    nextPFXLightEnable = (nextPFXLightEnable + 1) % SECONDARY_LIGHT_COUNT;
  }

  // Set the final primary light positions
  // (Should really do this for each spawn step)
  updatePrimaryLights(animateTime);

  // Update the PFX light positions for each tick
  for(uint i=0; i<SECONDARY_LIGHT_COUNT; i++)
  {
    pfxLights[i].Update(updateTime, bsp, lightDataArray[i + PRIMARY_LIGHT_COUNT]);
  }

}

///////////////////////////////////////////////////////////////////////////////
//
void App::drawPrecisionTest1(){

  renderer->setup2DMode(0, 1.0f, 1.0f, 0.0);

  renderer->changeRenderTarget(lightIndexBuffer);

  float clearColor[4] = {0.0f, 0.0f, 0.0f, 0.0f};

  //Clear the output color
  renderer->clear(true, false, clearColor);

  renderer->reset();
  renderer->setShader(plainColor);
  renderer->setBlendState(blendCopy);
  renderer->setDepthState(noDepthTest);
  renderer->apply();

  glBegin(GL_QUADS);

  float sizeOffset =  1.0f / 16.0f;

  // Loop for x direction
  for(uint x=0; x<16; x++){

    // Loop for Y direction
    for(uint y=0; y<16; y++){

      // Draw a quad of the required color
      glColor3f((float)(y*16 + x) / 255.0f, 0.0f, 0.0f);

      float baseX = (float)x * sizeOffset; 
      float baseY = (float)y * sizeOffset; 

      glVertex3f(baseX, baseY, 0.5f);
      glVertex3f(baseX + sizeOffset, baseY, 0.5f);
      glVertex3f(baseX + sizeOffset, baseY + sizeOffset, 0.5f);
      glVertex3f(baseX, baseY + sizeOffset, 0.5f);
    }
  }

  glEnd();

  renderer->changeToMainFramebuffer();

  // Bind the precision test shader
  renderer->reset();
  renderer->setShader(cmpTex);
  renderer->setBlendState(blendCopy);
  renderer->setDepthState(noDepthTest);
  renderer->setTexture("BaseTex", lightIndexBuffer);
  renderer->apply();

  // Loop for x direction
  for(uint x=0; x<16; x++){

    // Loop for Y direction
    for(uint y=0; y<16; y++){

      // Set the compare limits
      renderer->setShaderConstant2f("cmpLimits", vec2((float)(y*16 + x), (float)(y*16 + x + 1)));
      renderer->applyConstants();

      float baseX = (float)x * sizeOffset; 
      float baseY = (float)y * sizeOffset; 

      glBegin(GL_QUADS);

      glTexCoord2f(baseX, baseY);
      glVertex3f(baseX, baseY, 0.5f);

      glTexCoord2f(baseX + sizeOffset, baseY);
      glVertex3f(baseX + sizeOffset, baseY, 0.5f);

      glTexCoord2f(baseX + sizeOffset, baseY + sizeOffset);
      glVertex3f(baseX + sizeOffset, baseY + sizeOffset, 0.5f);

      glTexCoord2f(baseX, baseY + sizeOffset);
      glVertex3f(baseX, baseY + sizeOffset, 0.5f);

      glEnd();
    }
  }
}

