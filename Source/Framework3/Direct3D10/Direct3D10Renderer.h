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

#ifndef _DIRECT3D10RENDERER_H_
#define _DIRECT3D10RENDERER_H_


#include "../Renderer.h"
#include <d3d10.h>
//#include <d3dx10.h>

#define SAFE_RELEASE(p) { if (p){ p->Release(); p = NULL; } }

#define ROLLING_VB_SIZE (64 * 1024)

/*
#define VB_INVALID (-2)
*/


class Direct3D10Renderer : public Renderer {
public:
	Direct3D10Renderer(ID3D10Device *d3ddev);
	~Direct3D10Renderer();

	void resetToDefaults();
	void reset(const uint flags = RESET_ALL);
	void setD3Ddefaults();

//	bool resetDevice();

	TextureID addTexture(Image &img, const SamplerStateID samplerState = SS_NONE, uint flags = 0);

	TextureID addRenderTarget(const int width, const int height, const int depth, const int arraySize, const FORMAT format, const int msaaSamples = 1, const SamplerStateID samplerState = SS_NONE, uint flags = 0);
	TextureID addRenderDepth(const int width, const int height, const int arraySize, const FORMAT format, const int msaaSamples = 1, const SamplerStateID samplerState = SS_NONE, uint flags = 0);

	bool resizeRenderTarget(const TextureID renderTarget, const int width, const int height, const int depth, const int arraySize);

	void removeTexture(const TextureID texture);

	ShaderID addShader(const char *vsText, const char *gsText, const char *fsText, const int vsLine, const int gsLine, const int fsLine,
		const char *header = NULL, const char *extra = NULL, const char *fileName = NULL, const char **attributeNames = NULL, const int nAttributes = 0, const uint flags = 0);
	VertexFormatID addVertexFormat(const FormatDesc *formatDesc, const uint nAttribs, const ShaderID shader = SHADER_NONE);
	VertexBufferID addVertexBuffer(const long size, const BufferAccess bufferAccess, const void *data = NULL);
	IndexBufferID addIndexBuffer(const uint nIndices, const uint indexSize, const BufferAccess bufferAccess, const void *data = NULL);

	SamplerStateID addSamplerState(const Filter filter, const AddressMode s, const AddressMode t, const AddressMode r, const float lod = 0);
	BlendStateID addBlendState(const int srcFactor, const int destFactor, const int blendMode = BM_ADD, const int mask = ALL);
	DepthStateID addDepthState(const bool depthTest, const bool depthWrite, const int depthFunc = LEQUAL);
	RasterizerStateID addRasterizerState(const int cullMode, const int fillMode = SOLID, const bool multiSample = true, const bool scissor = false);

	void setTexture(const char *textureName, const TextureID texture);
	void setTexture(const char *textureName, const TextureID texture, const SamplerStateID samplerState);
	void applyTextures();

	void setSamplerState(const char *samplerName, const SamplerStateID samplerState);
	void applySamplerStates();

	void setShaderConstantRaw(const char *name, const void *data, const int size);
	void applyConstants();

//	void changeTexture(const uint imageUnit, const TextureID textureID);
	void changeRenderTargets(const TextureID *colorRTs, const uint nRenderTargets, const TextureID depthRT = TEXTURE_NONE, const int *faces = NULL);
	void changeToMainFramebuffer();
	void changeShader(const ShaderID shaderID);
	void changeVertexFormat(const VertexFormatID vertexFormatID);
	void changeVertexBuffer(const int stream, const VertexBufferID vertexBufferID, const intptr offset = 0);
	void changeIndexBuffer(const IndexBufferID indexBufferID);
	void changeCullFace(const int cullFace);

//	void changeSamplerState(const uint samplerUnit, const SamplerStateID samplerState);
	void changeBlendState(const BlendStateID blendState);
	void changeDepthState(const DepthStateID depthState);
	void changeRasterizerState(const RasterizerStateID rasterizerState);


	void clear(const bool clearColor, const bool clearDepth, const float *color = NULL, const float depth = 1.0f);

	void drawArrays(const Primitives primitives, const int firstVertex, const int nVertices);
	void drawElements(const Primitives primitives, const int firstIndex, const int nIndices, const int firstVertex, const int nVertices);

	void setup2DMode(const float left, const float right, const float top, const float bottom);
	void drawPlain(const Primitives primitives, vec2 *vertices, const uint nVertices, const BlendStateID blendState, const DepthStateID depthState, const vec4 *color = NULL);
	void drawTextured(const Primitives primitives, TexVertex *vertices, const uint nVertices, const TextureID texture, const SamplerStateID samplerState, const BlendStateID blendState, const DepthStateID depthState, const vec4 *color = NULL);

	void setFrameBuffer(ID3D10RenderTargetView *colorRTV, ID3D10DepthStencilView *depthDSV){
		backBufferRTV  = colorRTV;
		depthBufferDSV = depthDSV;
	}

	ID3D10Resource *getResource(const TextureID texture) const;

protected:
	ID3D10ShaderResourceView *createSRV(ID3D10Resource *resource, DXGI_FORMAT format = DXGI_FORMAT_UNKNOWN);
	ID3D10RenderTargetView   *createRTV(ID3D10Resource *resource, DXGI_FORMAT format = DXGI_FORMAT_UNKNOWN);
	ID3D10DepthStencilView   *createDSV(ID3D10Resource *resource, DXGI_FORMAT format = DXGI_FORMAT_UNKNOWN);

	ID3D10Device *device;
	ID3D10RenderTargetView *backBufferRTV;
	ID3D10DepthStencilView *depthBufferDSV;

	TextureID currentTexturesVS[MAX_TEXTUREUNIT], selectedTexturesVS[MAX_TEXTUREUNIT];
	TextureID currentTexturesGS[MAX_TEXTUREUNIT], selectedTexturesGS[MAX_TEXTUREUNIT];
	TextureID currentTexturesPS[MAX_TEXTUREUNIT], selectedTexturesPS[MAX_TEXTUREUNIT];

	SamplerStateID currentSamplerStatesVS[MAX_SAMPLERSTATE], selectedSamplerStatesVS[MAX_SAMPLERSTATE];
	SamplerStateID currentSamplerStatesGS[MAX_SAMPLERSTATE], selectedSamplerStatesGS[MAX_SAMPLERSTATE];
	SamplerStateID currentSamplerStatesPS[MAX_SAMPLERSTATE], selectedSamplerStatesPS[MAX_SAMPLERSTATE];

private:
	ubyte *mapRollingVB(const uint size);
	void unmapRollingVB(const uint size);
	uint copyToRollingVB(const void *src, const uint size);

	VertexBufferID rollingVB;
	int rollingVBOffset;

	ShaderID plainShader, texShader;
	VertexFormatID plainVF, texVF;

	float4 scaleBias2D;
};

#endif // _DIRECT3D10RENDERER_H_
