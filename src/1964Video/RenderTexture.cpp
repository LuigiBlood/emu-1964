/*
Copyright (C) 2005 Rice1964

This program is free software; you can redistribute it and/or
modify it under the terms of the GNU General Public License
as published by the Free Software Foundation; either version 2
of the License, or (at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program; if not, write to the Free Software
Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.

*/

// ===========================================================================

#include "stdafx.h"
#include "glh_genext.h"


CDXRenderTexture::CDXRenderTexture(int width, int height, RenderTextureInfo* pInfo, TextureUsage usage)
: CRenderTexture(width, height, pInfo, usage)
{
	m_pTexture = new CDirectXTexture(width, height, usage);
	if( m_pTexture )
	{
		m_width = width;
		m_height = height;
	}
	else
	{
		TRACE0("Error to create DX render_texture");
		SAFE_DELETE(m_pTexture);
	}

	m_pColorBufferSave = NULL;
	m_pDepthBufferSave = NULL;
	m_beingRendered = false;
}

CDXRenderTexture::~CDXRenderTexture()
{
	if( m_beingRendered )
	{
		g_pFrameBufferManager->CloseRenderTexture(false);
		SetAsRenderTarget(false);
	}

	SAFE_DELETE(m_pTexture);

	m_beingRendered = false;
}

bool CDXRenderTexture::SetAsRenderTarget(bool enable)
{
	if( m_usage != AS_RENDER_TARGET )	return false;

	if( enable )
	{
		if( !m_beingRendered )
		{
			if(m_pTexture )
			{
				MYLPDIRECT3DSURFACE pColorBuffer;

				// save the current back buffer
#if DIRECTX_VERSION == 8
				g_pD3DDev->GetBackBuffer(0, D3DBACKBUFFER_TYPE_MONO, &m_pColorBufferSave);
#else
				g_pD3DDev->GetBackBuffer(0, 0, D3DBACKBUFFER_TYPE_MONO, &m_pColorBufferSave);
#endif
				g_pD3DDev->GetDepthStencilSurface(&m_pDepthBufferSave);

				// Activate the render_texture
				(MYLPDIRECT3DTEXTURE(m_pTexture->GetTexture()))->GetSurfaceLevel(0,&pColorBuffer);
#if DIRECTX_VERSION == 8
				HRESULT res = g_pD3DDev->SetRenderTarget(pColorBuffer, NULL);
#else
				HRESULT res = g_pD3DDev->SetRenderTarget(0, pColorBuffer);
#endif
				SAFE_RELEASE(pColorBuffer);
				if( res != S_OK )
				{
					return false;
				}

				m_beingRendered = true;
				return true;
			}
			else
			{
				return false;
			}
		}
		else
		{
			return true;
		}
	}
	else
	{
		if( m_beingRendered )
		{
			if( m_pColorBufferSave && m_pDepthBufferSave )
			{
#if DIRECTX_VERSION == 8
				g_pD3DDev->SetRenderTarget(m_pColorBufferSave, m_pDepthBufferSave);
#else
				g_pD3DDev->SetRenderTarget(0, m_pColorBufferSave);
#endif
				m_beingRendered = false;
				SAFE_RELEASE(m_pColorBufferSave);
				SAFE_RELEASE(m_pDepthBufferSave);
				return true;
			}
			else
			{
				return false;
			}
		}
		else
		{
			return true;
		}
	}
}

void CDXRenderTexture::LoadTexture(TxtrCacheEntry* pEntry)
{
	bool IsBeingRendered = m_beingRendered;
	if( IsBeingRendered )
	{
		TXTRBUF_DUMP(TRACE0("Warning, loading from render_texture while it is being rendered"));

		SetAsRenderTarget(false);
		//return;
	}

	CTexture *pSurf = pEntry->pTexture;
	TxtrInfo &ti = pEntry->ti;

	// Need to load texture from the saved back buffer

	MYLPDIRECT3DTEXTURE pNewTexture = MYLPDIRECT3DTEXTURE(pSurf->GetTexture());
	MYLPDIRECT3DSURFACE pNewSurface = NULL;
	HRESULT res = pNewTexture->GetSurfaceLevel(0,&pNewSurface);
	MYLPDIRECT3DSURFACE pSourceSurface = NULL;
	(MYLPDIRECT3DTEXTURE(m_pTexture->GetTexture()))->GetSurfaceLevel(0,&pSourceSurface);

	int left = (pEntry->ti.Address- m_pInfo->CI_Info.dwAddr )%m_pInfo->CI_Info.bpl + pEntry->ti.LeftToLoad;
	int top = (pEntry->ti.Address- m_pInfo->CI_Info.dwAddr)/m_pInfo->CI_Info.bpl + pEntry->ti.TopToLoad;
	RECT srcrect = {uint32(left*m_pInfo->scaleX) ,uint32(top*m_pInfo->scaleY), 
		uint32(min(m_width, left+(int)ti.WidthToLoad)*m_pInfo->scaleX), 
		uint32(min(m_height,top+(int)ti.HeightToLoad)*m_pInfo->scaleY) };

	if( pNewSurface != NULL && pSourceSurface != NULL )
	{
		if( left < m_width && top<m_height )
		{
			RECT dstrect = {0,0,ti.WidthToLoad,ti.HeightToLoad};
			HRESULT res = D3DXLoadSurfaceFromSurface(pNewSurface,NULL,&dstrect,pSourceSurface,NULL,&srcrect,D3DX_FILTER_POINT ,0xFF000000);
			DEBUGGER_IF_DUMP(( res != S_OK), {DebuggerAppendMsg("Error to reload texture from render_texture, res=%x", res);} );
		}
	}

	if( IsBeingRendered )
	{
		SetAsRenderTarget(true);
	}

	pSurf->SetOthersVariables();
	SAFE_RELEASE(pSourceSurface);
	TXTRBUF_DETAIL_DUMP(DebuggerAppendMsg("Load texture from render_texture"););
}

void CDXRenderTexture::StoreToRDRAM(int infoIdx)
{
	if( !frameBufferOptions.bRenderTextureWriteBack )	return;

	RenderTextureInfo &info = gRenderTextureInfos[infoIdx];
	DXFrameBufferManager &FBmgr = *(DXFrameBufferManager*)g_pFrameBufferManager;

	uint32 fmt = info.CI_Info.dwFormat;

	MYLPDIRECT3DSURFACE pSourceSurface = NULL;
	(MYLPDIRECT3DTEXTURE(m_pTexture->GetTexture()))->GetSurfaceLevel(0,&pSourceSurface);

	// Ok, we are using texture render target right now
	// Need to copy content from the texture render target back to frame buffer
	// then reset the current render target

	// Here we need to copy the content from the texture frame buffer to RDRAM memory

	TXTRBUF_DUMP(TRACE2("Saving TextureBuffer %d to N64 RDRAM addr=%08X", infoIdx, info.CI_Info.dwAddr));

	if( pSourceSurface )
	{
		uint32 width, height, bufWidth, bufHeight, memsize; 
		width = info.N64Width;
		height = info.N64Height;
		bufWidth = info.bufferWidth;
		bufHeight = info.bufferHeight;
		if( info.CI_Info.dwSize == TXT_SIZE_8b && fmt == TXT_FMT_CI )
		{
			info.CI_Info.dwFormat = TXT_FMT_I;
			height = info.knownHeight ? info.N64Height : info.maxUsedHeight;
			memsize = info.N64Width*height;
			FBmgr.CopyD3DSurfaceToRDRAM(info.CI_Info.dwAddr, fmt, info.CI_Info.dwSize, width, height,
				bufWidth, bufHeight, info.CI_Info.dwAddr, memsize, info.N64Width, D3DFMT_A8R8G8B8, pSourceSurface);
			info.CI_Info.dwFormat = TXT_FMT_CI;
		}
		else
		{
			if( info.CI_Info.dwSize == TXT_SIZE_8b )
			{
				height = info.knownHeight ? info.N64Height : info.maxUsedHeight;
				memsize = info.N64Width*height;
				FBmgr.CopyD3DSurfaceToRDRAM(info.CI_Info.dwAddr, fmt, info.CI_Info.dwSize, width, height,
					bufWidth, bufHeight, info.CI_Info.dwAddr, memsize, info.N64Width, D3DFMT_A8R8G8B8, pSourceSurface);
			}
			else
			{
				height = info.knownHeight ? info.N64Height : info.maxUsedHeight;
				memsize = g_pRenderTextureInfo->N64Width*height*2;
				FBmgr.CopyD3DSurfaceToRDRAM(info.CI_Info.dwAddr, fmt, info.CI_Info.dwSize, width, height,
					bufWidth, bufHeight, info.CI_Info.dwAddr, memsize, info.N64Width, D3DFMT_X8R8G8B8, pSourceSurface);
			}
		}
		TXTRBUF_DUMP(TRACE2("Write back: width=%d, height=%d", width, height));	

		SAFE_RELEASE(pSourceSurface);
	}
	else
	{
		TRACE0("Error, cannot lock the render_texture");
	}
}

#ifdef _DEBUG
void CDXRenderTexture::Display(int idx)
{
	MYLPDIRECT3DSURFACE pSourceSurface = NULL;
	(MYLPDIRECT3DTEXTURE(m_pTexture->GetTexture()))->GetSurfaceLevel(0,&pSourceSurface);
	char filename[256];
	sprintf(filename,"\\DxTxtBuf%d",idx);
	((CDXGraphicsContext*)CGraphicsContext::g_pGraphicsContext)->SaveSurfaceToFile(filename, pSourceSurface);
	SAFE_RELEASE(pSourceSurface);
}
#endif

COGLRenderTexture::COGLRenderTexture(int width, int height, RenderTextureInfo* pInfo, TextureUsage usage)
	:	CRenderTexture(width, height, pInfo, usage),
		m_hBuffer(NULL), m_hRC(NULL), m_hDC(NULL), m_pOGLTexture(NULL)
{
	if( usage == AS_BACK_BUFFER_SAVE )
	{
		m_pTexture = m_pOGLTexture = new COGLTexture(width, height, usage);
		if( !m_pTexture )
		{
			TRACE0("Error to create DX render_texture");
			SAFE_DELETE(m_pTexture);
		}
	}

	m_width = width;
	m_height = height;
	m_beingRendered = false;
}

COGLRenderTexture::~COGLRenderTexture()
{
	if( m_beingRendered )
	{
		g_pFrameBufferManager->CloseRenderTexture(false);
		SetAsRenderTarget(false);
	}

	ShutdownPBuffer();
	SAFE_DELETE(m_pTexture);
	m_pOGLTexture = NULL;
	m_beingRendered = false;
}

static unsigned bit[0x100];

inline unsigned __stdcall nextpow2(unsigned x)
{
	--x;
	int shift = 0;
	if( x & 0xFFFF0000)
	{
		x >>= 16;
		shift = 16;
	}
	if( x & 0xFF00)
	{
		x >>= 8;
		shift += 8;
	}
	return bit[x] << shift;

}

bool COGLRenderTexture::InitPBuffer( void )
{
	//Check for pbuffer support
	if(	!wglChoosePixelFormatARB || !wglCreatePbufferARB )
	{
		TRACE0("Extension required for pbuffer unsupported");
		return false;
	}

	//Get the current device context
	HDC hCurrentDC=wglGetCurrentDC();
	if(!hCurrentDC)
	{
		TRACE0("Unable to get current Device Context");
		return false;
	}

	const int iAttribList[]={	WGL_DRAW_TO_PBUFFER_ARB, 1,
		WGL_COLOR_BITS_ARB, options.colorQuality == TEXTURE_FMT_A4R4G4B4 ? 16 : 32,
		WGL_ALPHA_BITS_ARB, options.colorQuality == TEXTURE_FMT_A4R4G4B4 ? 1 : 8,
		WGL_DEPTH_BITS_ARB, options.OpenglDepthBufferSetting,
		WGL_STENCIL_BITS_ARB, 0,
		WGL_BIND_TO_TEXTURE_RGBA_ARB, true,
		0 };
	const float fAttribList[]={0};
	int flags[]={WGL_TEXTURE_FORMAT_ARB, WGL_TEXTURE_RGBA_ARB, WGL_TEXTURE_TARGET_ARB, WGL_TEXTURE_2D_ARB, 0};

	//choose pixel format
	GLint pixelFormat;
	unsigned int numFormats;
	if(!wglChoosePixelFormatARB(hCurrentDC, iAttribList, fAttribList, 1, &pixelFormat, &numFormats))
	{
		TRACE0("Unable to find a pixel format for the pbuffer");
		return false;
	}

	//Create the pbuffer
	// Make the width and height be the power of 2
	
	m_widthCreated = nextpow2(m_widthCreated);
	m_heightCreated = nextpow2(m_heightCreated);

	m_hBuffer=wglCreatePbufferARB(hCurrentDC, pixelFormat, m_widthCreated, m_heightCreated, flags);
	TXTRBUF_DUMP(TRACE2("Pbuffer to create: (%d x %d)", m_widthCreated, m_heightCreated));

	if(!m_hBuffer)
	{
		DWORD error = GetLastError();
//		TRACE1("Unable to create pbuffer: %s", gluErrorString(glGetError()));
		return false;
	}

	//Get the pbuffer's device context
	m_hDC=wglGetPbufferDCARB(m_hBuffer);
	if(!m_hDC)
	{
		TRACE0("Unable to get pbuffer's device context");
		return false;
	}

	//Create a rendering context for the pbuffer
	m_hRC=wglCreateContext(m_hDC);
	if(!m_hRC)
	{
		TRACE0("Unable to create pbuffer's rendering context");
		return false;
	}

	//Set and output the actual pBuffer dimensions
	int realwidth, realheight;
	wglQueryPbufferARB(m_hBuffer, WGL_PBUFFER_WIDTH_ARB, &realwidth);
	wglQueryPbufferARB(m_hBuffer, WGL_PBUFFER_HEIGHT_ARB, &realheight);
	TXTRBUF_DUMP(TRACE2("Pbuffer Created: (%d x %d)", realwidth, realheight));

	return true;
}

void COGLRenderTexture::ShutdownPBuffer(void)
{
	if(m_hRC)
	{
		if(!wglDeleteContext(m_hRC))
			TRACE0("Release of Pbuffer Rendering Context Failed.");
		m_hRC=NULL;
	}

	if(m_hDC)
	{
		if(!wglReleasePbufferDCARB(m_hBuffer, m_hDC))
			TRACE0("Release of Pbuffer Device Context Failed.");
		m_hDC=NULL;
	}

	if(m_hBuffer)
	{
		if(!wglDestroyPbufferARB(m_hBuffer))
		{
			TRACE0("Unable to destroy pbuffer");
		}
		m_hBuffer = NULL;
	}
}

bool COGLRenderTexture::SetAsRenderTarget(bool enable)
{
	if( m_usage == AS_BACK_BUFFER_SAVE )	return false;
	if( enable )
	{
		if( !m_beingRendered )
		{
			 if( !frameBufferOptions.bIgnore )
			 {
				if( !m_hBuffer )
				{
					ShutdownPBuffer();
					if( !InitPBuffer() )	// Create pBuffer
						return false;
				}

				m_savedDC = wglGetCurrentDC();;
				m_savedRC = wglGetCurrentContext();

				if( !wglMakeCurrent(m_hDC,m_hRC) )	// set render target to the new pBuffer
				{
					TRACE0("Cannot render to pBuffer");
					return false;
				}
			 }

			m_beingRendered = true;

			// Initialize rendering properties
		}

		return true;
	}
	else
	{
		if( m_beingRendered )
		{
			if( !frameBufferOptions.bIgnore )
			{
				if( !wglMakeCurrent(m_savedDC,m_savedRC) )	// Set render target back to normal target
				{
					TRACE0("Cannot go back to normal back buffer from pbuffer");
					return false;
				}
			}
			m_beingRendered = false;
		}
		return true;
	}
}

void COGLRenderTexture::LoadTexture(TxtrCacheEntry* pEntry)
{
	// Load texture from pBuffer to the new texture

	bool IsBeingRendered = m_beingRendered;
	if( IsBeingRendered )
	{
		TXTRBUF_DUMP(TRACE0("Warning, loading from render_texture while it is being rendered"));
		SetAsRenderTarget(false);
		//return;
	}

	CTexture *pSurf = pEntry->pTexture;
	TxtrInfo &ti = pEntry->ti;

	// Need to load texture from the saved back buffer

	int left = (pEntry->ti.Address- m_pInfo->CI_Info.dwAddr )%m_pInfo->CI_Info.bpl + pEntry->ti.LeftToLoad;
	int top = (pEntry->ti.Address- m_pInfo->CI_Info.dwAddr)/m_pInfo->CI_Info.bpl + pEntry->ti.TopToLoad;
	RECT srcrect = {uint32(left*m_pInfo->scaleX) ,uint32(top*m_pInfo->scaleY), 
		uint32(min(m_width, left+(int)ti.WidthToLoad)*m_pInfo->scaleX), 
		uint32(min(m_height,top+(int)ti.HeightToLoad)*m_pInfo->scaleY) };

	if( m_usage == AS_RENDER_TARGET )
	{
		// Fix me here, the scales are not right
		SetAsRenderTarget(true);
		glEnable(GL_TEXTURE_2D);
		glBindTexture(GL_TEXTURE_2D, ((COGLTexture*)(pEntry->pTexture))->m_dwTextureName);
		glCopyTexSubImage2D(GL_TEXTURE_2D,0,0,0,srcrect.left,srcrect.top,pEntry->pTexture->m_dwWidth,pEntry->pTexture->m_dwHeight);
		SetAsRenderTarget(false);
	}
	else
	{
		// As a back buffer save
	}

	if( IsBeingRendered )
	{
		SetAsRenderTarget(true);
	}

	pSurf->SetOthersVariables();
	TXTRBUF_DETAIL_DUMP(DebuggerAppendMsg("Load texture from render_texture"););
}

void COGLRenderTexture::StoreToRDRAM(int infoIdx)
{
	if( m_usage == AS_BACK_BUFFER_SAVE )
	{
		TXTRBUF_DUMP(TRACE1("Should not save a saved backbuffer, check me here", infoIdx));
		return;
	}

	if( !m_hBuffer )
	{
		TRACE0("No pBuffer has been created, cannot save to RDRAM");
		return;
	}

	int realwidth, realheight;
	wglQueryPbufferARB(m_hBuffer, WGL_PBUFFER_WIDTH_ARB, &realwidth);
	wglQueryPbufferARB(m_hBuffer, WGL_PBUFFER_HEIGHT_ARB, &realheight);

	unsigned char *buffer = new unsigned char[realwidth*realheight*4+2000];
	if( !buffer )
	{
		TRACE0("No memory to process the render texture");
		return;
	}

	RenderTextureInfo &info = gRenderTextureInfos[infoIdx];

	uint32 fmt = info.CI_Info.dwFormat;

	m_savedDC = wglGetCurrentDC();;
	m_savedRC = wglGetCurrentContext();
	wglMakeCurrent(m_hDC,m_hRC);	// set render target to the new pBuffer

	glReadPixels(0,0,m_width,m_height,GL_RGBA,GL_UNSIGNED_BYTE,buffer);
	wglMakeCurrent(m_savedDC,m_savedRC);	// Set render target back to normal target

	TXTRBUF_DUMP(TRACE2("Saving TextureBuffer %d to N64 RDRAM addr=%08X", infoIdx, info.CI_Info.dwAddr));
	//SaveRGBBufferToFile(filename,buffer,m_width, m_height);

	// Save the buffer to RDRAM

	uint32 width, height, bufWidth, bufHeight, memsize; 
	width = info.N64Width;
	height = info.N64Height;
	bufWidth = info.bufferWidth;
	bufHeight = info.bufferHeight;
	TextureFmt bufFmt = TEXTURE_FMT_A8R8G8B8;
	if( info.CI_Info.dwSize == TXT_SIZE_8b && fmt == TXT_FMT_CI )
	{
		info.CI_Info.dwFormat = TXT_FMT_I;
		height = info.knownHeight ? info.N64Height : info.maxUsedHeight;
		memsize = info.N64Width*height;
		g_pFrameBufferManager->CopyBufferToRDRAM(info.CI_Info.dwAddr, fmt, info.CI_Info.dwSize, width, height, bufWidth, bufHeight, 
			info.CI_Info.dwAddr, memsize, info.N64Width, bufFmt, buffer, m_width*4);
		info.CI_Info.dwFormat = TXT_FMT_CI;
	}
	else
	{
		if( info.CI_Info.dwSize == TXT_SIZE_8b )
		{
			height = info.knownHeight ? info.N64Height : info.maxUsedHeight;
			memsize = info.N64Width*height;
			g_pFrameBufferManager->CopyBufferToRDRAM(info.CI_Info.dwAddr, fmt, info.CI_Info.dwSize, width, height, bufWidth, bufHeight, 
				info.CI_Info.dwAddr, memsize, info.N64Width, bufFmt, buffer, m_width*4);
		}
		else
		{
			height = info.knownHeight ? info.N64Height : info.maxUsedHeight;
			memsize = g_pRenderTextureInfo->N64Width*height*2;
			//CopyD3DSurfaceToRDRAM(info.CI_Info.dwAddr, fmt, info.CI_Info.dwSize, width, height,
			//	bufWidth, bufHeight, info.CI_Info.dwAddr, memsize, info.N64Width, D3DFMT_X8R8G8B8, pSourceSurface);
			g_pFrameBufferManager->CopyBufferToRDRAM(info.CI_Info.dwAddr, fmt, info.CI_Info.dwSize, width, height, 
				bufWidth, bufHeight, info.CI_Info.dwAddr, memsize, info.N64Width, bufFmt, buffer, m_width*4);
		}
	}

	delete [] buffer;
}


#ifdef _DEBUG
bool SaveOGLTextureToFile(char* filename, GLuint name, TextureChannel channel, bool show)
{
	int width, height;
	glEnable(GL_TEXTURE_2D);
	glBindTexture(GL_TEXTURE_2D, name);
	glGetTexLevelParameteriv(GL_TEXTURE_2D,0,GL_TEXTURE_WIDTH,&width);
	glGetTexLevelParameteriv(GL_TEXTURE_2D,0,GL_TEXTURE_HEIGHT,&height);
	unsigned char *buffer = (unsigned char*)malloc( width * height * 3 +1000);

	if( buffer )
	{
		if( channel == TXT_ALPHA )
		{
			glGetTexImage(GL_TEXTURE_2D, 0, GL_ALPHA, GL_UNSIGNED_BYTE, buffer);
			for( int y=height-1; y>=0; y-- )
				for( int x=width-1; x>=0; x--)
				{
					buffer[y*width*3+x*3+2] = buffer[y*width+x];
					buffer[y*width*3+x*3+1] = buffer[y*width+x];
					buffer[y*width*3+x*3  ] = buffer[y*width+x];
				}
		}
		else
			glGetTexImage(GL_TEXTURE_2D, 0, GL_BGR_EXT, GL_UNSIGNED_BYTE, buffer);

		SaveRGBBufferToFile(filename,buffer,width, height);
		free( buffer );

		if( show )
			ShellExecute(g_GraphicsInfo.hWnd, "open", filename, NULL, NULL, SW_SHOWNORMAL);
		return true;
	}
	else
		return false;
}

void COGLRenderTexture::Display(int idx)
{
	char filename[256];
	sprintf(filename,"\\texture_buffer_%d", idx);

	if( m_usage == AS_RENDER_TARGET )
	{
		if( m_hBuffer )
		{
			//Set and output the actual pBuffer dimensions
			//int realwidth, realheight;
			//wglQueryPbufferARB(m_hBuffer, WGL_PBUFFER_WIDTH_ARB, &realwidth);
			//wglQueryPbufferARB(m_hBuffer, WGL_PBUFFER_HEIGHT_ARB, &realheight);

			//unsigned char *buffer = new unsigned char[realwidth*realheight*3+2000];
			unsigned char *buffer = new unsigned char[m_widthCreated*m_heightCreated*3+2000];
			
			if( buffer )
			{
				glEnable(GL_TEXTURE_2D);
				if( wglBindTexImageARB(m_hBuffer,WGL_BACK_LEFT_ARB) )
				{
					glGetTexImage(GL_TEXTURE_2D,0,GL_BGR_EXT, GL_UNSIGNED_BYTE, buffer);
					wglReleaseTexImageARB(m_hBuffer,WGL_BACK_LEFT_ARB);
					SaveRGBBufferToFile(filename,buffer,m_widthCreated, m_heightCreated);
					ShellExecute(g_GraphicsInfo.hWnd, "open", filename, NULL, NULL, SW_SHOWNORMAL);
				}
				else
				{
					TRACE1("Cannot bind pBuffer, error = %08X", GetLastError());
				}

				delete [] buffer;
			}
			else
			{
				TRACE0("No memory to display the render texture");
			}
		}
		else
		{
			TRACE0("No pBuffer has been created, cannot display");
		}
	}
	else
	{
		// AS_BACKBUFFER_SAVE
		SaveOGLTextureToFile(filename, m_pOGLTexture->m_dwTextureName, TXT_RGB, true);
	}
}
#endif
