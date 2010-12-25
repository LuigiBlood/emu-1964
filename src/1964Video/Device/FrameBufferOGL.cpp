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

#include "stdafx.h"


void OGLFrameBufferManager::CopyBackBufferToRenderTexture(int idx, RecentCIInfo &ciInfo, RECT* pDstRect)			// Copy the current back buffer to temp buffer
{
	GLfloat scaleX = 1.0f;
	GLfloat scaleY = 1.0f;

	if( defaultRomOptions.bInN64Resolution )
	{
		scaleX = (float)ciInfo.dwWidth/windowSetting.uDisplayWidth;
		scaleY = (float)ciInfo.dwHeight/windowSetting.uDisplayHeight;
	}

	glPixelZoom(scaleX,scaleY);

	RECT srcrect = {0,0,windowSetting.uDisplayWidth -1,windowSetting.uDisplayHeight-1};
	if( pDstRect )
	{
		srcrect.left = uint32(pDstRect->left*scaleX);
		srcrect.top = uint32(pDstRect->top*scaleY);
		srcrect.right = uint32(pDstRect->right*scaleX);
		srcrect.bottom = uint32(pDstRect->bottom*scaleY);
	}

	int dstleft = pDstRect ? pDstRect->left : 0;
	int dsttop = pDstRect ? pDstRect->top : 0;

	glEnable(GL_TEXTURE_2D);
	glBindTexture(GL_TEXTURE_2D, ((COGLTexture*)(gRenderTextureInfos[idx].pRenderTexture->m_pTexture->GetTexture()))->m_dwTextureName);
	glCopyTexSubImage2D(GL_TEXTURE_2D,0,dstleft,dsttop,srcrect.left,srcrect.top,srcrect.right-srcrect.left,srcrect.bottom-srcrect.top);
}

void OGLFrameBufferManager::StoreBackBufferToRDRAM(uint32 addr, uint32 fmt, uint32 siz, uint32 width, 
	uint32 height, uint32 bufWidth, uint32 bufHeight, uint32 startaddr, 
	uint32 memsize, uint32 pitch, D3DFORMAT surf_fmt)
{
	// Get a copy of the back buffer
	unsigned char *buffer = (unsigned char*)malloc( windowSetting.uDisplayWidth * windowSetting.uDisplayHeight * 4 +2000 );
	if( !buffer )
	{
		TRACE0("No memory to save back buffer");
		return;
	}

	glReadBuffer( GL_FRONT );
	glReadPixels( 0, windowSetting.statusBarHeightToUse, windowSetting.uDisplayWidth, windowSetting.uDisplayHeight, GL_BGR_EXT, GL_UNSIGNED_BYTE, buffer );
	glReadBuffer( GL_BACK );


	TXTRBUF_DUMP(TRACE1("Saving back buffer to N64 RDRAM addr=%08X", addr));

	// Save the buffer to RDRAM
	g_pFrameBufferManager->CopyBufferToRDRAM(addr, fmt, siz, width, height, bufWidth, bufHeight, 
		startaddr, memsize, pitch, TEXTURE_FMT_A8R8G8B8, buffer, windowSetting.uDisplayWidth*4);

	free( buffer );
}
