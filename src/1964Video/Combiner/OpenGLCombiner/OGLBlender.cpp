/*
Copyright (C) 2003-2009 Rice1964

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

//========================================================================
uint32 DirectX_OGL_BlendFuncMaps [] =
{
	GL_SRC_ALPHA,		//Nothing
    GL_ZERO,			//D3DBLEND_ZERO               = 1,
    GL_ONE,				//D3DBLEND_ONE                = 2,
    GL_SRC_COLOR,		//D3DBLEND_SRCCOLOR           = 3,
    GL_ONE_MINUS_SRC_COLOR,		//D3DBLEND_INVSRCCOLOR        = 4,
    GL_SRC_ALPHA,				//D3DBLEND_SRCALPHA           = 5,
    GL_ONE_MINUS_SRC_ALPHA,		//D3DBLEND_INVSRCALPHA        = 6,
    GL_DST_ALPHA,				//D3DBLEND_DESTALPHA          = 7,
    GL_ONE_MINUS_DST_ALPHA,		//D3DBLEND_INVDESTALPHA       = 8,
    GL_DST_COLOR,				//D3DBLEND_DESTCOLOR          = 9,
    GL_ONE_MINUS_DST_COLOR,		//D3DBLEND_INVDESTCOLOR       = 10,
    GL_SRC_ALPHA_SATURATE,		//D3DBLEND_SRCALPHASAT        = 11,
    GL_SRC_ALPHA_SATURATE,		//D3DBLEND_BOTHSRCALPHA       = 12,	
    GL_SRC_ALPHA_SATURATE,		//D3DBLEND_BOTHINVSRCALPHA    = 13,
};

//========================================================================

COGLBlender::COGLBlender(CRender *pRender) 
	: CBlender(pRender),m_pOGLRender((OGLRender*)pRender) 
{
}

void COGLBlender::NormalAlphaBlender(void)
{
	glEnable(GL_BLEND);
	OPENGL_CHECK_ERRORS;
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
	OPENGL_CHECK_ERRORS;
}

void COGLBlender::DisableAlphaBlender(void)
{
	glEnable(GL_BLEND);
	OPENGL_CHECK_ERRORS;
	glBlendFunc(GL_ONE, GL_ZERO);
	OPENGL_CHECK_ERRORS;
}

void COGLBlender::BlendFunc(uint32 srcFunc, uint32 desFunc)
{
	glBlendFunc(DirectX_OGL_BlendFuncMaps[srcFunc], DirectX_OGL_BlendFuncMaps[desFunc]);
	OPENGL_CHECK_ERRORS;
}

void COGLBlender::Enable()
{
	glEnable(GL_BLEND);
	OPENGL_CHECK_ERRORS;
}

void COGLBlender::Disable()
{
	glDisable(GL_BLEND);
	OPENGL_CHECK_ERRORS;
}

