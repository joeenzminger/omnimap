#include <string>
#include <vector>
#include "OmniMapD3D10.h"
#include "OmniMapChannelD3D10.h"
#include "OmniMapShaderD3D10.h"
#include "ScreenRendererD3D10.h"
#include "OM_ErrorHandling.h"


static ID3D10Buffer *logoBuffer;
static ID3D10Texture2D *logoTexture;
static ID3D10ShaderResourceView *logoShaderResView;


#ifdef USE_VC7
#define MBSTOWCS(DSIZE, DSTRING, SSIZE, SSTRING) DSIZE = mbstowcs(DSTRING, SSTRING, SSIZE);
#else
#define MBSTOWCS(DSIZE, DSTRING, SSIZE, SSTRING) mbstowcs_s(&DSIZE, DSTRING, SSIZE, SSTRING, _TRUNCATE);
#endif




OMNIMAP_UNMANAGEDGLUE_API OmniMapD3D10 *UnmanagedGlue_OmnimapConstructorD3D10(int _resW, int _resH, ID3D10Device *_d3dDevice, const char *strStartUpScriptFile, const char *luaResDir);
OMNIMAP_UNMANAGEDGLUE_API void UnmanagedGlue_OmnimapDestructorD3D10(OmniMapD3D10 *omnimapHandle);
OMNIMAP_UNMANAGEDGLUE_API int UnmanagedGlue_OmnimapGetChannelTotalD3D10(OmniMapD3D10 *omnimapHandle);
OMNIMAP_UNMANAGEDGLUE_API OmniMapChannelD3D10 *UnmanagedGlue_OmnimapGetChannelInfoD3D10(OmniMapD3D10 *omnimapHandle, int channelNumber, float viewMatrix[16], float projectionMatrix[16]);
OMNIMAP_UNMANAGEDGLUE_API void UnmanagedGlue_OmnimapBeginRenderChannelD3D10(OmniMapChannelD3D10 *channelHandle);
OMNIMAP_UNMANAGEDGLUE_API void UnmanagedGlue_OmnimapEndRenderChannelD3D10(OmniMapChannelD3D10 *channelHandle);
OMNIMAP_UNMANAGEDGLUE_API void UnmanagedGlue_OmnimapPostRenderD3D10(OmniMapD3D10 *omnimapHandle);


OMNIMAP_UNMANAGEDGLUE_API OmniMapD3D10 *UnmanagedGlue_OmnimapConstructorD3D10(int _resW, int _resH, ID3D10Device *_d3dDevice, const char *strStartUpScriptFile, const char *luaResDir) {
  const char *strStartUpScriptFileBase = (strStartUpScriptFile) ? strStartUpScriptFile : "Resources/omnimap_startup.lua";
  const char *luaResDirBase = (luaResDir) ? luaResDir : "Resources/";
  return new OmniMapD3D10(_resW, _resH, _d3dDevice, strStartUpScriptFileBase, luaResDirBase);
}


OMNIMAP_UNMANAGEDGLUE_API void UnmanagedGlue_OmnimapDestructorD3D10(OmniMapD3D10 *omnimapHandle) {
  delete omnimapHandle;
}


OMNIMAP_UNMANAGEDGLUE_API int UnmanagedGlue_OmnimapGetChannelTotalD3D10(OmniMapD3D10 *omnimapHandle) {
  return omnimapHandle->GetNumChannels();
}

OMNIMAP_UNMANAGEDGLUE_API OmniMapChannelD3D10 *UnmanagedGlue_OmnimapGetChannelInfoD3D10(OmniMapD3D10 *omnimapHandle, int channelNumber, float viewMatrix[16], float projectionMatrix[16]) {
  OmniMapChannelD3D10 *channel = (OmniMapChannelD3D10 *)omnimapHandle->GetChannel(channelNumber);

  if (viewMatrix) {
    D3DXMATRIX viewMat;
    channel->GetViewMatrix(&viewMat);
    memcpy(viewMatrix, (FLOAT*)viewMat, sizeof(float)*16);
  }
  if (projectionMatrix) {
    D3DXMATRIX projMat;
    channel->GetProjectionMatrix(&projMat);
    memcpy(projectionMatrix, (FLOAT*)projMat, sizeof(float)*16);
  }

  return channel;
}

OMNIMAP_UNMANAGEDGLUE_API void UnmanagedGlue_OmnimapBeginRenderChannelD3D10(OmniMapChannelD3D10 *channelHandle) {
  channelHandle->beginRenderToChannel();
}

OMNIMAP_UNMANAGEDGLUE_API void UnmanagedGlue_OmnimapEndRenderChannelD3D10(OmniMapChannelD3D10 *channelHandle) {
  channelHandle->endRenderToChannel();
}

OMNIMAP_UNMANAGEDGLUE_API void UnmanagedGlue_OmnimapPostRenderD3D10(OmniMapD3D10 *omnimapHandle) {
  omnimapHandle->PostRender();
}





void OmniMapD3D10::construct(bool doInit)
{
	pStateBlock = NULL;
	//om_protector.unlock("","");

	glViewportsettings0 = OMNIMAP_DEFAULT_glViewportsettings0;
	glViewportsettings1 = OMNIMAP_DEFAULT_glViewportsettings1;
	glViewportsettings2 = OMNIMAP_DEFAULT_glViewportsettings2;
	glViewportsettings3 = OMNIMAP_DEFAULT_glViewportsettings3;
	GL_STATE_CLEAR_AT_STARTFRAME = OMNIMAP_DEFAULT_GL_STATE_CLEAR_AT_STARTFRAME;
	GL_STATE_glDisable_GL_DEPTH_TEST = OMNIMAP_DEFAULT_GL_STATE_glDisable_GL_DEPTH_TEST;
	GL_STATE_glDepthMask_FALSE = OMNIMAP_DEFAULT_GL_STATE_glDepthMask_FALSE;

	SetUpPropertyAccess();
  strcpy(rendererType, "D3D10");
  ScreenRendererFactory.addScreenRendererCreator("D3D10", &ScreenRendererD3D10::CreateScreenRendererD3D10);

	if (doInit)
		init();
}




OmniMapD3D10::OmniMapD3D10(int _resW, int _resH, ID3D10Device *_d3dDevice, const char* strStartUpScriptFile, 
					   const char *luaResDir, bool doInit) :
	OmniMapBase(_resW, _resH, strStartUpScriptFile, luaResDir)
{
  EH_DECLARE;
  EH_Log("Constructing OmniMapD3D10");

  shaders = NULL;
  logoBuffer = NULL;
  logoTexture = NULL;
  logoShaderResView = NULL;

  graphicsContext = (void *) _d3dDevice;

  fxShaderFilename[0] = '\0';

  d3d10Device = _d3dDevice;

  pRasterState = NULL;
  construct(doInit);
}




OmniMapD3D10::~OmniMapD3D10()
{
  SAFE_RELEASE(pRasterState);
  SAFE_RELEASE(logoBuffer);
  SAFE_RELEASE(logoTexture);
  SAFE_RELEASE(logoShaderResView);

  Clear_Channels();

  if (shaders != NULL) {
    delete ((OmniMapShaderD3D10 *) shaders);
    shaders = NULL;
  }

  ClearScreenShapes();
}




void OmniMapD3D10::SetUpPropertyAccess()
{
	///////////////
	access.Register("glViewportsettings0",&glViewportsettings0);
	access.Register("glViewportsettings1",&glViewportsettings1);
	access.Register("glViewportsettings2",&glViewportsettings2);
	access.Register("glViewportsettings3",&glViewportsettings3); 
	access.Register("GL_STATE_CLEAR_AT_STARTFRAME",&GL_STATE_CLEAR_AT_STARTFRAME); 
	access.Register("GL_STATE_glDisable_GL_DEPTH_TEST",&GL_STATE_glDisable_GL_DEPTH_TEST); 
	access.Register("GL_STATE_glDepthMask_FALSE",&GL_STATE_glDepthMask_FALSE); 
	access.Register("FXShaderFile", &fxShaderFilename[0]);
	OmniMapBase::SetUpPropertyAccess();

}



void OmniMapD3D10::init()
{
  D3D10_RASTERIZER_DESC CurrentRasterizerState;
  EH_DECLARE;

  std::string setLuaDir = "LuaSupportDir = \"";
  setLuaDir.append(luaSupportDir);
  setLuaDir.append("\"");
  size_t index = 0;
  while ((index = setLuaDir.find("\\", index)) != std::string::npos)
  {
    setLuaDir.replace(index, 1, "/");
    index++;
  }

  EH_Zero(ScriptingEngine->RunString(setLuaDir.c_str()));
  EH_Zero(ScriptingEngine->RunScript(StartUpScript));

  CurrentRasterizerState.FillMode = D3D10_FILL_SOLID;
  CurrentRasterizerState.CullMode = D3D10_CULL_NONE;
  CurrentRasterizerState.FrontCounterClockwise = true;
  CurrentRasterizerState.DepthBias = false;
  CurrentRasterizerState.DepthBiasClamp = 0;
  CurrentRasterizerState.SlopeScaledDepthBias = 0;
  CurrentRasterizerState.DepthClipEnable = !GL_STATE_glDisable_GL_DEPTH_TEST;
  CurrentRasterizerState.ScissorEnable = false;
  CurrentRasterizerState.MultisampleEnable = false;
  CurrentRasterizerState.AntialiasedLineEnable = false;
  CurrentRasterizerState.MultisampleEnable = true;
  EH_Test(d3d10Device->CreateRasterizerState(&CurrentRasterizerState, &pRasterState));

  EH_Ptr(shaders = new OmniMapShaderD3D10(d3d10Device));

  ((OmniMapShaderD3D10 *)shaders)->CobraWarpWithTrueDimension = (this)->CobraWarpWithTrueDimension;

  if (CobraWarpWithTrueDimension>0) {
    EH_Log("CobraWarpWithTrueDimension (0 is off 1 is Warp with TrueDimension On, 2 is Warp with TrueDimension off ) = %d", CobraWarpWithTrueDimension);
  }

  if (fxShaderFilename[0] != '\0') {
    EH_Log("loading shader %s", fxShaderFilename);
    std::string vertexShaderFilename = luaSupportDir;
    vertexShaderFilename.append(fxShaderFilename);
    ((OmniMapShaderD3D10 *) shaders)->setShadersFilenames(vertexShaderFilename.c_str(), NULL);
  }

  ((OmniMapShaderD3D10 *) shaders)->setNumChannels(GetNumChannels());
  if(StencilMask_filename)
    ((OmniMapShaderD3D10 *)shaders)->StencilMask_Load(StencilMask_filename); // the final mask needs to be done before the shader is compiled....

  shaders->init();

  for (int i = 0; i < GetNumChannels(); i++)
  {
    ((OmniMapChannelD3D10 *) this->GetChannel(i))->setOMShader((OmniMapShaderD3D10 *) shaders);
  }

  for(int i = 0; i < GetNumScreenShapes(); i++)
  {
    ScreenGeometryBase *screenShape = this->GetScreenShape(i);
    ScreenRendererD3D10 *renderer = (ScreenRendererD3D10 *) screenShape->GetRenderer();
    renderer->setOMShader((OmniMapShaderD3D10 *)shaders);
  }

  CreateLogoSurface();

  EH_Log("------------OMNIMAP LOADED----------------");

  EH_OnError() {
    EH_Log("------------OMNIMAP FAILED----------------");
    SAFE_RELEASE(pRasterState);
    if (shaders) {
      delete ((OmniMapShaderD3D10 *) shaders);
      shaders = NULL;
    }
  }

  EH_Log("glViewportsettings %f %f %f %f",glViewportsettings0,glViewportsettings1,glViewportsettings2,glViewportsettings3);
  EH_Log("GL_STATE_CLEAR_AT_STARTFRAME, GL_STATE_glDisable_GL_DEPTH_TEST,GL_STATE_glDepthMask_FALSE %d %d %d",	GL_STATE_CLEAR_AT_STARTFRAME, GL_STATE_glDisable_GL_DEPTH_TEST,GL_STATE_glDepthMask_FALSE );
  EH_Log("width %d",		resWidth);
  EH_Log("height %d",		resHeight);
  EH_Log("displayHUD  %d",displayHUD );
  EH_Log("clear color %f %f %f %f",clearcolor_r,clearcolor_g,clearcolor_b,clearcolor_a );
  EH_Log("AudiencePosition %f %f %f\n",AudiencePosition.x,AudiencePosition.y,AudiencePosition.z);
  EH_Log("ProjectorPosition.pos %f %f %f\n",ProjectorPosition.pos.x,ProjectorPosition.pos.y,ProjectorPosition.pos.z);
  EH_Log("ProjectorPosition.headsUp %f %f %f\n",ProjectorPosition.headsUp.x,ProjectorPosition.headsUp.y,ProjectorPosition.headsUp.z);
  EH_Log("ProjectorPosition.lookAtpos %f %f %f\n",ProjectorPosition.lookAtpos.x,ProjectorPosition.lookAtpos.y,ProjectorPosition.lookAtpos.z);
  EH_Log("startupscript : %s",StartUpScript);
  EH_Log("------------------------------");

}



OmniMapChannelBase *OmniMapD3D10::Create_Channel(const char * name) 
{
	name;
  return new OmniMapChannelD3D10(d3d10Device);
}



void OmniMapD3D10::Clear()
{
  float clearColor[4] = { clearcolor_r,clearcolor_g,clearcolor_b,clearcolor_a };
  ID3D10RenderTargetView* apOldRTVs = NULL;
  d3d10Device->OMGetRenderTargets( 1, &apOldRTVs, NULL );
  d3d10Device->ClearRenderTargetView( apOldRTVs, clearColor );
  SAFE_RELEASE(apOldRTVs);
}




void OmniMapD3D10::SetupHudMatrix()
{
  D3DXMATRIX projMat;
  D3DXMatrixOrthoLH(&projMat, 2.0f, 2.0f, -1.0f, 1.0f);
	((OmniMapShaderD3D10 *) shaders)->setHudProjMatrix((float *) &projMat);
}




void OmniMapD3D10::DrawHeadsUpDisplay()
{
	//d3dDevice->SetRenderState(D3DRS_ZENABLE, false);
	OmniMapBase::DrawHeadsUpDisplays();
	//d3dDevice->SetRenderState(D3DRS_ZENABLE, true);
}




void OmniMapD3D10::SetProjectiveTexturingMatricies()
{
  D3DXMATRIX matView;
  D3DXVECTOR3 camPos( ProjectorPosition.pos.x, ProjectorPosition.pos.y, ProjectorPosition.pos.z ); // Camera position
  D3DXVECTOR3 camLookat( ProjectorPosition.lookAtpos.x, ProjectorPosition.lookAtpos.y, ProjectorPosition.lookAtpos.z );   // Look-at point
  D3DXVECTOR3 camUp( ProjectorPosition.headsUp.x, ProjectorPosition.headsUp.y, ProjectorPosition.headsUp.z ); // Up vector

  D3DXMatrixOrthoRH(&worldViewProjection, 2.0, 2.0, 0.0, 1.0f);
  D3DXMatrixLookAtRH( &worldView, &camPos, &camLookat, &camUp);
}




void OmniMapD3D10::SetupAllShaderVaribles()
{
	int N_Channels;
	int TextureIds[OMNIMAP_maxChannelFaces];
	D3DXMATRIX textureMats[OMNIMAP_maxChannelFaces];

  GetShaderVariables(N_Channels, TextureIds);
  ((OmniMapShaderD3D10 *) shaders)->SetTextureIds(N_Channels);

  for (int i = 0; i < N_Channels; i++)
  {
    OmniMapChannelBase *channel = this->GetChannel(i);
    D3DXQUATERNION tmpQuat;
    D3DXMATRIX *pTextureMat = &textureMats[i];
    D3DXMATRIX tmpMat;
    D3DXMatrixIdentity(pTextureMat);
    const D3DXVECTOR3 zeroVec(0.0,0.0,0.0);
    const D3DXVECTOR3 transVec(.5,.5,.5);

    D3DXMatrixAffineTransformation(pTextureMat, .5, &zeroVec, D3DXQuaternionIdentity(&tmpQuat),
      &transVec);	

    D3DXMATRIX channelProj;
    ((OmniMapChannelD3D10 *) channel)->GetProjectionMatrixRH(&channelProj);

    D3DXMatrixMultiply(pTextureMat, &channelProj, pTextureMat);
    D3DXMATRIX viewMatrix;
    ((OmniMapChannelD3D10 *) channel)->GetViewMatrix2(&viewMatrix);
    D3DXMatrixMultiply(pTextureMat, &viewMatrix, pTextureMat);
  }
  ((OmniMapShaderD3D10 *) shaders)->SetTextureMatrices(textureMats, N_Channels);


  D3D10_VIEWPORT vp;
  UINT one = 1;
  d3d10Device->RSGetViewports(&one, &vp);
  //float yOffset2 = ((glViewportsettings3/2.0)/((float) vp.Height-(float) glViewportsettings1))/2.0;
  float yOffset1, yOffset2, yScale;
  if (((glViewportsettings0 + glViewportsettings2) <= vp.Width)
    && (glViewportsettings0 >= 0)
    && ((glViewportsettings1 + glViewportsettings3) <= vp.Height)
    && (glViewportsettings1 >= 0))
  {
    //xOffset1 = 0.0;
    //xOffset2 = 0.0;
    //xScale = ((float) glViewportsettings2)/ ((float) vp.Width);
    vp.TopLeftX = (int) glViewportsettings0;
    vp.TopLeftY = (int) glViewportsettings1;
    vp.Width = (int) glViewportsettings2;
    vp.Height = (int) glViewportsettings3;
    d3d10Device->RSSetViewports(1, &vp);
    yOffset1 = 0.0;
    yOffset2 = 0.0;
    yScale = 1.0;
  } else {
    yOffset2 = ((glViewportsettings3/2.0f)/((float) vp.Height-(float) glViewportsettings1))/2.0f;
    yScale = (((float) glViewportsettings2) / ((float) vp.Height));
    yOffset1 = (((float) glViewportsettings1) / ((float) glViewportsettings3));
  }


  if (yOffset1 != 0.0) yOffset2 = 0.0;

  ((OmniMapShaderD3D10 *) shaders)->SetYOffsets(yScale, yOffset1, yOffset2);
  ((OmniMapShaderD3D10 *) shaders)->SetViewProjMats((float *) &worldViewProjection, (float *) &worldView);

  if(StencilMask_filename)
    ((OmniMapShaderD3D10 *) shaders)->StencilMask_Bind(6);//+1
}




void OmniMapD3D10::PostRender()
{
	HRESULT hr;

	// Post Render does a few things
	// A0 Clear
	// B1 set the projective shader
	// B2 Set up Shaders 
	// B3 textures
	// B4 Set up matricies
	// C1 Display Screen
	// D0 disable shaders
	// D1 Disable Textures
	// D2 Set active texutre to 0

	// E0 PushAndSetupMatrix
	// E1 draw lua console
	// E2 draw heads up display
	// E7 PopMatrix
	//  8 

	// A. 0 Clear
  D3D10_STATE_BLOCK_MASK stateBlockMask;
  D3D10StateBlockMaskEnableAll(&stateBlockMask);
  hr =  D3D10CreateStateBlock(d3d10Device, &stateBlockMask, &pStateBlock);

  pStateBlock->Capture();

  if(GL_STATE_CLEAR_AT_STARTFRAME)
    Clear();

  // hr = d3d10Device->BeginScene();
  // Still need to make sure we are rendering to final buffer here.

  // Set us up for multisampling

  d3d10Device->RSSetState(pRasterState);

  shaders->EnableShader();

  // B.2  set the textures
  BindAllChannelTextures();

  SetProjectiveTexturingMatricies();

  SetupAllShaderVaribles();

  DisplayScreenShapes();

  shaders->DisableShader();

  if (this->displayHUD)
  {
    // Render HUD
    SetupHudMatrix();
    DrawHeadsUpDisplay();
  }

  pStateBlock->Apply();
  SAFE_RELEASE(pStateBlock);
}






void OmniMapD3D10::CreateTextureFromFile(ID3D10Device *d3dDevice, char *filename, ID3D10ShaderResourceView **shaderResView, 
								 ID3D10Texture2D **texture)
{
  D3DX10_IMAGE_LOAD_INFO loadInfo;
  D3D10_TEXTURE2D_DESC desc;
  D3D10_MAPPED_TEXTURE2D mappedTex2D = {0};
  D3D10_SHADER_RESOURCE_VIEW_DESC SRVDesc;
  ID3D10Resource *pRes = NULL;
  ID3D10Texture2D* pTemp = NULL;;
  size_t origsize = strlen(filename) + 1;
  size_t convertedChars = 0;
  wchar_t wcstring[2048];
  EH_DECLARE;

  MBSTOWCS(convertedChars, wcstring, origsize, filename);

  ZeroMemory( &loadInfo, sizeof( D3DX10_IMAGE_LOAD_INFO ) );
  loadInfo.Width = D3DX_FROM_FILE;
  loadInfo.Height  = D3DX_FROM_FILE;
  loadInfo.Depth  = D3DX_FROM_FILE;
  loadInfo.FirstMipLevel = 0;
  loadInfo.MipLevels = 1;
  loadInfo.Usage = D3D10_USAGE_STAGING;
  loadInfo.BindFlags = 0;
  loadInfo.CpuAccessFlags = D3D10_CPU_ACCESS_WRITE | D3D10_CPU_ACCESS_READ;
  loadInfo.MiscFlags = 0;
  loadInfo.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
  loadInfo.Filter = D3DX10_FILTER_NONE;
  loadInfo.MipFilter = D3DX10_FILTER_NONE;

  ZeroMemory( &desc, sizeof(D3D10_TEXTURE2D_DESC) );
  EH_Test(D3DX10CreateTextureFromFile( d3dDevice, wcstring, &loadInfo, NULL, &pRes, NULL ));
  EH_Test(pRes->QueryInterface( __uuidof( ID3D10Texture2D ), (LPVOID*)&pTemp ));

  pTemp->GetDesc( &desc );

  EH_Zero(DXGI_FORMAT_R8G8B8A8_UNORM == desc.Format); //make sure we're R8G8B8A8

  //if(desc.MipLevels > 4)
  //desc.MipLevels -= 4;

  desc.Usage = D3D10_USAGE_DEFAULT;
  desc.BindFlags = D3D10_BIND_SHADER_RESOURCE;
  desc.CPUAccessFlags = 0;
  desc.ArraySize = 1;

  EH_Test(d3dDevice->CreateTexture2D( &desc, NULL, texture));

  for(UINT iMip=0; iMip < desc.MipLevels; iMip++)
  {
    EH_Test(pTemp->Map( iMip, D3D10_MAP_READ, 0, &mappedTex2D ));
    d3dDevice->UpdateSubresource( *texture, 
      D3D10CalcSubresource( iMip, 0, desc.MipLevels ),
      NULL,
      mappedTex2D.pData,
      mappedTex2D.RowPitch,
      0 );
    pTemp->Unmap( iMip );
  }

  // Create the resource view
  ZeroMemory( &SRVDesc, sizeof(SRVDesc) );
  SRVDesc.Format = desc.Format;
  SRVDesc.ViewDimension = D3D10_SRV_DIMENSION_TEXTURE2D;
  SRVDesc.Texture2D.MipLevels = desc.MipLevels;
  SRVDesc.Texture2D.MostDetailedMip = 0;
  EH_Test(d3dDevice->CreateShaderResourceView( *texture, &SRVDesc, shaderResView ));

  EH_OnError() {}

  SAFE_RELEASE( pTemp );
  SAFE_RELEASE( pRes );
}




void OmniMapD3D10::CreateTextureFromMemory(ID3D10Device *d3dDevice, unsigned char *textureData, int size, ID3D10ShaderResourceView **shaderResView, 
								 ID3D10Texture2D **texture)
{
  D3DX10_IMAGE_LOAD_INFO imgLoadInfo;
  D3DX10_IMAGE_INFO imginfo = {0};
  D3D10_TEXTURE2D_DESC desc;
  D3D10_SHADER_RESOURCE_VIEW_DESC SRVDesc;
  D3D10_TEXTURE2D_DESC dstex;
  ID3D10Resource *pRes = NULL;
  ID3D10Texture2D* pTemp = NULL;
  EH_DECLARE;

  ZeroMemory( &dstex, sizeof( D3D10_TEXTURE2D_DESC ) );
  dstex.Width = 128;
  dstex.Height = 128;
  dstex.MipLevels = 1;
  dstex.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
  dstex.Usage = D3D10_USAGE_DEFAULT;
  dstex.BindFlags = D3D10_BIND_SHADER_RESOURCE;
  dstex.CPUAccessFlags = 0;
  dstex.MiscFlags = 0;
  dstex.ArraySize = 1;
  EH_Test(D3DX10GetImageInfoFromMemory(textureData, size, NULL, &imginfo, NULL));

  ZeroMemory( &imgLoadInfo, sizeof(imgLoadInfo) );
  imgLoadInfo.Width = imginfo.Width;
  imgLoadInfo.Height = imginfo.Height;
  imgLoadInfo.BindFlags = D3D10_BIND_SHADER_RESOURCE;
  imgLoadInfo.CpuAccessFlags = D3D10_CPU_ACCESS_WRITE;
  imgLoadInfo.FirstMipLevel = 0;
  imgLoadInfo.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
  imgLoadInfo.MipFilter = D3DX10_FILTER_NONE;
  imgLoadInfo.MipLevels = 1;
  imgLoadInfo.pSrcInfo = &imginfo;
  imgLoadInfo.Usage = D3D10_USAGE_DYNAMIC ;
  EH_Test(D3DX10CreateTextureFromMemory(d3dDevice, textureData, size, &imgLoadInfo, NULL, &pRes, NULL));
  EH_Test(pRes->QueryInterface( __uuidof( ID3D10Texture2D ), (LPVOID*)&pTemp ));

  pTemp->GetDesc( &desc );

  *texture = pTemp;

  // Create the resource view
  ZeroMemory( &SRVDesc, sizeof(SRVDesc) );
  SRVDesc.Format = desc.Format;
  SRVDesc.ViewDimension = D3D10_SRV_DIMENSION_TEXTURE2D;
  SRVDesc.Texture2D.MipLevels = desc.MipLevels;
  SRVDesc.Texture2D.MostDetailedMip = 0;
  EH_Test(d3dDevice->CreateShaderResourceView( *texture, &SRVDesc, shaderResView ));

  EH_OnError() {}

  SAFE_RELEASE( pRes );
}




void OmniMapD3D10::CreateLogoSurface()
{
}




void OmniMapD3D10::DrawBG()
{
}
