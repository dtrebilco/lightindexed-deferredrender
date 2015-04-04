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

#include "Direct3D10Renderer.h"
#include "../Util/String.h"

struct Texture {
	ID3D10Resource *texture;
	ID3D10ShaderResourceView *srv;
	ID3D10RenderTargetView *rtv;
	ID3D10DepthStencilView *dsv;
	DXGI_FORMAT texFormat;
	DXGI_FORMAT srvFormat;
	DXGI_FORMAT rtvFormat;
	DXGI_FORMAT dsvFormat;
	int width, height;
	uint flags;
};

struct Constant {
	char *name;
	ubyte *vsData;
	ubyte *gsData;
	ubyte *psData;
	int vsBuffer;
	int gsBuffer;
	int psBuffer;
};

int constantComp(const void *s0, const void *s1){
	return strcmp(((Constant *) s0)->name, ((Constant *) s1)->name);
}

struct Sampler {
	char *name;
	int vsIndex;
	int gsIndex;
	int psIndex;
};

int samplerComp(const void *s0, const void *s1){
	return strcmp(((Sampler *) s0)->name, ((Sampler *) s1)->name);
}

struct Shader {
	ID3D10VertexShader *vertexShader;
	ID3D10PixelShader *pixelShader;
	ID3D10GeometryShader *geometryShader;
	ID3D10Blob *inputSignature;

	ID3D10Buffer **vsConstants;
	ID3D10Buffer **gsConstants;
	ID3D10Buffer **psConstants;
	uint nVSCBuffers;
	uint nGSCBuffers;
	uint nPSCBuffers;

	Constant *constants;
	Sampler *textures;
	Sampler *samplers;

	uint nConstants;
	uint nTextures;
	uint nSamplers;

	ubyte **vsConstMem;
	ubyte **gsConstMem;
	ubyte **psConstMem;

	bool *vsDirty;
	bool *gsDirty;
	bool *psDirty;
};

struct VertexFormat {
	ID3D10InputLayout *inputLayout;
	uint vertexSize[MAX_VERTEXSTREAM];
};

struct VertexBuffer {
	ID3D10Buffer *vertexBuffer;
	long size;
};

struct IndexBuffer {
	ID3D10Buffer *indexBuffer;
	uint nIndices;
	uint indexSize;
};

struct SamplerState {
	ID3D10SamplerState *samplerState;
};

struct BlendState {
	ID3D10BlendState *blendState;
};

struct DepthState {
	ID3D10DepthStencilState *dsState;
};

struct RasterizerState {
	ID3D10RasterizerState *rsState;
};

// Blending constants
const int ZERO                = D3D10_BLEND_ZERO;
const int ONE                 = D3D10_BLEND_ONE;
const int SRC_COLOR           = D3D10_BLEND_SRC_COLOR;
const int ONE_MINUS_SRC_COLOR = D3D10_BLEND_INV_SRC_COLOR;
const int DST_COLOR           = D3D10_BLEND_DEST_COLOR;
const int ONE_MINUS_DST_COLOR = D3D10_BLEND_INV_DEST_COLOR;
const int SRC_ALPHA           = D3D10_BLEND_SRC_ALPHA;
const int ONE_MINUS_SRC_ALPHA = D3D10_BLEND_INV_SRC_ALPHA;
const int DST_ALPHA           = D3D10_BLEND_DEST_ALPHA;
const int ONE_MINUS_DST_ALPHA = D3D10_BLEND_INV_DEST_ALPHA;
const int SRC_ALPHA_SATURATE  = D3D10_BLEND_SRC_ALPHA_SAT;

const int BM_ADD              = D3D10_BLEND_OP_ADD;
const int BM_SUBTRACT         = D3D10_BLEND_OP_SUBTRACT;
const int BM_REVERSE_SUBTRACT = D3D10_BLEND_OP_REV_SUBTRACT;
const int BM_MIN              = D3D10_BLEND_OP_MIN;
const int BM_MAX              = D3D10_BLEND_OP_MAX;

// Depth testing constants
const int NEVER    = D3D10_COMPARISON_NEVER;
const int LESS     = D3D10_COMPARISON_LESS;
const int EQUAL    = D3D10_COMPARISON_EQUAL;
const int LEQUAL   = D3D10_COMPARISON_LESS_EQUAL;
const int GREATER  = D3D10_COMPARISON_GREATER;
const int NOTEQUAL = D3D10_COMPARISON_NOT_EQUAL;
const int GEQUAL   = D3D10_COMPARISON_GREATER_EQUAL;
const int ALWAYS   = D3D10_COMPARISON_ALWAYS;

// Culling constants
const int CULL_NONE  = D3D10_CULL_NONE;
const int CULL_BACK  = D3D10_CULL_BACK;
const int CULL_FRONT = D3D10_CULL_FRONT;

// Fillmode constants
const int SOLID = D3D10_FILL_SOLID;
const int WIREFRAME = D3D10_FILL_WIREFRAME;


static DXGI_FORMAT formats[] = {
	DXGI_FORMAT_UNKNOWN,
	DXGI_FORMAT_R8_UNORM,
	DXGI_FORMAT_R8G8_UNORM,
	DXGI_FORMAT_UNKNOWN, // RGB8 not directly supported
	DXGI_FORMAT_R8G8B8A8_UNORM,

	DXGI_FORMAT_R16_UNORM,
	DXGI_FORMAT_R16G16_UNORM,
	DXGI_FORMAT_UNKNOWN, // RGB16 not directly supported
	DXGI_FORMAT_R16G16B16A16_UNORM,

	DXGI_FORMAT_R16_FLOAT,
	DXGI_FORMAT_R16G16_FLOAT,
	DXGI_FORMAT_UNKNOWN, // RGB16F not directly supported
	DXGI_FORMAT_R16G16B16A16_FLOAT,

	DXGI_FORMAT_R32_FLOAT,
	DXGI_FORMAT_R32G32_FLOAT,
	DXGI_FORMAT_UNKNOWN, // RGB32F not directly supported
	DXGI_FORMAT_R32G32B32A32_FLOAT,

	DXGI_FORMAT_UNKNOWN, // RGBE8 not directly supported
	DXGI_FORMAT_R9G9B9E5_SHAREDEXP,
	DXGI_FORMAT_R11G11B10_FLOAT,
	DXGI_FORMAT_B5G6R5_UNORM,
	DXGI_FORMAT_UNKNOWN, // RGBA4 not directly supported
	DXGI_FORMAT_R10G10B10A2_UNORM,

	DXGI_FORMAT_D16_UNORM,
	DXGI_FORMAT_D24_UNORM_S8_UINT,

	DXGI_FORMAT_BC1_UNORM,
	DXGI_FORMAT_BC2_UNORM,
	DXGI_FORMAT_BC3_UNORM,
	DXGI_FORMAT_BC4_UNORM,
	DXGI_FORMAT_BC5_UNORM,
};


Direct3D10Renderer::Direct3D10Renderer(ID3D10Device *d3ddev) : Renderer(){
	device = d3ddev;

	nImageUnits = 16;
	maxAnisotropic = 16;


//	textureLod = new float[nImageUnits];

	nMRTs = 8;
	if (nMRTs > MAX_MRTS) nMRTs = MAX_MRTS;

	plainShader = SHADER_NONE;
	plainVF = VF_NONE;
	texShader = SHADER_NONE;
	texVF = VF_NONE;
//	rollingVB = NULL;
	rollingVB = VB_NONE;
	rollingVBOffset = 0;

	backBufferRTV = NULL;
	depthBufferDSV = NULL;

	setD3Ddefaults();
	resetToDefaults();
}

Direct3D10Renderer::~Direct3D10Renderer(){
	device->ClearState();


/*
	releaseFrameBufferSurfaces();
*/

	// Delete shaders
	for (uint i = 0; i < shaders.getCount(); i++){
		if (shaders[i].vertexShader  ) shaders[i].vertexShader->Release();
		if (shaders[i].geometryShader) shaders[i].geometryShader->Release();
		if (shaders[i].pixelShader   ) shaders[i].pixelShader->Release();
		if (shaders[i].inputSignature) shaders[i].inputSignature->Release();

		for (uint k = 0; k < shaders[i].nVSCBuffers; k++){
			shaders[i].vsConstants[k]->Release();
			delete shaders[i].vsConstMem[k];
		}
		for (uint k = 0; k < shaders[i].nGSCBuffers; k++){
			shaders[i].gsConstants[k]->Release();
			delete shaders[i].gsConstMem[k];
		}
		for (uint k = 0; k < shaders[i].nPSCBuffers; k++){
			shaders[i].psConstants[k]->Release();
			delete shaders[i].psConstMem[k];
		}
		delete shaders[i].vsConstants;
		delete shaders[i].gsConstants;
		delete shaders[i].psConstants;
		delete shaders[i].vsConstMem;
		delete shaders[i].gsConstMem;
		delete shaders[i].psConstMem;

		for (uint k = 0; k < shaders[i].nConstants; k++){
			delete shaders[i].constants[k].name;
		}
		delete shaders[i].constants;

		for (uint k = 0; k < shaders[i].nTextures; k++){
			delete shaders[i].textures[k].name;
		}
		delete shaders[i].textures;

		for (uint k = 0; k < shaders[i].nSamplers; k++){
			delete shaders[i].samplers[k].name;
		}
		delete shaders[i].samplers;

		delete shaders[i].vsDirty;
		delete shaders[i].gsDirty;
		delete shaders[i].psDirty;
	}

    // Delete vertex formats
	for (uint i = 0; i < vertexFormats.getCount(); i++){
		if (vertexFormats[i].inputLayout) vertexFormats[i].inputLayout->Release();
	}

    // Delete vertex buffers
	for (uint i = 0; i < vertexBuffers.getCount(); i++){
		if (vertexBuffers[i].vertexBuffer) vertexBuffers[i].vertexBuffer->Release();
	}

	// Delete index buffers
	for (uint i = 0; i < indexBuffers.getCount(); i++){
		if (indexBuffers[i].indexBuffer) indexBuffers[i].indexBuffer->Release();
	}

	// Delete samplerstates
	for (uint i = 0; i < samplerStates.getCount(); i++){
		if (samplerStates[i].samplerState) samplerStates[i].samplerState->Release();
	}

	// Delete blendstates
	for (uint i = 0; i < blendStates.getCount(); i++){
		if (blendStates[i].blendState) blendStates[i].blendState->Release();
	}

	// Delete depthstates
	for (uint i = 0; i < depthStates.getCount(); i++){
		if (depthStates[i].dsState) depthStates[i].dsState->Release();
	}

	// Delete rasterizerstates
	for (uint i = 0; i < rasterizerStates.getCount(); i++){
		if (rasterizerStates[i].rsState) rasterizerStates[i].rsState->Release();
	}

	// Delete textures
	for (uint i = 0; i < textures.getCount(); i++){
		removeTexture(i);
	}

//	if (rollingVB) rollingVB->Release();
}

void Direct3D10Renderer::reset(const uint flags){
	Renderer::reset(flags);

	if (flags & RESET_TEX){
		for (uint i = 0; i < MAX_TEXTUREUNIT; i++){
			selectedTexturesVS[i] = TEXTURE_NONE;
			selectedTexturesGS[i] = TEXTURE_NONE;
			selectedTexturesPS[i] = TEXTURE_NONE;
		}
	}

	if (flags & RESET_SS){
		for (uint i = 0; i < MAX_SAMPLERSTATE; i++){
			selectedSamplerStatesVS[i] = SS_NONE;
			selectedSamplerStatesGS[i] = SS_NONE;
			selectedSamplerStatesPS[i] = SS_NONE;
		}
	}
}

void Direct3D10Renderer::resetToDefaults(){
	Renderer::resetToDefaults();

	for (uint i = 0; i < MAX_TEXTUREUNIT; i++){
		currentTexturesVS[i] = TEXTURE_NONE;
		currentTexturesGS[i] = TEXTURE_NONE;
		currentTexturesPS[i] = TEXTURE_NONE;
	}

	for (uint i = 0; i < MAX_SAMPLERSTATE; i++){
		currentSamplerStatesVS[i] = SS_NONE;
		currentSamplerStatesGS[i] = SS_NONE;
		currentSamplerStatesPS[i] = SS_NONE;
	}

	// TODO: Fix ...
	currentRasterizerState = -2;


/*
	currentDepthRT = FB_DEPTH;
*/
}

void Direct3D10Renderer::setD3Ddefaults(){
/*
	// Set some of my preferred defaults
	dev->SetRenderState(D3DRS_LIGHTING, FALSE);
	dev->SetRenderState(D3DRS_CULLMODE, D3DCULL_NONE);

	dev->SetTextureStageState(0, D3DTSS_ALPHAOP, D3DTOP_MODULATE);

	if (maxAnisotropic > 1){
		for (uint i = 0; i < nImageUnits; i++){
			dev->SetSamplerState(i, D3DSAMP_MAXANISOTROPY, maxAnisotropic);
		}
	}
*/
}

/*bool createRenderTarget(LPDIRECT3DDEVICE9 dev, Texture &tex){
	if (isDepthFormat(tex.format)){
		if (tex.surfaces == NULL) tex.surfaces = new LPDIRECT3DSURFACE9;
		if (dev->CreateDepthStencilSurface(tex.width, tex.height, formats[tex.format], D3DMULTISAMPLE_NONE, 0, FALSE, tex.surfaces, NULL) != D3D_OK){
			delete tex.surfaces;

			ErrorMsg("Couldn't create depth surface");
			return false;
		}
	} else {
		bool mipMapped = (tex.filter.mipFilter != D3DTEXF_NONE);

		if (tex.flags & CUBEMAP){
			if (dev->CreateCubeTexture(tex.width, mipMapped? 0 : 1, tex.usage, formats[tex.format], D3DPOOL_DEFAULT, (LPDIRECT3DCUBETEXTURE9 *) &tex.texture, NULL) != D3D_OK){
				ErrorMsg("Couldn't create render target");
				return false;
			}

			if (tex.surfaces == NULL) tex.surfaces = new LPDIRECT3DSURFACE9[6];
			for (uint i = 0; i < 6; i++){
				((LPDIRECT3DCUBETEXTURE9) tex.texture)->GetCubeMapSurface((D3DCUBEMAP_FACES) i, 0, &tex.surfaces[i]);
			}
		} else {
			if (dev->CreateTexture(tex.width, tex.height, mipMapped? 0 : 1, tex.usage, formats[tex.format], D3DPOOL_DEFAULT, (LPDIRECT3DTEXTURE9 *) &tex.texture, NULL) != D3D_OK){
				ErrorMsg("Couldn't create render target");
				return false;
			}
			if (tex.surfaces == NULL) tex.surfaces = new LPDIRECT3DSURFACE9;
			((LPDIRECT3DTEXTURE9) tex.texture)->GetSurfaceLevel(0, tex.surfaces);
		}
	}

	return true;
}
*/

/*bool Direct3D10Renderer::resetDevice(){
	for (uint i = 0; i < textures.getCount(); i++){
		if (textures[i].surfaces){
			int n = (textures[i].flags & CUBEMAP)? 6 : 1;

			if (textures[i].texture) textures[i].texture->Release();
			for (int k = 0; k < n; k++){
				textures[i].surfaces[k]->Release();
			}
		}
	}

	if (!releaseFrameBufferSurfaces()) return false;

	if (dev->Reset(&d3dpp) != D3D_OK){
		ErrorMsg("Device reset failed");
		return false;
	}

	if (!createFrameBufferSurfaces()) return false;

	resetToDefaults();
	setD3Ddefaults();

	for (uint i = 0; i < textures.getCount(); i++){
		if (textures[i].surfaces){
			createRenderTarget(dev, textures[i]);
		}
	}

	return true;
}*/

TextureID Direct3D10Renderer::addTexture(Image &img, const SamplerStateID samplerState, uint flags){
	Texture tex;
	memset(&tex, 0, sizeof(tex));

	switch (img.getFormat()){
		case FORMAT_RGB8:
			img.convert(FORMAT_RGBA8);
//			img.convert(FORMAT_RGB10A2);
			break;
		case FORMAT_RGB16:
			img.convert(FORMAT_RGBA16);
			break;
		case FORMAT_RGB16F:
			img.convert(FORMAT_RGBA16F);
			break;
		case FORMAT_RGB32F:
			img.convert(FORMAT_RGBA32F);
			break;
	}

	FORMAT format = img.getFormat();
	uint nMipMaps = img.getMipMapCount();
	uint nSlices = img.isCube()? 6 : 1;

	D3D10_SUBRESOURCE_DATA texData[128], *dest = texData;
	for (uint k = 0; k < nSlices; k++){
		for (uint i = 0; i < nMipMaps; i++){
			uint pitch, slicePitch;
			if (isCompressedFormat(format)){
				pitch = ((img.getWidth(i) + 3) >> 2) * getBytesPerBlock(format);
				slicePitch = pitch * ((img.getHeight(i) + 3) >> 2);
			} else {
				pitch = img.getWidth(i) * getBytesPerPixel(format);
				slicePitch = pitch * img.getHeight(i);
			}

			dest->pSysMem = img.getPixels(i) + k * slicePitch;
			dest->SysMemPitch = pitch;
			dest->SysMemSlicePitch = slicePitch;
			dest++;
		}
	}

	tex.texFormat = formats[format];
	if (flags & SRGB){
		// Change to the matching sRGB format
		switch (tex.texFormat){
			case DXGI_FORMAT_R8G8B8A8_UNORM: tex.texFormat = DXGI_FORMAT_R8G8B8A8_UNORM_SRGB; break;
			case DXGI_FORMAT_BC1_UNORM: tex.texFormat = DXGI_FORMAT_BC1_UNORM_SRGB; break;
			case DXGI_FORMAT_BC2_UNORM: tex.texFormat = DXGI_FORMAT_BC2_UNORM_SRGB; break;
			case DXGI_FORMAT_BC3_UNORM: tex.texFormat = DXGI_FORMAT_BC3_UNORM_SRGB; break;
		}
	}

	HRESULT hr;
	if (img.is1D()){
		D3D10_TEXTURE1D_DESC desc;
		desc.Width  = img.getWidth();
		desc.Format = tex.texFormat;
		desc.MipLevels = nMipMaps;
		desc.Usage = D3D10_USAGE_IMMUTABLE;
		desc.BindFlags = D3D10_BIND_SHADER_RESOURCE;
		desc.CPUAccessFlags = 0;
		desc.ArraySize = 1;
		desc.MiscFlags = 0;

		hr = device->CreateTexture1D(&desc, texData, (ID3D10Texture1D **) &tex.texture);
	} else if (img.is2D() || img.isCube()){
		D3D10_TEXTURE2D_DESC desc;
		desc.Width  = img.getWidth();
		desc.Height = img.getHeight();
		desc.Format = tex.texFormat;
		desc.MipLevels = nMipMaps;
		desc.SampleDesc.Count = 1;
		desc.SampleDesc.Quality = 0;
		desc.Usage = D3D10_USAGE_IMMUTABLE;
		desc.BindFlags = D3D10_BIND_SHADER_RESOURCE;
		desc.CPUAccessFlags = 0;
		if (img.isCube()){
			desc.ArraySize = 6;
			desc.MiscFlags = D3D10_RESOURCE_MISC_TEXTURECUBE;
		} else {
			desc.ArraySize = 1;
			desc.MiscFlags = 0;
		}

		hr = device->CreateTexture2D(&desc, texData, (ID3D10Texture2D **) &tex.texture);
	} else if (img.is3D()){
		D3D10_TEXTURE3D_DESC desc;
		desc.Width  = img.getWidth();
		desc.Height = img.getHeight();
		desc.Depth  = img.getDepth();
		desc.Format = tex.texFormat;
		desc.MipLevels = nMipMaps;
		desc.Usage = D3D10_USAGE_IMMUTABLE;
		desc.BindFlags = D3D10_BIND_SHADER_RESOURCE;
		desc.CPUAccessFlags = 0;
		desc.MiscFlags = 0;

		hr = device->CreateTexture3D(&desc, texData, (ID3D10Texture3D **) &tex.texture);
	}

	if (FAILED(hr)){
		ErrorMsg("Couldn't create texture");
		return TEXTURE_NONE;
	}

	tex.srvFormat = tex.texFormat;
	tex.srv = createSRV(tex.texture, tex.srvFormat);

	return textures.add(tex);
}

TextureID Direct3D10Renderer::addRenderTarget(const int width, const int height, const int depth, const int arraySize, const FORMAT format, const int msaaSamples, const SamplerStateID samplerState, uint flags){
	// For now ...
	if (depth > 1) return TEXTURE_NONE;

	Texture tex;
	memset(&tex, 0, sizeof(tex));

	tex.width  = width;
	tex.height = height;
	tex.flags  = flags;
	tex.texFormat = formats[format];

	D3D10_TEXTURE2D_DESC desc;
	desc.Width  = width;
	desc.Height = height;
	desc.Format = tex.texFormat;
	desc.MipLevels = 1;
	desc.SampleDesc.Count = msaaSamples;
	desc.SampleDesc.Quality = 0;
	desc.Usage = D3D10_USAGE_DEFAULT;
	desc.BindFlags = D3D10_BIND_SHADER_RESOURCE | D3D10_BIND_RENDER_TARGET;
	desc.CPUAccessFlags = 0;
	if (flags & CUBEMAP){
		desc.ArraySize = 6;
		desc.MiscFlags = D3D10_RESOURCE_MISC_TEXTURECUBE;
	} else {
		desc.ArraySize = arraySize;
		desc.MiscFlags = 0;
	}

	if (FAILED(device->CreateTexture2D(&desc, NULL, (ID3D10Texture2D **) &tex.texture))){
		ErrorMsg("Couldn't create render target");
		return TEXTURE_NONE;
	}

	tex.srvFormat = tex.texFormat;
	tex.rtvFormat = tex.texFormat;
	tex.srv = createSRV(tex.texture, tex.srvFormat);
	tex.rtv = createRTV(tex.texture, tex.rtvFormat);

	return textures.add(tex);
}

TextureID Direct3D10Renderer::addRenderDepth(const int width, const int height, const int arraySize, const FORMAT format, const int msaaSamples, const SamplerStateID samplerState, uint flags){
	Texture tex;
	memset(&tex, 0, sizeof(tex));

	tex.width  = width;
	tex.height = height;
	tex.texFormat = tex.dsvFormat = formats[format];

	D3D10_TEXTURE2D_DESC desc;
	desc.Width  = width;
	desc.Height = height;
	desc.MipLevels = 1;
	desc.SampleDesc.Count = msaaSamples;
	desc.SampleDesc.Quality = 0;
	desc.Usage = D3D10_USAGE_DEFAULT;
	desc.BindFlags = D3D10_BIND_DEPTH_STENCIL;
	desc.CPUAccessFlags = 0;
	desc.ArraySize = arraySize;
	desc.MiscFlags = 0;

	if (flags & SAMPLE_DEPTH){
		switch (tex.dsvFormat){
			case DXGI_FORMAT_D16_UNORM:
				tex.texFormat = DXGI_FORMAT_R16_TYPELESS;
				tex.srvFormat = DXGI_FORMAT_R16_UNORM;
				break;
			case DXGI_FORMAT_D24_UNORM_S8_UINT:
				tex.texFormat = DXGI_FORMAT_R24G8_TYPELESS;
				tex.srvFormat = DXGI_FORMAT_R24_UNORM_X8_TYPELESS;
				break;
		}
		desc.BindFlags |= D3D10_BIND_SHADER_RESOURCE;
	}
	desc.Format = tex.texFormat;

	if (FAILED(device->CreateTexture2D(&desc, NULL, (ID3D10Texture2D **) &tex.texture))){
		ErrorMsg("Couldn't create depth target");
		return TEXTURE_NONE;
	}

	tex.dsv = createDSV(tex.texture, tex.dsvFormat);
	if (flags & SAMPLE_DEPTH) tex.srv = createSRV(tex.texture, tex.srvFormat);

	return textures.add(tex);
}

bool Direct3D10Renderer::resizeRenderTarget(const TextureID renderTarget, const int width, const int height, const int depth, const int arraySize){
	D3D10_RESOURCE_DIMENSION type;
	textures[renderTarget].texture->GetType(&type);

	switch (type){
/*		case D3D10_RESOURCE_DIMENSION_TEXTURE1D:
			D3D10_TEXTURE1D_DESC desc1d;
			((ID3D10Texture1D *) textures[renderTarget].texture)->GetDesc(&desc1d);

			desc1d.Width     = width;
			desc1d.ArraySize = arraySize;
			break;*/
		case D3D10_RESOURCE_DIMENSION_TEXTURE2D:
			D3D10_TEXTURE2D_DESC desc2d;
			((ID3D10Texture2D *) textures[renderTarget].texture)->GetDesc(&desc2d);

			desc2d.Width     = width;
			desc2d.Height    = height;
			desc2d.ArraySize = arraySize;

			textures[renderTarget].texture->Release();
			if (FAILED(device->CreateTexture2D(&desc2d, NULL, (ID3D10Texture2D **) &textures[renderTarget].texture))){
				ErrorMsg("Couldn't create render target");
				return false;
			}
			break;
/*		case D3D10_RESOURCE_DIMENSION_TEXTURE3D:
			D3D10_TEXTURE3D_DESC desc3d;
			((ID3D10Texture3D *) textures[renderTarget].texture)->GetDesc(&desc3d);

			desc3d.Width  = width;
			desc3d.Height = height;
			desc3d.Depth  = depth;
			break;*/
		default:
			return false;
	}

	if (textures[renderTarget].rtv){
		textures[renderTarget].rtv->Release();
		textures[renderTarget].rtv = createRTV(textures[renderTarget].texture, textures[renderTarget].rtvFormat);
	}
	if (textures[renderTarget].dsv){
		textures[renderTarget].dsv->Release();
		textures[renderTarget].dsv = createDSV(textures[renderTarget].texture, textures[renderTarget].dsvFormat);
	}
	if (textures[renderTarget].srv){
		textures[renderTarget].srv->Release();
		textures[renderTarget].srv = createSRV(textures[renderTarget].texture, textures[renderTarget].srvFormat);
	}

	textures[renderTarget].width  = width;
	textures[renderTarget].height = height;

	return true;
}

void Direct3D10Renderer::removeTexture(const TextureID texture){
	SAFE_RELEASE(textures[texture].texture);
	SAFE_RELEASE(textures[texture].srv);
	SAFE_RELEASE(textures[texture].rtv);
	SAFE_RELEASE(textures[texture].dsv);
}

ShaderID Direct3D10Renderer::addShader(const char *vsText, const char *gsText, const char *fsText, const int vsLine, const int gsLine, const int fsLine,
									   const char *header, const char *extra, const char *fileName, const char **attributeNames, const int nAttributes, const uint flags){
	if (vsText == NULL && gsText == NULL && fsText == NULL) return SHADER_NONE;

	Shader shader;
	memset(&shader, 0, sizeof(shader));

	ID3D10Blob *shaderBuf = NULL;
	ID3D10Blob *errorsBuf = NULL;

	ID3D10ShaderReflection *vsRefl = NULL;
	ID3D10ShaderReflection *gsRefl = NULL;
	ID3D10ShaderReflection *psRefl = NULL;

	if (vsText != NULL){
		String shaderString;
		if (extra != NULL) shaderString += extra;
		if (header != NULL) shaderString += header;
		shaderString.sprintf("#line %d\n", vsLine + 1);
		shaderString += vsText;

		const char *profile = D3D10GetVertexShaderProfile(device);

		if (SUCCEEDED(D3D10CompileShader(shaderString, shaderString.getLength(), fileName, NULL, NULL, "main", profile, D3D10_SHADER_PACK_MATRIX_ROW_MAJOR | D3D10_SHADER_ENABLE_STRICTNESS, &shaderBuf, &errorsBuf))){
			if (SUCCEEDED(device->CreateVertexShader(shaderBuf->GetBufferPointer(), shaderBuf->GetBufferSize(), &shader.vertexShader))){
				D3D10GetInputSignatureBlob(shaderBuf->GetBufferPointer(), shaderBuf->GetBufferSize(), &shader.inputSignature);
				D3D10ReflectShader(shaderBuf->GetBufferPointer(), shaderBuf->GetBufferSize(), &vsRefl);

#ifdef _DEBUG
				if (flags & ASSEMBLY){
					ID3D10Blob *disasm = NULL;
					if (SUCCEEDED(D3D10DisassembleShader(shaderBuf->GetBufferPointer(), shaderBuf->GetBufferSize(), FALSE, NULL, &disasm))){
						outputDebugString((const char *) disasm->GetBufferPointer());
					}
					SAFE_RELEASE(disasm);
				}
#endif
			}
		} else {
			ErrorMsg((const char *) errorsBuf->GetBufferPointer());
		}
		SAFE_RELEASE(shaderBuf);
		SAFE_RELEASE(errorsBuf);

		if (shader.vertexShader == NULL) return SHADER_NONE;
	}

	if (gsText != NULL){
		String shaderString;
		if (extra != NULL) shaderString += extra;
		if (header != NULL) shaderString += header;
		shaderString.sprintf("#line %d\n", gsLine + 1);
		shaderString += gsText;

		const char *profile = D3D10GetGeometryShaderProfile(device);

		if (SUCCEEDED(D3D10CompileShader(shaderString, shaderString.getLength(), fileName, NULL, NULL, "main", profile, D3D10_SHADER_PACK_MATRIX_ROW_MAJOR | D3D10_SHADER_ENABLE_STRICTNESS, &shaderBuf, &errorsBuf))){
			if (SUCCEEDED(device->CreateGeometryShader(shaderBuf->GetBufferPointer(), shaderBuf->GetBufferSize(), &shader.geometryShader))){
				D3D10ReflectShader(shaderBuf->GetBufferPointer(), shaderBuf->GetBufferSize(), &gsRefl);
#ifdef _DEBUG
				if (flags & ASSEMBLY){
					ID3D10Blob *disasm = NULL;
					if (SUCCEEDED(D3D10DisassembleShader(shaderBuf->GetBufferPointer(), shaderBuf->GetBufferSize(), FALSE, NULL, &disasm))){
						outputDebugString((const char *) disasm->GetBufferPointer());
					}
					SAFE_RELEASE(disasm);
				}
#endif
			}
		} else {
			ErrorMsg((const char *) errorsBuf->GetBufferPointer());
		}
		SAFE_RELEASE(shaderBuf);
		SAFE_RELEASE(errorsBuf);

		if (shader.geometryShader == NULL) return SHADER_NONE;
	}

	if (fsText != NULL){
		String shaderString;
		if (extra != NULL) shaderString += extra;
		if (header != NULL) shaderString += header;
		shaderString.sprintf("#line %d\n", fsLine + 1);
		shaderString += fsText;

		const char *profile = D3D10GetPixelShaderProfile(device);

		if (SUCCEEDED(D3D10CompileShader(shaderString, shaderString.getLength(), fileName, NULL, NULL, "main", profile, D3D10_SHADER_PACK_MATRIX_ROW_MAJOR | D3D10_SHADER_ENABLE_STRICTNESS, &shaderBuf, &errorsBuf))){
			if (SUCCEEDED(device->CreatePixelShader(shaderBuf->GetBufferPointer(), shaderBuf->GetBufferSize(), &shader.pixelShader))){
				D3D10ReflectShader(shaderBuf->GetBufferPointer(), shaderBuf->GetBufferSize(), &psRefl);
#ifdef _DEBUG
				if (flags & ASSEMBLY){
					ID3D10Blob *disasm = NULL;
					if (SUCCEEDED(D3D10DisassembleShader(shaderBuf->GetBufferPointer(), shaderBuf->GetBufferSize(), FALSE, NULL, &disasm))){
						outputDebugString((const char *) disasm->GetBufferPointer());
					}
					SAFE_RELEASE(disasm);
				}
#endif
			}
		} else {
			ErrorMsg((const char *) errorsBuf->GetBufferPointer());
		}
		SAFE_RELEASE(shaderBuf);
		SAFE_RELEASE(errorsBuf);

		if (shader.pixelShader == NULL) return SHADER_NONE;
	}

	D3D10_SHADER_DESC vsDesc, gsDesc, psDesc;
	if (vsRefl){
		vsRefl->GetDesc(&vsDesc);

		if (vsDesc.ConstantBuffers){
			shader.nVSCBuffers = vsDesc.ConstantBuffers;
			shader.vsConstants = new ID3D10Buffer *[shader.nVSCBuffers];
			shader.vsConstMem = new ubyte *[shader.nVSCBuffers];
			shader.vsDirty = new bool[shader.nVSCBuffers];
		}
	}
	if (gsRefl){
		gsRefl->GetDesc(&gsDesc);

		if (gsDesc.ConstantBuffers){
			shader.nGSCBuffers = gsDesc.ConstantBuffers;
			shader.gsConstants = new ID3D10Buffer *[shader.nGSCBuffers];
			shader.gsConstMem = new ubyte *[shader.nGSCBuffers];
			shader.gsDirty = new bool[shader.nGSCBuffers];
		}
	}
	if (psRefl){
		psRefl->GetDesc(&psDesc);

		if (psDesc.ConstantBuffers){
			shader.nPSCBuffers = psDesc.ConstantBuffers;
			shader.psConstants = new ID3D10Buffer *[shader.nPSCBuffers];
			shader.psConstMem = new ubyte *[shader.nPSCBuffers];
			shader.psDirty = new bool[shader.nPSCBuffers];
		}
	}

	D3D10_SHADER_BUFFER_DESC sbDesc;

	D3D10_BUFFER_DESC cbDesc;
	cbDesc.Usage = D3D10_USAGE_DEFAULT;//D3D10_USAGE_DYNAMIC;
	cbDesc.BindFlags = D3D10_BIND_CONSTANT_BUFFER;
	cbDesc.CPUAccessFlags = 0;//D3D10_CPU_ACCESS_WRITE;
	cbDesc.MiscFlags = 0;

	Array <Constant> constants;

	for (uint i = 0; i < shader.nVSCBuffers; i++){
		vsRefl->GetConstantBufferByIndex(i)->GetDesc(&sbDesc);

		cbDesc.ByteWidth = sbDesc.Size;
		device->CreateBuffer(&cbDesc, NULL, &shader.vsConstants[i]);

		shader.vsConstMem[i] = new ubyte[sbDesc.Size];
		for (uint k = 0; k < sbDesc.Variables; k++){
			D3D10_SHADER_VARIABLE_DESC vDesc;
			vsRefl->GetConstantBufferByIndex(i)->GetVariableByIndex(k)->GetDesc(&vDesc);

			Constant constant;
			size_t length = strlen(vDesc.Name);
			constant.name = new char[length + 1];
			strcpy(constant.name, vDesc.Name);
			constant.vsData = shader.vsConstMem[i] + vDesc.StartOffset;
			constant.gsData = NULL;
			constant.psData = NULL;
			constant.vsBuffer = i;
			constant.gsBuffer = -1;
			constant.psBuffer = -1;
			constants.add(constant);
		}

		shader.vsDirty[i] = false;
	}
	uint maxConst = constants.getCount();
	for (uint i = 0; i < shader.nGSCBuffers; i++){
		gsRefl->GetConstantBufferByIndex(i)->GetDesc(&sbDesc);

		cbDesc.ByteWidth = sbDesc.Size;
		device->CreateBuffer(&cbDesc, NULL, &shader.gsConstants[i]);

		shader.gsConstMem[i] = new ubyte[sbDesc.Size];
		for (uint k = 0; k < sbDesc.Variables; k++){
			D3D10_SHADER_VARIABLE_DESC vDesc;
			gsRefl->GetConstantBufferByIndex(i)->GetVariableByIndex(k)->GetDesc(&vDesc);

			int merge = -1;
			for (uint i = 0; i < maxConst; i++){
				if (strcmp(constants[i].name, vDesc.Name) == 0){
					merge = i;
					break;
				}
			}

			if (merge < 0){
				Constant constant;
				size_t length = strlen(vDesc.Name);
				constant.name = new char[length + 1];
				strcpy(constant.name, vDesc.Name);
				constant.vsData = NULL;
				constant.gsData = shader.gsConstMem[i] + vDesc.StartOffset;
				constant.psData = NULL;
				constant.vsBuffer = -1;
				constant.gsBuffer = i;
				constant.psBuffer = -1;
				constants.add(constant);
			} else {
				constants[merge].gsData = shader.gsConstMem[i] + vDesc.StartOffset;
				constants[merge].gsBuffer = i;
			}
		}

		shader.gsDirty[i] = false;
	}
	maxConst = constants.getCount();
	for (uint i = 0; i < shader.nPSCBuffers; i++){
		psRefl->GetConstantBufferByIndex(i)->GetDesc(&sbDesc);

		cbDesc.ByteWidth = sbDesc.Size;
		device->CreateBuffer(&cbDesc, NULL, &shader.psConstants[i]);

		shader.psConstMem[i] = new ubyte[sbDesc.Size];
		for (uint k = 0; k < sbDesc.Variables; k++){
			D3D10_SHADER_VARIABLE_DESC vDesc;
			psRefl->GetConstantBufferByIndex(i)->GetVariableByIndex(k)->GetDesc(&vDesc);

			int merge = -1;
			for (uint i = 0; i < maxConst; i++){
				if (strcmp(constants[i].name, vDesc.Name) == 0){
					merge = i;
					break;
				}
			}

			if (merge < 0){
				Constant constant;
				size_t length = strlen(vDesc.Name);
				constant.name = new char[length + 1];
				strcpy(constant.name, vDesc.Name);
				constant.vsData = NULL;
				constant.gsData = NULL;
				constant.psData = shader.psConstMem[i] + vDesc.StartOffset;
				constant.vsBuffer = -1;
				constant.gsBuffer = -1;
				constant.psBuffer = i;
				constants.add(constant);
			} else {
				constants[merge].psData = shader.psConstMem[i] + vDesc.StartOffset;
				constants[merge].psBuffer = i;
			}
		}

		shader.psDirty[i] = false;
	}

	shader.nConstants = constants.getCount();
	shader.constants = new Constant[shader.nConstants];
	memcpy(shader.constants, constants.getArray(), shader.nConstants * sizeof(Constant));
	qsort(shader.constants, shader.nConstants, sizeof(Constant), constantComp);

	uint nMaxVSRes = vsRefl? vsDesc.BoundResources : 0;
	uint nMaxGSRes = gsRefl? gsDesc.BoundResources : 0;
	uint nMaxPSRes = psRefl? psDesc.BoundResources : 0;

	int maxResources = nMaxVSRes + nMaxGSRes + nMaxPSRes;
	if (maxResources){
		shader.textures = (Sampler *) malloc(maxResources * sizeof(Sampler));
		shader.samplers = (Sampler *) malloc(maxResources * sizeof(Sampler));
		shader.nTextures = 0;
		shader.nSamplers = 0;

		D3D10_SHADER_INPUT_BIND_DESC siDesc;
		for (uint i = 0; i < nMaxVSRes; i++){
			vsRefl->GetResourceBindingDesc(i, &siDesc);

			if (siDesc.Type == D3D10_SIT_TEXTURE){
				size_t length = strlen(siDesc.Name);
				shader.textures[shader.nTextures].name = new char[length + 1];
				strcpy(shader.textures[shader.nTextures].name, siDesc.Name);
				shader.textures[shader.nTextures].vsIndex = siDesc.BindPoint;
				shader.textures[shader.nTextures].gsIndex = -1;
				shader.textures[shader.nTextures].psIndex = -1;
				shader.nTextures++;
			} else if (siDesc.Type == D3D10_SIT_SAMPLER){
				size_t length = strlen(siDesc.Name);
				shader.samplers[shader.nSamplers].name = new char[length + 1];
				strcpy(shader.samplers[shader.nSamplers].name, siDesc.Name);
				shader.samplers[shader.nSamplers].vsIndex = siDesc.BindPoint;
				shader.samplers[shader.nSamplers].gsIndex = -1;
				shader.samplers[shader.nSamplers].psIndex = -1;
				shader.nSamplers++;
			}
		}
		uint maxTexture = shader.nTextures;
		uint maxSampler = shader.nSamplers;
		for (uint i = 0; i < nMaxGSRes; i++){
			gsRefl->GetResourceBindingDesc(i, &siDesc);

			if (siDesc.Type == D3D10_SIT_TEXTURE){
				int merge = -1;
				for (uint i = 0; i < maxTexture; i++){
					if (strcmp(shader.textures[i].name, siDesc.Name) == 0){
						merge = i;
						break;
					}
				}
				if (merge < 0){
					size_t length = strlen(siDesc.Name);
					shader.textures[shader.nTextures].name = new char[length + 1];
					strcpy(shader.textures[shader.nTextures].name, siDesc.Name);
					shader.textures[shader.nTextures].vsIndex = -1;
					shader.textures[shader.nTextures].gsIndex = siDesc.BindPoint;
					shader.textures[shader.nTextures].psIndex = -1;
					shader.nTextures++;
				} else {
					shader.textures[merge].gsIndex = siDesc.BindPoint;
				}
			} else if (siDesc.Type == D3D10_SIT_SAMPLER){
				int merge = -1;
				for (uint i = 0; i < maxSampler; i++){
					if (strcmp(shader.samplers[i].name, siDesc.Name) == 0){
						merge = i;
						break;
					}
				}
				if (merge < 0){
					size_t length = strlen(siDesc.Name);
					shader.samplers[shader.nSamplers].name = new char[length + 1];
					strcpy(shader.samplers[shader.nSamplers].name, siDesc.Name);
					shader.samplers[shader.nSamplers].vsIndex = -1;
					shader.samplers[shader.nSamplers].gsIndex = siDesc.BindPoint;
					shader.samplers[shader.nSamplers].psIndex = -1;
					shader.nSamplers++;
				} else {
					shader.samplers[merge].gsIndex = siDesc.BindPoint;
				}
			}
		}
		maxTexture = shader.nTextures;
		maxSampler = shader.nSamplers;
		for (uint i = 0; i < nMaxPSRes; i++){
			psRefl->GetResourceBindingDesc(i, &siDesc);

			if (siDesc.Type == D3D10_SIT_TEXTURE){
				int merge = -1;
				for (uint i = 0; i < maxTexture; i++){
					if (strcmp(shader.textures[i].name, siDesc.Name) == 0){
						merge = i;
						break;
					}
				}
				if (merge < 0){
					size_t length = strlen(siDesc.Name);
					shader.textures[shader.nTextures].name = new char[length + 1];
					strcpy(shader.textures[shader.nTextures].name, siDesc.Name);
					shader.textures[shader.nTextures].vsIndex = -1;
					shader.textures[shader.nTextures].gsIndex = -1;
					shader.textures[shader.nTextures].psIndex = siDesc.BindPoint;
					shader.nTextures++;
				} else {
					shader.textures[merge].psIndex = siDesc.BindPoint;
				}
			} else if (siDesc.Type == D3D10_SIT_SAMPLER){
				int merge = -1;
				for (uint i = 0; i < maxSampler; i++){
					if (strcmp(shader.samplers[i].name, siDesc.Name) == 0){
						merge = i;
						break;
					}
				}
				if (merge < 0){
					size_t length = strlen(siDesc.Name);
					shader.samplers[shader.nSamplers].name = new char[length + 1];
					strcpy(shader.samplers[shader.nSamplers].name, siDesc.Name);
					shader.samplers[shader.nSamplers].vsIndex = -1;
					shader.samplers[shader.nSamplers].gsIndex = -1;
					shader.samplers[shader.nSamplers].psIndex = siDesc.BindPoint;
					shader.nSamplers++;
				} else {
					shader.samplers[merge].psIndex = siDesc.BindPoint;
				}
			}
		}
		shader.textures = (Sampler *) realloc(shader.textures, shader.nTextures * sizeof(Sampler));
		shader.samplers = (Sampler *) realloc(shader.samplers, shader.nSamplers * sizeof(Sampler));
		qsort(shader.textures, shader.nTextures, sizeof(Sampler), samplerComp);
		qsort(shader.samplers, shader.nSamplers, sizeof(Sampler), samplerComp);
	}

	if (vsRefl) vsRefl->Release();
	if (gsRefl) gsRefl->Release();
	if (psRefl) psRefl->Release();

	return shaders.add(shader);
}

VertexFormatID Direct3D10Renderer::addVertexFormat(const FormatDesc *formatDesc, const uint nAttribs, const ShaderID shader){
	static const DXGI_FORMAT formats[][4] = {
		DXGI_FORMAT_R32_FLOAT, DXGI_FORMAT_R32G32_FLOAT, DXGI_FORMAT_R32G32B32_FLOAT, DXGI_FORMAT_R32G32B32A32_FLOAT,
		DXGI_FORMAT_R16_FLOAT, DXGI_FORMAT_R16G16_FLOAT, DXGI_FORMAT_UNKNOWN,         DXGI_FORMAT_R16G16B16A16_FLOAT,
		DXGI_FORMAT_R8_UNORM,  DXGI_FORMAT_R8G8_UNORM,   DXGI_FORMAT_UNKNOWN,         DXGI_FORMAT_R8G8B8A8_UNORM,
	};

	static const char *semantics[] = {
		NULL,
		"Position",
		"Texcoord",
		"Normal",
		"Tangent",
		"Binormal",
	};


	int index[6];
	memset(index, 0, sizeof(index));

	VertexFormat vf;
	memset(vf.vertexSize, 0, sizeof(vf.vertexSize));

	D3D10_INPUT_ELEMENT_DESC *desc = new D3D10_INPUT_ELEMENT_DESC[nAttribs];

	// Fill the vertex element array
	for (uint i = 0; i < nAttribs; i++){
		int stream = formatDesc[i].stream;
		int size = formatDesc[i].size;
		desc[i].InputSlot = stream;
		desc[i].AlignedByteOffset = vf.vertexSize[stream];
		desc[i].SemanticName = semantics[formatDesc[i].type];
		desc[i].SemanticIndex = index[formatDesc[i].type]++;
		desc[i].Format = formats[formatDesc[i].format][size - 1];
		desc[i].InputSlotClass = D3D10_INPUT_PER_VERTEX_DATA;
		desc[i].InstanceDataStepRate = 0;

		vf.vertexSize[stream] += size * getFormatSize(formatDesc[i].format);
	}

	HRESULT hr = device->CreateInputLayout(desc, nAttribs, shaders[shader].inputSignature->GetBufferPointer(), shaders[shader].inputSignature->GetBufferSize(), &vf.inputLayout);
	delete desc;

	if (FAILED(hr)){
		ErrorMsg("Couldn't create vertex declaration");
		return VF_NONE;
	}

	return vertexFormats.add(vf);
}

D3D10_USAGE usage[] = {
	D3D10_USAGE_IMMUTABLE,
	D3D10_USAGE_DEFAULT,
	D3D10_USAGE_DYNAMIC,
};

VertexBufferID Direct3D10Renderer::addVertexBuffer(const long size, const BufferAccess bufferAccess, const void *data){
	VertexBuffer vb;
	vb.size = size;

	D3D10_BUFFER_DESC desc;
	desc.Usage = usage[bufferAccess];
	desc.ByteWidth = size;
	desc.BindFlags = D3D10_BIND_VERTEX_BUFFER;
	desc.CPUAccessFlags = (bufferAccess == DYNAMIC)? D3D10_CPU_ACCESS_WRITE : 0;
	desc.MiscFlags = 0;

	D3D10_SUBRESOURCE_DATA vbData;
	vbData.pSysMem = data;
	vbData.SysMemPitch = 0;
	vbData.SysMemSlicePitch = 0;

	if (FAILED(device->CreateBuffer(&desc, data? &vbData : NULL, &vb.vertexBuffer))){
        ErrorMsg("Couldn't create vertex buffer");
		return VB_NONE;
	}

	return vertexBuffers.add(vb);
}

IndexBufferID Direct3D10Renderer::addIndexBuffer(const uint nIndices, const uint indexSize, const BufferAccess bufferAccess, const void *data){
	IndexBuffer ib;
	ib.indexSize = indexSize;
	ib.nIndices = nIndices;

	D3D10_BUFFER_DESC desc;
	desc.Usage = usage[bufferAccess];
	desc.ByteWidth = nIndices * indexSize;
	desc.BindFlags = D3D10_BIND_INDEX_BUFFER;
	desc.CPUAccessFlags = (bufferAccess == DYNAMIC)? D3D10_CPU_ACCESS_WRITE : 0;
	desc.MiscFlags = 0;

	D3D10_SUBRESOURCE_DATA ibData;
	ibData.pSysMem = data;
	ibData.SysMemPitch = 0;
	ibData.SysMemSlicePitch = 0;

	if (FAILED(device->CreateBuffer(&desc, data? &ibData : NULL, &ib.indexBuffer))){
        ErrorMsg("Couldn't create vertex buffer");
		return IB_NONE;
	}

	return indexBuffers.add(ib);
}

D3D10_FILTER filters[] = {
	D3D10_FILTER_MIN_MAG_MIP_POINT,
	D3D10_FILTER_MIN_MAG_LINEAR_MIP_POINT,
	D3D10_FILTER_MIN_MAG_LINEAR_MIP_POINT,
	D3D10_FILTER_MIN_MAG_MIP_LINEAR,
	D3D10_FILTER_ANISOTROPIC,
	D3D10_FILTER_ANISOTROPIC,
};

SamplerStateID Direct3D10Renderer::addSamplerState(const Filter filter, const AddressMode s, const AddressMode t, const AddressMode r, const float lod){
	SamplerState samplerState;

	D3D10_SAMPLER_DESC desc;
	desc.Filter = filters[filter];
	desc.AddressU = (s == WRAP)? D3D10_TEXTURE_ADDRESS_WRAP : D3D10_TEXTURE_ADDRESS_CLAMP;
	desc.AddressV = (t == WRAP)? D3D10_TEXTURE_ADDRESS_WRAP : D3D10_TEXTURE_ADDRESS_CLAMP;
	desc.AddressW = (r == WRAP)? D3D10_TEXTURE_ADDRESS_WRAP : D3D10_TEXTURE_ADDRESS_CLAMP;
	desc.MipLODBias = lod;
	desc.MaxAnisotropy = hasAniso(filter)? 16 : 1;
	desc.ComparisonFunc = D3D10_COMPARISON_ALWAYS;
	desc.BorderColor[0] = 0;
	desc.BorderColor[1] = 0;
	desc.BorderColor[2] = 0;
	desc.BorderColor[3] = 0;
	desc.MinLOD = 0;
	desc.MaxLOD = hasMipmaps(filter)? D3D10_FLOAT32_MAX : 0;

	if (FAILED(device->CreateSamplerState(&desc, &samplerState.samplerState))){
		ErrorMsg("Couldn't create samplerstate");
		return SS_NONE;
	}

	return samplerStates.add(samplerState);
}

BlendStateID Direct3D10Renderer::addBlendState(const int srcFactor, const int destFactor, const int blendMode, const int mask){
	BlendState blendState;

	BOOL blendEnable = (srcFactor != D3D10_BLEND_ONE || destFactor != D3D10_BLEND_ZERO);

	D3D10_BLEND_DESC desc;
	desc.AlphaToCoverageEnable = FALSE;
	desc.BlendOp = (D3D10_BLEND_OP) blendMode;
	desc.SrcBlend = (D3D10_BLEND) srcFactor;
	desc.DestBlend = (D3D10_BLEND) destFactor;
	desc.BlendOpAlpha = (D3D10_BLEND_OP) blendMode;
	desc.SrcBlendAlpha = (D3D10_BLEND) srcFactor;
	desc.DestBlendAlpha = (D3D10_BLEND) destFactor;

	memset(&desc.BlendEnable, 0, sizeof(desc.BlendEnable));
	memset(&desc.RenderTargetWriteMask, 0, sizeof(desc.RenderTargetWriteMask));
	desc.BlendEnable[0] = blendEnable;
	desc.RenderTargetWriteMask[0] = mask;

	if (FAILED(device->CreateBlendState(&desc, &blendState.blendState))){
		ErrorMsg("Couldn't create blendstate");
		return BS_NONE;
	}

	return blendStates.add(blendState);
}

DepthStateID Direct3D10Renderer::addDepthState(const bool depthTest, const bool depthWrite, const int depthFunc){
	DepthState depthState;

	D3D10_DEPTH_STENCIL_DESC desc;
	desc.DepthEnable = (BOOL) depthTest;
	desc.DepthWriteMask = depthWrite? D3D10_DEPTH_WRITE_MASK_ALL : D3D10_DEPTH_WRITE_MASK_ZERO;
	desc.DepthFunc = (D3D10_COMPARISON_FUNC) depthFunc;
	desc.StencilEnable = FALSE;
	desc.StencilReadMask = 0;
	desc.StencilWriteMask = 0;
	desc.BackFace.StencilFunc = D3D10_COMPARISON_ALWAYS;
	desc.BackFace.StencilDepthFailOp = D3D10_STENCIL_OP_KEEP;
	desc.BackFace.StencilFailOp = D3D10_STENCIL_OP_KEEP;
	desc.BackFace.StencilPassOp = D3D10_STENCIL_OP_KEEP;
	desc.FrontFace.StencilFunc = D3D10_COMPARISON_ALWAYS;
	desc.FrontFace.StencilDepthFailOp = D3D10_STENCIL_OP_KEEP;
	desc.FrontFace.StencilFailOp = D3D10_STENCIL_OP_KEEP;
	desc.FrontFace.StencilPassOp = D3D10_STENCIL_OP_KEEP;

	if (FAILED(device->CreateDepthStencilState(&desc, &depthState.dsState))){
		ErrorMsg("Couldn't create depthstate");
		return DS_NONE;
	}

	return depthStates.add(depthState);	
}

RasterizerStateID Direct3D10Renderer::addRasterizerState(const int cullMode, const int fillMode, const bool multiSample, const bool scissor){
	RasterizerState rasterizerState;

	D3D10_RASTERIZER_DESC desc;
	desc.CullMode = (D3D10_CULL_MODE) cullMode;
	desc.FillMode = (D3D10_FILL_MODE) fillMode;
	desc.FrontCounterClockwise = FALSE;
	desc.DepthBias = 0;
	desc.DepthBiasClamp = 0.0f;
	desc.SlopeScaledDepthBias = 0.0f;
	desc.AntialiasedLineEnable = FALSE;
	desc.DepthClipEnable = TRUE;
	desc.MultisampleEnable = (BOOL) multiSample;
	desc.ScissorEnable = (BOOL) scissor;

	if (FAILED(device->CreateRasterizerState(&desc, &rasterizerState.rsState))){
		ErrorMsg("Couldn't create rasterizerstate");
		return RS_NONE;
	}

	return rasterizerStates.add(rasterizerState);
}

const Sampler *getSampler(const Sampler *samplers, const int count, const char *name){
	int minSampler = 0;
	int maxSampler = count - 1;

	// Do a quick lookup in the sorted table with a binary search
	while (minSampler <= maxSampler){
		int currSampler = (minSampler + maxSampler) >> 1;
        int res = strcmp(name, samplers[currSampler].name);
		if (res == 0){
			return samplers + currSampler;
		} else if (res > 0){
            minSampler = currSampler + 1;
		} else {
            maxSampler = currSampler - 1;
		}
	}

	return NULL;
}

void Direct3D10Renderer::setTexture(const char *textureName, const TextureID texture){
	ASSERT(selectedShader != SHADER_NONE);

	const Sampler *s = getSampler(shaders[selectedShader].textures, shaders[selectedShader].nTextures, textureName);
	if (s){
		if (s->vsIndex >= 0) selectedTexturesVS[s->vsIndex] = texture;
		if (s->gsIndex >= 0) selectedTexturesGS[s->gsIndex] = texture;
		if (s->psIndex >= 0) selectedTexturesPS[s->psIndex] = texture;
	}
}

void Direct3D10Renderer::setTexture(const char *textureName, const TextureID texture, const SamplerStateID samplerState){
	ASSERT(selectedShader != SHADER_NONE);

	const Sampler *s = getSampler(shaders[selectedShader].textures, shaders[selectedShader].nTextures, textureName);
	if (s){
		if (s->vsIndex >= 0){
			selectedTexturesVS[s->vsIndex] = texture;
			selectedSamplerStatesVS[s->vsIndex] = samplerState;
		}
		if (s->gsIndex >= 0){
			selectedTexturesGS[s->gsIndex] = texture;
			selectedSamplerStatesGS[s->gsIndex] = samplerState;
		}
		if (s->psIndex >= 0){
			selectedTexturesPS[s->psIndex] = texture;
			selectedSamplerStatesPS[s->psIndex] = samplerState;
		}
	}
}

bool fillSRV(ID3D10ShaderResourceView **dest, int &min, int &max, const TextureID selectedTextures[], TextureID currentTextures[], const Texture *textures){
	min = 0;
	do {
		if (selectedTextures[min] != currentTextures[min]){
			max = MAX_TEXTUREUNIT;
			do {
				max--;
			} while (selectedTextures[max] == currentTextures[max]);

			for (int i = min; i <= max; i++){
				if (selectedTextures[i] != TEXTURE_NONE){
					*dest++ = textures[selectedTextures[i]].srv;
				} else {
					*dest++ = NULL;
				}
				currentTextures[i] = selectedTextures[i];
			}
			return true;
		}
		min++;
	} while (min < MAX_TEXTUREUNIT);

	return false;
}

void Direct3D10Renderer::applyTextures(){
	ID3D10ShaderResourceView *srViews[MAX_TEXTUREUNIT];

	int min, max;
	if (fillSRV(srViews, min, max, selectedTexturesVS, currentTexturesVS, textures.getArray())){
		device->VSSetShaderResources(min, max - min + 1, srViews);
	}
	if (fillSRV(srViews, min, max, selectedTexturesGS, currentTexturesGS, textures.getArray())){
		device->GSSetShaderResources(min, max - min + 1, srViews);
	}
	if (fillSRV(srViews, min, max, selectedTexturesPS, currentTexturesPS, textures.getArray())){
		device->PSSetShaderResources(min, max - min + 1, srViews);
	}
}

void Direct3D10Renderer::setSamplerState(const char *samplerName, const SamplerStateID samplerState){
	ASSERT(selectedShader != SHADER_NONE);

	const Sampler *s = getSampler(shaders[selectedShader].samplers, shaders[selectedShader].nSamplers, samplerName);
	if (s){
		if (s->vsIndex >= 0) selectedSamplerStatesVS[s->vsIndex] = samplerState;
		if (s->gsIndex >= 0) selectedSamplerStatesGS[s->gsIndex] = samplerState;
		if (s->psIndex >= 0) selectedSamplerStatesPS[s->psIndex] = samplerState;
	}
}

bool fillSS(ID3D10SamplerState **dest, int &min, int &max, const SamplerStateID selectedSamplerStates[], SamplerStateID currentSamplerStates[], const SamplerState *samplerStates){
	min = 0;
	do {
		if (selectedSamplerStates[min] != currentSamplerStates[min]){
			max = MAX_SAMPLERSTATE;
			do {
				max--;
			} while (selectedSamplerStates[max] == currentSamplerStates[max]);

			for (int i = min; i <= max; i++){
				if (selectedSamplerStates[i] != SS_NONE){
					*dest++ = samplerStates[selectedSamplerStates[i]].samplerState;
				} else {
					*dest++ = NULL;
				}
				currentSamplerStates[i] = selectedSamplerStates[i];
			}
			return true;
		}
		min++;
	} while (min < MAX_SAMPLERSTATE);

	return false;
}

void Direct3D10Renderer::applySamplerStates(){
	ID3D10SamplerState *samplers[MAX_SAMPLERSTATE];

	int min, max;
	if (fillSS(samplers, min, max, selectedSamplerStatesVS, currentSamplerStatesVS, samplerStates.getArray())){
		device->VSSetSamplers(min, max - min + 1, samplers);
	}
	if (fillSS(samplers, min, max, selectedSamplerStatesGS, currentSamplerStatesGS, samplerStates.getArray())){
		device->GSSetSamplers(min, max - min + 1, samplers);
	}
	if (fillSS(samplers, min, max, selectedSamplerStatesPS, currentSamplerStatesPS, samplerStates.getArray())){
		device->PSSetSamplers(min, max - min + 1, samplers);
	}
}

void Direct3D10Renderer::setShaderConstantRaw(const char *name, const void *data, const int size){
	int minConstant = 0;
	int maxConstant = shaders[selectedShader].nConstants - 1;
	Constant *constants = shaders[selectedShader].constants;

	// Do a quick lookup in the sorted table with a binary search
	while (minConstant <= maxConstant){
		int currConstant = (minConstant + maxConstant) >> 1;
		int res = strcmp(name, constants[currConstant].name);
		if (res == 0){
			Constant *c = constants + currConstant;

			if (c->vsData){
				if (memcmp(c->vsData, data, size)){
					memcpy(c->vsData, data, size);
					shaders[selectedShader].vsDirty[c->vsBuffer] = true;
				}
			}
			if (c->gsData){
				if (memcmp(c->gsData, data, size)){
					memcpy(c->gsData, data, size);
					shaders[selectedShader].gsDirty[c->gsBuffer] = true;
				}
			}
			if (c->psData){
				if (memcmp(c->psData, data, size)){
					memcpy(c->psData, data, size);
					shaders[selectedShader].psDirty[c->psBuffer] = true;
				}
			}
			return;

		} else if (res > 0){
			minConstant = currConstant + 1;
		} else {
			maxConstant = currConstant - 1;
		}
	}

#ifdef _DEBUG
	char str[256];
	sprintf(str, "Invalid constant \"%s\"", name);
	outputDebugString(str);
#endif
}

void Direct3D10Renderer::applyConstants(){
	if (currentShader != SHADER_NONE){
		Shader *shader = &shaders[currentShader];

		for (uint i = 0; i < shader->nVSCBuffers; i++){
			if (shader->vsDirty[i]){
				device->UpdateSubresource(shader->vsConstants[i], 0, NULL, shader->vsConstMem[i], 0, 0);
				shader->vsDirty[i] = false;
			}
		}
		for (uint i = 0; i < shader->nGSCBuffers; i++){
			if (shader->gsDirty[i]){
				device->UpdateSubresource(shader->gsConstants[i], 0, NULL, shader->gsConstMem[i], 0, 0);
				shader->gsDirty[i] = false;
			}
		}
		for (uint i = 0; i < shader->nPSCBuffers; i++){
			if (shader->psDirty[i]){
				device->UpdateSubresource(shader->psConstants[i], 0, NULL, shader->psConstMem[i], 0, 0);
				shader->psDirty[i] = false;
			}
		}
	}
}

void Direct3D10Renderer::changeRenderTargets(const TextureID *colorRTs, const uint nRenderTargets, const TextureID depthRT, const int *faces){
	ID3D10RenderTargetView *rtv[16];
	ID3D10DepthStencilView *dsv;

	if (depthRT == FB_COLOR){
		dsv = depthBufferDSV;
	} else if (depthRT == TEXTURE_NONE){
		dsv = NULL;
	} else {
		dsv = textures[depthRT].dsv;
	}
	currentDepthRT = depthRT;

	for (uint i = 0; i < nRenderTargets; i++){
		TextureID rt = colorRTs[i];
		int face = (faces != NULL)? faces[i] : 0;

		rtv[i] = textures[rt].rtv;

		currentColorRT[i] = rt;
		currentColorRTFace[i] = face;
	}

	device->OMSetRenderTargets(nRenderTargets, rtv, dsv);

	D3D10_VIEWPORT vp = { 0, 0, textures[colorRTs[0]].width, textures[colorRTs[0]].height, 0.0f, 1.0f };
	device->RSSetViewports(1, &vp);

/*
	for (uint i = 0; i < nRenderTargets; i++){
		TextureID rt = colorRTs[i];
		int face = (faces != NULL)? faces[i] : 0;

		if (rt != currentColorRT[i] || face != currentColorRTFace[i]){
			dev->SetRenderTarget(i, textures[rt].surfaces[face]);

			currentColorRT[i] = rt;
			currentColorRTFace[i] = face;
		}
	}

	for (uint i = nRenderTargets; i < nMRTs; i++){
		if (currentColorRT[i] != TEXTURE_NONE){
			dev->SetRenderTarget(i, NULL);
			currentColorRT[i] = TEXTURE_NONE;
		}
	}

	if (depthRT != currentDepthRT){
		if (depthRT == TEXTURE_NONE){
			dev->SetDepthStencilSurface(NULL);
		} else if (depthRT == FB_DEPTH){
			dev->SetDepthStencilSurface(fbDepth);
		} else {
			dev->SetDepthStencilSurface(textures[depthRT].surfaces[0]);
		}
		currentDepthRT = depthRT;
	}
*/
}

void Direct3D10Renderer::changeToMainFramebuffer(){
	device->OMSetRenderTargets(1, &backBufferRTV, depthBufferDSV);
	
	D3D10_VIEWPORT vp = { 0, 0, viewportWidth, viewportHeight, 0.0f, 1.0f };
	device->RSSetViewports(1, &vp);

	for (uint i = 1; i < MAX_MRTS; i++){
		currentColorRT[i] = FB_COLOR;
		currentColorRTFace[i] = 0;
	}
	currentDepthRT = FB_DEPTH;

/*
	if (currentColorRT[0] != TEXTURE_NONE){
		dev->SetRenderTarget(0, fbColor);
		currentColorRT[0] = TEXTURE_NONE;
	}

	for (uint i = 1; i < nMRTs; i++){
		if (currentColorRT[i] != TEXTURE_NONE){
			dev->SetRenderTarget(i, NULL);
			currentColorRT[i] = TEXTURE_NONE;
		}
	}

	if (currentDepthRT != FB_DEPTH){
		dev->SetDepthStencilSurface(fbDepth);
		currentDepthRT = FB_DEPTH;
	}
*/
}

void Direct3D10Renderer::changeShader(const ShaderID shaderID){
	if (shaderID != currentShader){
		if (shaderID == SHADER_NONE){
//			device->VSSetShader(NULL);
//			device->GSSetShader(NULL);
//			device->PSSetShader(NULL);
		} else {
			device->VSSetShader(shaders[shaderID].vertexShader);
			device->GSSetShader(shaders[shaderID].geometryShader);
			device->PSSetShader(shaders[shaderID].pixelShader);

			if (shaders[shaderID].nVSCBuffers) device->VSSetConstantBuffers(0, shaders[shaderID].nVSCBuffers, shaders[shaderID].vsConstants);
			if (shaders[shaderID].nGSCBuffers) device->GSSetConstantBuffers(0, shaders[shaderID].nGSCBuffers, shaders[shaderID].gsConstants);
			if (shaders[shaderID].nPSCBuffers) device->PSSetConstantBuffers(0, shaders[shaderID].nPSCBuffers, shaders[shaderID].psConstants);
		}

		currentShader = shaderID;
	}
}

void Direct3D10Renderer::changeVertexFormat(const VertexFormatID vertexFormatID){
	if (vertexFormatID != currentVertexFormat){
		if (vertexFormatID == VF_NONE){
			device->IASetInputLayout(NULL);
		} else {
			device->IASetInputLayout(vertexFormats[vertexFormatID].inputLayout);

			/*if (currentVertexFormat != VF_NONE){
				for (int i = 0; i < MAX_VERTEXSTREAM; i++){
					if (vertexFormats[vertexFormatID].vertexSize[i] != vertexFormats[currentVertexFormat].vertexSize[i]){
						currentVertexBuffers[i] = VB_INVALID;
					}
				}
			}*/
		}

		currentVertexFormat = vertexFormatID;
	}
}

void Direct3D10Renderer::changeVertexBuffer(const int stream, const VertexBufferID vertexBufferID, const intptr offset){
	if (vertexBufferID != currentVertexBuffers[stream] || offset != currentOffsets[stream]){
		UINT strides[1];
		UINT offsets[1];
		if (vertexBufferID == VB_NONE){
			strides[0] = 0;
			offsets[0] = 0;
			ID3D10Buffer *null[] = { NULL };
			device->IASetVertexBuffers(stream, 1, null, strides, offsets);
		} else {
			strides[0] = vertexFormats[currentVertexFormat].vertexSize[stream];
			offsets[0] = (UINT) offset;
			device->IASetVertexBuffers(stream, 1, &vertexBuffers[vertexBufferID].vertexBuffer, strides, offsets);
		}

		currentVertexBuffers[stream] = vertexBufferID;
		currentOffsets[stream] = offset;
	}
}

void Direct3D10Renderer::changeIndexBuffer(const IndexBufferID indexBufferID){
	if (indexBufferID != currentIndexBuffer){
		if (indexBufferID == IB_NONE){
			device->IASetIndexBuffer(NULL, DXGI_FORMAT_UNKNOWN, 0);
		} else {
			DXGI_FORMAT format = indexBuffers[indexBufferID].indexSize < 4? DXGI_FORMAT_R16_UINT : DXGI_FORMAT_R32_UINT;
			device->IASetIndexBuffer(indexBuffers[indexBufferID].indexBuffer, format, 0);
		}

		currentIndexBuffer = indexBufferID;
	}
}

void Direct3D10Renderer::changeBlendState(const BlendStateID blendState){
	if (blendState != currentBlendState){
		if (blendState == BS_NONE){
			device->OMSetBlendState(NULL, float4(0, 0, 0, 0), 0xFFFFFFFF);
		} else {
			device->OMSetBlendState(blendStates[blendState].blendState, float4(0, 0, 0, 0), 0xFFFFFFFF);
		}

		currentBlendState = blendState;
	}
}

void Direct3D10Renderer::changeDepthState(const DepthStateID depthState){
	if (depthState != currentDepthState){
		if (depthState == DS_NONE){
			device->OMSetDepthStencilState(NULL, 0);
		} else {
			device->OMSetDepthStencilState(depthStates[depthState].dsState, 0);
		}

		currentDepthState = depthState;
	}
}

void Direct3D10Renderer::changeRasterizerState(const RasterizerStateID rasterizerState){
	if (rasterizerState != currentRasterizerState){
		if (rasterizerState == RS_NONE){
			//device->RSSetState(NULL);
			device->RSSetState(rasterizerStates[0].rsState);
		} else {
			device->RSSetState(rasterizerStates[rasterizerState].rsState);
		}

		currentRasterizerState = rasterizerState;
	}
}

void Direct3D10Renderer::clear(const bool clearColor, const bool clearDepth, const float *color, const float depth){
	if (clearColor){
		if (currentColorRT[0] == FB_COLOR){
			device->ClearRenderTargetView(backBufferRTV, color);
		}

		for (int i = 0; i < MAX_MRTS; i++){
			if (currentColorRT[i] >= 0){
				device->ClearRenderTargetView(textures[currentColorRT[i]].rtv, color);
			}
		}
	}
	if (clearDepth){
		if (currentDepthRT == FB_DEPTH){
			device->ClearDepthStencilView(depthBufferDSV, D3D10_CLEAR_DEPTH, depth, 0);
		} else if (currentDepthRT >= 0){
			device->ClearDepthStencilView(textures[currentDepthRT].dsv, D3D10_CLEAR_DEPTH, depth, 0);
		}
	}
}


const D3D10_PRIMITIVE_TOPOLOGY d3dPrim[] = {
	D3D10_PRIMITIVE_TOPOLOGY_TRIANGLELIST,
	D3D10_PRIMITIVE_TOPOLOGY_UNDEFINED, // Triangle fans not supported
	D3D10_PRIMITIVE_TOPOLOGY_TRIANGLESTRIP,
	D3D10_PRIMITIVE_TOPOLOGY_UNDEFINED, // Quads not supported
	D3D10_PRIMITIVE_TOPOLOGY_LINELIST,
	D3D10_PRIMITIVE_TOPOLOGY_LINESTRIP,
	D3D10_PRIMITIVE_TOPOLOGY_UNDEFINED, // Line loops not supported
	D3D10_PRIMITIVE_TOPOLOGY_POINTLIST,
};

void Direct3D10Renderer::drawArrays(const Primitives primitives, const int firstVertex, const int nVertices){
	device->IASetPrimitiveTopology(d3dPrim[primitives]);
	device->Draw(nVertices, firstVertex);
}

void Direct3D10Renderer::drawElements(const Primitives primitives, const int firstIndex, const int nIndices, const int firstVertex, const int nVertices){
	device->IASetPrimitiveTopology(d3dPrim[primitives]);
	device->DrawIndexed(nIndices, firstIndex, 0);
}

void Direct3D10Renderer::setup2DMode(const float left, const float right, const float top, const float bottom){
	scaleBias2D.x = 2.0f / (right - left);
	scaleBias2D.y = 2.0f / (top - bottom);
	scaleBias2D.z = -1.0f;
	scaleBias2D.w =  1.0f;
}

const char *plainVS =
"float4 scaleBias;"
"float4 main(float4 position: Position): SV_Position {"
"	position.xy = position.xy * scaleBias.xy + scaleBias.zw;"
"	return position;"
"}";

const char *plainPS =
"float4 color;"
"float4 main() : SV_Target {"
"	return color;"
"}";

const char *texDefs =
"struct VsIn {"
"	float4 position: Position;"
"	float2 texCoord: TexCoord;"
"};"
"struct PsIn {"
"	float4 position: SV_Position;"
"	float2 texCoord: TexCoord;"
"};\n";

const char *texVS =
"float4 scaleBias;"
"PsIn main(VsIn In){"
"	PsIn Out;"
"	Out.position = In.position;"
"	Out.position.xy = Out.position.xy * scaleBias.xy + scaleBias.zw;"
"	Out.texCoord = In.texCoord;"
"	return Out;"
"}";

const char *texPS =
"Texture2D Base: register(t0);"
"SamplerState base: register(s0);"
"float4 color;"
"float4 main(PsIn In) : SV_Target {"
"	return Base.Sample(base, In.texCoord) * color;"
"}";

void Direct3D10Renderer::drawPlain(const Primitives primitives, vec2 *vertices, const uint nVertices, const BlendStateID blendState, const DepthStateID depthState, const vec4 *color){
	int size = nVertices * sizeof(vec2);

	UINT stride = sizeof(vec2);
	UINT offset = copyToRollingVB(vertices, size);

	if (plainShader == SHADER_NONE){
		plainShader = addShader(plainVS, NULL, plainPS, 0, 0, 0);

		FormatDesc format[] = { 0, TYPE_VERTEX, FORMAT_FLOAT, 2 };
		plainVF = addVertexFormat(format, elementsOf(format), plainShader);
	}

	float4 col = float4(1, 1, 1, 1);
	if (color) col = *color;

	reset();
	setShader(plainShader);
	setShaderConstant4f("scaleBias", scaleBias2D);
	setShaderConstant4f("color", col);
	setBlendState(blendState);
	setDepthState(depthState);
	setVertexFormat(plainVF);
	setVertexBuffer(0, rollingVB, offset);
	apply();

	device->IASetPrimitiveTopology(d3dPrim[primitives]);
	device->Draw(nVertices, 0);
}

void Direct3D10Renderer::drawTextured(const Primitives primitives, TexVertex *vertices, const uint nVertices, const TextureID texture, const SamplerStateID samplerState, const BlendStateID blendState, const DepthStateID depthState, const vec4 *color){
	int size = nVertices * sizeof(TexVertex);

	UINT stride = sizeof(TexVertex);
	UINT offset = copyToRollingVB(vertices, size);

	if (texShader == SHADER_NONE){
		texShader = addShader(texVS, NULL, texPS, 0, 0, 0, texDefs);

		FormatDesc format[] = {
			0, TYPE_VERTEX,   FORMAT_FLOAT, 2,
			0, TYPE_TEXCOORD, FORMAT_FLOAT, 2,
		};
		texVF = addVertexFormat(format, elementsOf(format), texShader);
	}

	float4 col = float4(1, 1, 1, 1);
	if (color) col = *color;

	reset();
	setShader(texShader);
	setShaderConstant4f("scaleBias", scaleBias2D);
	setShaderConstant4f("color", col);
	setTexture("Base", texture);
	setSamplerState("base", samplerState);
	setBlendState(blendState);
	setDepthState(depthState);
	setVertexFormat(texVF);
	setVertexBuffer(0, rollingVB, offset);
	apply();

	device->IASetPrimitiveTopology(d3dPrim[primitives]);
	device->Draw(nVertices, 0);
}

ID3D10ShaderResourceView *Direct3D10Renderer::createSRV(ID3D10Resource *resource, DXGI_FORMAT format){
	D3D10_RESOURCE_DIMENSION type;
	resource->GetType(&type);

	D3D10_SHADER_RESOURCE_VIEW_DESC srvDesc;
	ID3D10ShaderResourceView *srv;

	switch (type){
		case D3D10_RESOURCE_DIMENSION_TEXTURE1D:
			D3D10_TEXTURE1D_DESC desc1d;
			((ID3D10Texture1D *) resource)->GetDesc(&desc1d);

			srvDesc.Format = (format != DXGI_FORMAT_UNKNOWN)? format : desc1d.Format;
			if (desc1d.ArraySize > 1){
				srvDesc.ViewDimension = D3D10_SRV_DIMENSION_TEXTURE2DARRAY;
				srvDesc.Texture1DArray.FirstArraySlice = 0;
				srvDesc.Texture1DArray.ArraySize = desc1d.ArraySize;
				srvDesc.Texture1DArray.MostDetailedMip = 0;
				srvDesc.Texture1DArray.MipLevels = desc1d.MipLevels;
			} else {
				srvDesc.ViewDimension = D3D10_SRV_DIMENSION_TEXTURE1D;
				srvDesc.Texture1D.MostDetailedMip = 0;
				srvDesc.Texture1D.MipLevels = desc1d.MipLevels;
			}
			break;
		case D3D10_RESOURCE_DIMENSION_TEXTURE2D:
			D3D10_TEXTURE2D_DESC desc2d;
			((ID3D10Texture2D *) resource)->GetDesc(&desc2d);

			srvDesc.Format = (format != DXGI_FORMAT_UNKNOWN)? format : desc2d.Format;
			if (desc2d.ArraySize > 1){
				if (desc2d.MiscFlags & D3D10_RESOURCE_MISC_TEXTURECUBE){
					srvDesc.ViewDimension = D3D10_SRV_DIMENSION_TEXTURECUBE;
					srvDesc.TextureCube.MostDetailedMip = 0;
					srvDesc.TextureCube.MipLevels = desc2d.MipLevels;
				} else {
					srvDesc.ViewDimension = D3D10_SRV_DIMENSION_TEXTURE2DARRAY;
					srvDesc.Texture2DArray.FirstArraySlice = 0;
					srvDesc.Texture2DArray.ArraySize = desc2d.ArraySize;
					srvDesc.Texture2DArray.MostDetailedMip = 0;
					srvDesc.Texture2DArray.MipLevels = desc2d.MipLevels;
				}
			} else {
				srvDesc.ViewDimension = (desc2d.SampleDesc.Count > 1)? D3D10_SRV_DIMENSION_TEXTURE2DMS : D3D10_SRV_DIMENSION_TEXTURE2D;
				srvDesc.Texture2D.MostDetailedMip = 0;
				srvDesc.Texture2D.MipLevels = desc2d.MipLevels;
			}
			break;
		case D3D10_RESOURCE_DIMENSION_TEXTURE3D:
			D3D10_TEXTURE3D_DESC desc3d;
			((ID3D10Texture3D *) resource)->GetDesc(&desc3d);

			srvDesc.Format = (format != DXGI_FORMAT_UNKNOWN)? format : desc3d.Format;
			srvDesc.ViewDimension = D3D10_SRV_DIMENSION_TEXTURE3D;
			srvDesc.Texture3D.MostDetailedMip = 0;
			srvDesc.Texture3D.MipLevels = desc3d.MipLevels;
			break;
		default:
			ErrorMsg("Unsupported type");
			return NULL;
	}

	if (FAILED(device->CreateShaderResourceView(resource, &srvDesc, &srv))){
		ErrorMsg("CreateShaderResourceView failed");
		return NULL;
	}

	return srv;
}

ID3D10RenderTargetView *Direct3D10Renderer::createRTV(ID3D10Resource *resource, DXGI_FORMAT format){
	D3D10_RESOURCE_DIMENSION type;
	resource->GetType(&type);

	D3D10_RENDER_TARGET_VIEW_DESC rtvDesc;
	ID3D10RenderTargetView *rtv;

	switch (type){
		case D3D10_RESOURCE_DIMENSION_TEXTURE2D:
			D3D10_TEXTURE2D_DESC desc2d;
			((ID3D10Texture2D *) resource)->GetDesc(&desc2d);

			rtvDesc.Format = (format != DXGI_FORMAT_UNKNOWN)? format : desc2d.Format;
			if (desc2d.ArraySize > 1){
				rtvDesc.ViewDimension = D3D10_RTV_DIMENSION_TEXTURE2DARRAY;
				rtvDesc.Texture2DArray.FirstArraySlice = 0;
				rtvDesc.Texture2DArray.ArraySize = desc2d.ArraySize;
				rtvDesc.Texture2DArray.MipSlice = 0;
			} else {
				rtvDesc.ViewDimension = (desc2d.SampleDesc.Count > 1)? D3D10_RTV_DIMENSION_TEXTURE2DMS : D3D10_RTV_DIMENSION_TEXTURE2D;
				rtvDesc.Texture2D.MipSlice = 0;
			}
			break;
		default:
			ErrorMsg("Unsupported type");
			return NULL;
	}

	if (FAILED(device->CreateRenderTargetView(resource, &rtvDesc, &rtv))){
		ErrorMsg("CreateRenderTargetView failed");
		return NULL;
	}

	return rtv;
}

ID3D10DepthStencilView *Direct3D10Renderer::createDSV(ID3D10Resource *resource, DXGI_FORMAT format){
	D3D10_RESOURCE_DIMENSION type;
	resource->GetType(&type);

	D3D10_DEPTH_STENCIL_VIEW_DESC dsvDesc;
	ID3D10DepthStencilView *dsv;

	switch (type){
		case D3D10_RESOURCE_DIMENSION_TEXTURE2D:
			D3D10_TEXTURE2D_DESC desc2d;
			((ID3D10Texture2D *) resource)->GetDesc(&desc2d);

			dsvDesc.Format = (format != DXGI_FORMAT_UNKNOWN)? format : desc2d.Format;
			if (desc2d.ArraySize > 1){
				dsvDesc.ViewDimension = D3D10_DSV_DIMENSION_TEXTURE2DARRAY;
				dsvDesc.Texture2DArray.FirstArraySlice = 0;
				dsvDesc.Texture2DArray.ArraySize = desc2d.ArraySize;
				dsvDesc.Texture2DArray.MipSlice = 0;
			} else {
				dsvDesc.ViewDimension = (desc2d.SampleDesc.Count > 1)? D3D10_DSV_DIMENSION_TEXTURE2DMS : D3D10_DSV_DIMENSION_TEXTURE2D;
				dsvDesc.Texture2D.MipSlice = 0;
			}
			break;
		default:
			ErrorMsg("Unsupported type");
			return NULL;
	}

	if (FAILED(device->CreateDepthStencilView(resource, &dsvDesc, &dsv))){
		ErrorMsg("CreateDepthStencilView failed");
		return NULL;
	}

	return dsv;
}

ubyte *Direct3D10Renderer::mapRollingVB(const uint size){
	ASSERT(size <= ROLLING_VB_SIZE);

	if (rollingVB == VB_NONE) rollingVB = addVertexBuffer(ROLLING_VB_SIZE, DEFAULT);

	ubyte *data = NULL;
	D3D10_MAP flag = D3D10_MAP_WRITE_NO_OVERWRITE;
	if (rollingVBOffset + size > ROLLING_VB_SIZE){
		flag = D3D10_MAP_WRITE_DISCARD;
		rollingVBOffset = 0;
	}

	vertexBuffers[rollingVB].vertexBuffer->Map(flag, 0, (void **) &data);

	return data + rollingVBOffset;
}

void Direct3D10Renderer::unmapRollingVB(const uint size){
	vertexBuffers[rollingVB].vertexBuffer->Unmap();

	rollingVBOffset += size;
}

uint Direct3D10Renderer::copyToRollingVB(const void *src, const uint size){
	ASSERT(size <= ROLLING_VB_SIZE);

	if (rollingVB == VB_NONE) rollingVB = addVertexBuffer(ROLLING_VB_SIZE, DYNAMIC);

	ubyte *data = NULL;
	D3D10_MAP flag = D3D10_MAP_WRITE_NO_OVERWRITE;
	if (rollingVBOffset + size > ROLLING_VB_SIZE){
		flag = D3D10_MAP_WRITE_DISCARD;
		rollingVBOffset = 0;
	}

	uint offset = rollingVBOffset;
	vertexBuffers[rollingVB].vertexBuffer->Map(flag, 0, (void **) &data);
		memcpy(data + offset, src, size);
	vertexBuffers[rollingVB].vertexBuffer->Unmap();
/*
	D3D10_BOX box;
	box.left   = offset;
	box.right  = offset + size;
	box.top    = 0;
	box.bottom = 1;
	box.front  = 0;
	box.back   = 1;

	device->UpdateSubresource(vertexBuffers[rollingVB].vertexBuffer, 0, &box, src, 0, 0);
*/
	rollingVBOffset += size;

	return offset;
}

ID3D10Resource *Direct3D10Renderer::getResource(const TextureID texture) const {
	return textures[texture].texture;
}
