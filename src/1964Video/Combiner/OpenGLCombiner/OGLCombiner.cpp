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
#include "../../glh_genext.h"


COGLFragmentShaderCombiner::COGLFragmentShaderCombiner(CRender *pRender) :
	CColorCombiner(pRender),
	m_pOGLRender((OGLRender*)pRender)
{
	m_pDecodedMux = new DecodedMuxForPixelShader;
	m_pDecodedMux->m_maxConstants = 0;
	m_pDecodedMux->m_maxTextures = 1;
}

COGLFragmentShaderCombiner::~COGLFragmentShaderCombiner()
{
	int size = m_vCompiledShaders.size();
	for (int i=0; i<size; i++)
	{
		GLuint ID = m_vCompiledShaders[i].programID;
		glDeleteProgramsARB(1, &ID);
		OPENGL_CHECK_ERRORS;
		m_vCompiledShaders[i].programID = 0;
	}

	m_vCompiledShaders.clear();
}

bool COGLFragmentShaderCombiner::Initialize(void)
{
	m_supportedStages = 1;
	m_bSupportMultiTexture = true;

	COGLGraphicsContext *pcontext = (COGLGraphicsContext *)(CGraphicsContext::g_pGraphicsContext);
	if( pcontext->IsExtensionSupported("GL_ARB_fragment_shader") )
	{
		glGetIntegerv(GL_MAX_TEXTURE_UNITS_ARB,&m_maxTexUnits);
		OPENGL_CHECK_ERRORS;
		if( m_maxTexUnits > 8 ) m_maxTexUnits = 8;

		TRACE0("Starting Ogl Fragment Shader multitexture combiner" );
		TRACE1("m_maxTexUnits = %d", m_maxTexUnits);
	}
	else
	{
		ErrorMsg("Your video card does not support GL_ARB_fragment_shader, Rice Video will not function properly");
	}

	m_supportedStages = m_maxTexUnits;

	return true;
}

void COGLFragmentShaderCombiner::DisableCombiner(void)
{
	m_pOGLRender->DisableMultiTexture();
	glEnable(GL_BLEND);
	OPENGL_CHECK_ERRORS;
	glBlendFunc(GL_ONE, GL_ZERO);
	OPENGL_CHECK_ERRORS;

	if( m_bTexelsEnable )
	{
		COGLTexture* pTexture = g_textures[gRSP.curTile].m_pCOGLTexture;
		if( pTexture ) 
		{
			m_pOGLRender->EnableTexUnit(0,TRUE);
			m_pOGLRender->BindTexture(pTexture->m_dwTextureName, 0);
			glTexEnvi(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_MODULATE);
			OPENGL_CHECK_ERRORS;
			m_pOGLRender->SetAllTexelRepeatFlag();
		}
#ifdef _DEBUG
		else
		{
			DebuggerAppendMsg("Check me, texture is NULL but it is enabled");
		}
#endif
	}
	else
	{
		glTexEnvi(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_MODULATE);
		OPENGL_CHECK_ERRORS;
		m_pOGLRender->EnableTexUnit(0,FALSE);
	}
}

void COGLFragmentShaderCombiner::InitCombinerCycleCopy(void)
{
	m_pOGLRender->DisableMultiTexture();
	m_pOGLRender->EnableTexUnit(0,TRUE);
	COGLTexture* pTexture = g_textures[gRSP.curTile].m_pCOGLTexture;
	if( pTexture )
	{
		m_pOGLRender->BindTexture(pTexture->m_dwTextureName, 0);
		m_pOGLRender->SetTexelRepeatFlags(gRSP.curTile);
	}
#ifdef _DEBUG
	else
	{
		DebuggerAppendMsg("Check me, texture is NULL");
	}
#endif

	glTexEnvi(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_REPLACE);
	OPENGL_CHECK_ERRORS;
}

void COGLFragmentShaderCombiner::InitCombinerCycleFill(void)
{
	glDisable(GL_FRAGMENT_PROGRAM_ARB);
	OPENGL_CHECK_ERRORS;

	for( int i=0; i<m_supportedStages; i++ )
	{
		glActiveTextureARB(GL_TEXTURE0_ARB+i);
		OPENGL_CHECK_ERRORS;
		m_pOGLRender->EnableTexUnit(i,FALSE);
	}
}

const char *muxToFP_Maps[][2] = {
	//color -- alpha
	{"0", "0"}, //MUX_0 = 0,
	{"1", "1"}, //MUX_1,
	{"comb", "comb.a"}, //MUX_COMBINED,
	{"t0", "t0.a"}, //MUX_TEXEL0,
	{"t1", "t1.a"}, //MUX_TEXEL1,
	{"program.env[2]", "program.env[2].a"}, //MUX_PRIM,
	{"fragment.color", "fragment.color.a"}, //MUX_SHADE,
	{"program.env[1]", "program.env[1].a"}, //MUX_ENV,
	{"comb.a", "comb.a"}, //MUX_COMBALPHA,
	{"t0.a", "t0.a"}, //MUX_T0_ALPHA,   
	{"t1.a", "t1.a"}, //MUX_T1_ALPHA,   
	{"primcolor.a", "primcolor.a"}, //MUX_PRIM_ALPHA,   
	{"fragment.color.a", "fragment.color.a"}, //MUX_SHADE_ALPHA,    
	{"envcolor.a", "envcolor.a"}, //MUX_ENV_ALPHA,  
	{"program.env[3]", "program.env[3]"}, //MUX_LODFRAC,
	{"program.env[4]", "program.env[4]"}, //MUX_PRIMLODFRAC,
	{"1", "1"}, //MUX_K5,
	{"1", "1"}, //MUX_UNK,  // Should not be used
};


const char *oglFPTest = 
"!!ARBfp1.0\n"
"#Declarations\n"
"TEMP t0;\n"
"TEMP t1;\n"
"TEMP comb;\n"
"TEMP comb2;\n"
"\n"
"ATTRIB coord0 = fragment.texcoord[0];\n"
"ATTRIB coord1 = fragment.texcoord[1];\n"
"ATTRIB shade = fragment.color;\n"
"ATTRIB fogfactor = fragment.fogcoord;\n"
"\n"
"OUTPUT out = result.color;\n"
"\n"
"#Instructions\n"
"TEX t0, coord0, texture[0], 2D;\n"
"TEX t1, coord1, texture[1], 2D;\n"
"\n"
"MAD_SAT out, t0, program.env[1],program.env[0];\n"
//"SUB comb.rgb, 	t0, 0;\n"
//"MAD_SAT out.rgb, comb, program.env[1], 0;\n"
//"SUB comb.a,   	t0, 0;\n"
//"MAD_SAT out.a,   comb, program.env[1], 0;\n"
"END\n";

char oglNewFP[4092];

char* MuxToOC(uint8 val)
{
	// For color channel
	if( val&MUX_ALPHAREPLICATE )
		return (char*)muxToFP_Maps[val&0x1F][1];
	else
		return (char*)muxToFP_Maps[val&0x1F][0];
}

char* MuxToOA(uint8 val)
{
	// For alpha channel
	return (char*)muxToFP_Maps[val&0x1F][0];
}

void COGLFragmentShaderCombiner::GenerateProgramStr()
{
	DecodedMuxForPixelShader &mux = *(DecodedMuxForPixelShader*)m_pDecodedMux;

	mux.splitType[0] = mux.splitType[1] = mux.splitType[2] = mux.splitType[3] = CM_FMT_TYPE_NOT_CHECKED;
	m_pDecodedMux->Reformat(false);

	char *leadstr = "!!ARBfp1.0\n"
		"#Declarations\n"
		"%s\n" //"OPTION ARB_fog_linear;\n"
		"TEMP t0;\n"
		"TEMP t1;\n"
		"TEMP comb;\n"
		"TEMP comb2;\n"
		"\n"
		"ATTRIB coord0 = fragment.texcoord[0];\n"
		"ATTRIB coord1 = fragment.texcoord[1];\n"
		"ATTRIB shade = fragment.color;\n"
		"\n"
		"OUTPUT out = result.color;\n"
		"\n"
		"#Instructions\n"
		"TEX t0, coord0, texture[0], 2D;\n"
		"TEX t1, coord1, texture[1], 2D;\n"
		"\n"
		"# N64 cycle 1, result is in comb\n";

	// New solution
	bool bFog = gRDP.bFogEnableInBlender && gRSP.bFogEnabled;
	oglNewFP[0] = 0;
	char tempstr[500];

	sprintf(oglNewFP, leadstr, bFog?"OPTION ARB_fog_linear;":"\n");
	for( int cycle=0; cycle<2; cycle++ )
	{
		for( int channel=0; channel<2; channel++)
		{
			char* (*func)(uint8) = channel==0?MuxToOC:MuxToOA;
			char *dst = channel==0?"rgb":"a";
			N64CombinerType &m = mux.m_n64Combiners[cycle*2+channel];
			switch( mux.splitType[cycle*2+channel] )
			{
			case CM_FMT_TYPE_NOT_USED:
				sprintf(tempstr, "\n");
				break;
			case CM_FMT_TYPE_D:
				sprintf(tempstr, "MOV comb.%s, %s;\n", dst, func(m.d));
				break;
			case CM_FMT_TYPE_A_MOD_C:
				sprintf(tempstr, "MUL comb.%s, %s, %s;\n", dst, func(m.a), func(m.c));
				break;
			case CM_FMT_TYPE_A_ADD_D:
				sprintf(tempstr, "ADD_SAT comb.%s, %s, %s;\n", dst, func(m.a), func(m.d));
				break;
			case CM_FMT_TYPE_A_SUB_B:
				sprintf(tempstr, "SUB comb.%s, %s, %s;\n", dst, func(m.a), func(m.b));
				break;
			case CM_FMT_TYPE_A_MOD_C_ADD_D:
				sprintf(tempstr, "MAD_SAT comb.%s, %s, %s, %s;\n", dst, func(m.a), func(m.c), func(m.d));
				break;
			case CM_FMT_TYPE_A_LERP_B_C:
				sprintf(tempstr, "LRP_SAT comb.%s, %s, %s, %s;\n", dst, func(m.c), func(m.a), func(m.b));
				//sprintf(tempstr, "SUB comb.%s, %s, %s;\nMAD_SAT comb.%s, comb, %s, %s;\n", dst, func(m.a), func(m.b), dst, func(m.c), func(m.b));
				break;
			default:
				sprintf(tempstr, "SUB comb2.%s, %s, %s;\nMAD_SAT comb.%s, comb2, %s, %s;\n", dst, func(m.a), func(m.b), dst, func(m.c), func(m.d));
				break;
			}

			strcat(oglNewFP, tempstr);
		}
	}

	strcat(oglNewFP, "MOV out, comb;\nEND\n\n");
}

int COGLFragmentShaderCombiner::ParseDecodedMux()
{
	OGLShaderCombinerSaveType res;

	
	glGenProgramsARB( 1, &res.programID);
	OPENGL_CHECK_ERRORS;
	GenerateProgramStr();
	glBindProgramARB( GL_FRAGMENT_PROGRAM_ARB, res.programID);

	glProgramStringARB(	GL_FRAGMENT_PROGRAM_ARB, GL_PROGRAM_FORMAT_ASCII_ARB, strlen(oglNewFP), oglNewFP);
	OPENGL_CHECK_ERRORS;
	//glProgramStringARB(	GL_FRAGMENT_PROGRAM_ARB, GL_PROGRAM_FORMAT_ASCII_ARB, strlen(oglFPTest), oglFPTest);

	if (glGetError() !=	0)
	{OPENGL_CHECK_ERRORS;
		GLint	position;
#ifdef _DEBUG
			char *str = (char*)glGetString(GL_PROGRAM_ERROR_STRING_ARB);
#endif
		glGetIntegerv( GL_PROGRAM_ERROR_POSITION_ARB, &position);
		if( position >= 0 )
		{
#ifdef _DEBUG
			if( m_lastIndex >= 0 ) COGLFragmentShaderCombiner4::DisplaySimpleMuxString();
			ErrorMsg("%s\n\n%s\n", str, oglNewFP+position);
#endif
			glDisable(GL_FRAGMENT_PROGRAM_ARB);
			return 0;
		}
	}

	glEnable(GL_FRAGMENT_PROGRAM_ARB);
	OPENGL_CHECK_ERRORS;
	res.dwMux0 = m_pDecodedMux->m_dwMux0;
	res.dwMux1 = m_pDecodedMux->m_dwMux1;
	res.fogIsUsed = gRDP.bFogEnableInBlender && gRSP.bFogEnabled;

	m_vCompiledShaders.push_back(res);
	m_lastIndex = m_vCompiledShaders.size()-1;

	return m_lastIndex;
}

void COGLFragmentShaderCombiner::GenerateCombinerSetting(int index)
{
	GLuint ID = m_vCompiledShaders[index].programID;
	glBindProgramARB( GL_FRAGMENT_PROGRAM_ARB, ID );
	OPENGL_CHECK_ERRORS;
	glEnable(GL_FRAGMENT_PROGRAM_ARB);
	OPENGL_CHECK_ERRORS;
}

void COGLFragmentShaderCombiner::GenerateCombinerSettingConstants(int index)
{
	float *pf;
	pf = GetEnvColorfv();
	glProgramEnvParameter4fvARB(GL_FRAGMENT_PROGRAM_ARB, 1, pf);
	OPENGL_CHECK_ERRORS;
	pf = GetPrimitiveColorfv();
	glProgramEnvParameter4fvARB(GL_FRAGMENT_PROGRAM_ARB, 2, pf);
	OPENGL_CHECK_ERRORS;

	float frac = gRDP.LODFrac / 255.0f;
	float tempf[4] = {frac,frac,frac,frac};
	glProgramEnvParameter4fvARB(GL_FRAGMENT_PROGRAM_ARB, 3, tempf);
	OPENGL_CHECK_ERRORS;

	float frac2 = gRDP.primLODFrac / 255.0f;
	float tempf2[4] = {frac2,frac2,frac2,frac2};
	glProgramEnvParameter4fvARB(GL_FRAGMENT_PROGRAM_ARB, 4, tempf2);
	OPENGL_CHECK_ERRORS;

	float tempf3[4] = {0,0,0,0};
	glProgramEnvParameter4fvARB(GL_FRAGMENT_PROGRAM_ARB, 0, tempf3);
	OPENGL_CHECK_ERRORS;
	float tempf4[4] = {1.0f,1.0f,1.0f,1.0f};
	glProgramEnvParameter4fvARB(GL_FRAGMENT_PROGRAM_ARB, 6, tempf3);
	OPENGL_CHECK_ERRORS;
}

int COGLFragmentShaderCombiner::FindCompiledMux()
{
#ifdef _DEBUG
	if( debuggerDropCombiners )
	{
		m_vCompiledShaders.clear();
		//m_dwLastMux0 = m_dwLastMux1 = 0;
		debuggerDropCombiners = false;
	}
#endif
	for( uint32 i=0; i<m_vCompiledShaders.size(); i++ )
	{
		if( m_vCompiledShaders[i].dwMux0 == m_pDecodedMux->m_dwMux0 
			&& m_vCompiledShaders[i].dwMux1 == m_pDecodedMux->m_dwMux1 
			&& m_vCompiledShaders[i].fogIsUsed == (gRDP.bFogEnableInBlender && gRSP.bFogEnabled) )
			return (int)i;
	}

	return -1;
}

void COGLFragmentShaderCombiner::InitCombinerCycle12(void)
{

#ifdef _DEBUG
	if( debuggerDropCombiners )
	{
		UpdateCombiner(m_pDecodedMux->m_dwMux0,m_pDecodedMux->m_dwMux1);
		m_vCompiledShaders.clear();
		m_dwLastMux0 = m_dwLastMux1 = 0;
		debuggerDropCombiners = false;
	}
#endif

	m_pOGLRender->EnableMultiTexture();

	bool combinerIsChanged = false;

	if( m_pDecodedMux->m_dwMux0 != m_dwLastMux0 || m_pDecodedMux->m_dwMux1 != m_dwLastMux1 || m_lastIndex < 0 )
	{
		combinerIsChanged = true;
		m_lastIndex = FindCompiledMux();
		if( m_lastIndex < 0 )		// Can not found
		{
			m_lastIndex = ParseDecodedMux();
		}

		m_dwLastMux0 = m_pDecodedMux->m_dwMux0;
		m_dwLastMux1 = m_pDecodedMux->m_dwMux1;
	}


	GenerateCombinerSettingConstants(m_lastIndex);
	if( m_bCycleChanged || combinerIsChanged || gRDP.texturesAreReloaded || gRDP.colorsAreReloaded )
	{
		if( m_bCycleChanged || combinerIsChanged )
		{
			GenerateCombinerSettingConstants(m_lastIndex);
			GenerateCombinerSetting(m_lastIndex);
		}
		else if( gRDP.colorsAreReloaded )
		{
			GenerateCombinerSettingConstants(m_lastIndex);
		}

		m_pOGLRender->SetAllTexelRepeatFlag();

		gRDP.colorsAreReloaded = false;
		gRDP.texturesAreReloaded = false;
	}
	else
	{
		m_pOGLRender->SetAllTexelRepeatFlag();
	}
}

void COGLFragmentShaderCombiner::InitCombinerBlenderForSimpleTextureDraw(uint32 tile)
{
	m_pOGLRender->DisableMultiTexture();
	if( g_textures[tile].m_pCTexture )
	{
		m_pOGLRender->EnableTexUnit(0,TRUE);
		glBindTexture(GL_TEXTURE_2D, ((COGLTexture*)(g_textures[tile].m_pCTexture))->m_dwTextureName);
		OPENGL_CHECK_ERRORS;
	}
	m_pOGLRender->SetAllTexelRepeatFlag();

	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP);
	OPENGL_CHECK_ERRORS;
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP);
	OPENGL_CHECK_ERRORS;
	glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MIN_FILTER,GL_LINEAR);	// Linear Filtering
	OPENGL_CHECK_ERRORS;
	glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MAG_FILTER,GL_LINEAR);	// Linear Filtering
	OPENGL_CHECK_ERRORS;

	glTexEnvi(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_REPLACE);
	OPENGL_CHECK_ERRORS;
	m_pOGLRender->SetAlphaTestEnable(FALSE);
}

#ifdef _DEBUG
extern const char *translatedCombTypes[];
void COGLFragmentShaderCombiner::DisplaySimpleMuxString(void)
{
	TRACE0("\nSimplified Mux\n");
	m_pDecodedMux->DisplaySimpliedMuxString("Used");
}
#endif

