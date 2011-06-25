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

#ifndef _OGL_COMBINER_H_
#define _OGL_COMBINER_H_

#include "../Combiner.h"

class OGLRender;

typedef struct {
	uint32	dwMux0;
	uint32	dwMux1;

	bool	fogIsUsed;
	GLuint	programID;
} OGLShaderCombinerSaveType;

class COGLFragmentShaderCombiner : public CColorCombiner
{
public:
	bool Initialize(void);
	void InitCombinerBlenderForSimpleTextureDraw(uint32 tile=0);
protected:
	friend class OGLDeviceBuilder;

	void DisableCombiner(void);
	void InitCombinerCycleCopy(void);
	void InitCombinerCycleFill(void);
	void InitCombinerCycle12(void);

	COGLFragmentShaderCombiner(CRender *pRender);
	~COGLFragmentShaderCombiner();
	OGLRender *m_pOGLRender;

	std::vector<OGLShaderCombinerSaveType>		m_vCompiledShaders;
	int m_maxTexUnits;
	int	m_lastIndex;
	uint32 m_dwLastMux0;
	uint32 m_dwLastMux1;
#ifdef _DEBUG
	void DisplaySimpleMuxString(void);
#endif
private:
	virtual int ParseDecodedMux();
	virtual void GenerateProgramStr();
	int FindCompiledMux();
	virtual void GenerateCombinerSetting(int index);
	virtual void GenerateCombinerSettingConstants(int index);
};


#endif



