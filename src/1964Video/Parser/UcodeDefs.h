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

#ifndef _UCODE_DEFS_H_
#define _UCODE_DEFS_H_

typedef struct {
	union {
		u32 w0;
		struct {
			u32 arg0:24;
			u32 cmd:8;
		};
	};
	u32 w1;
} Gwords;

typedef struct {
	u32 w0;
	u32 v2:8;
	u32 v1:8;
	u32 v0:8;
	u32 flag:8;
} GGBI0_Tri1;

typedef struct {
	u32 v0:8;
	u32 v1:8;
	u32 v2:8;
	u32 cmd:8;
	u32 pad:24;
	u32 flag:8;
} GGBI2_Tri1;

typedef struct {
	u32 :1;
	u32 v3:7;
	u32 :1;
	u32 v4:7;
	u32 :1;
	u32 v5:7;
	u32 cmd:8;
	u32 :1;
	u32 v0:7;
	u32 :1;
	u32 v1:7;
	u32 :1;
	u32 v2:7;
	u32 flag:8;
} GGBI2_Tri2;

typedef struct {
	u32 w0;
	u32 v2:8;
	u32 v1:8;
	u32 v0:8;
	u32 v3:8;
} GGBI0_Ln3DTri2;

typedef struct {
	u32 v5:8;
	u32 v4:8;
	u32 v3:8;
	u32 cmd:8;

	u32 v2:8;
	u32 v1:8;
	u32 v0:8;
	u32 flag:8;
} GGBI1_Tri2;

typedef struct {
	u32 v3:8;
	u32 v4:8;
	u32 v5:8;
	u32 cmd:8;

	u32 v0:8;
	u32 v1:8;
	u32 v2:8;
	u32 flag:8;
} GGBI2_Line3D;

typedef struct {
	u32 len:16;
	u32 v0:4;
	u32 n:4;
	u32 cmd:8;
	u32 addr;
} GGBI0_Vtx;

typedef struct {
	u32 len:10;
	u32 n:6;
	u32 :1;
	u32 v0:7;
	u32 cmd:8;
	u32 addr;
} GGBI1_Vtx;

typedef struct {
	u32 vend:8;
	u32 :4;
	u32 n:8;
	u32 :4;
	u32 cmd:8;
	u32 addr;
} GGBI2_Vtx;

typedef struct {
	u32    width:12;
	u32    :7;
	u32    siz:2;
	u32    fmt:3;
	u32	cmd:8;
	u32    addr;
} GSetImg;

typedef struct {
	u32	prim_level:8;
	u32	prim_min_level:8;
	u32	pad:8;
	u32	cmd:8;

	union {
		u32	color;
		struct {
			u32 fillcolor:16;
			u32 fillcolor2:16;
		};
		struct {
			u32 a:8;
			u32 b:8;
			u32 g:8;
			u32 r:8;
		};
	};
} GSetColor;

typedef struct {
	u32	:16;
	u32	param:8;
	u32	cmd:8;
	u32    addr;
} GGBI0_Dlist;

typedef struct {
	u32	len:16;
	u32	projection:1;
	u32	load:1;
	u32	push:1;
	u32	:5;
	u32	cmd:8;
	u32    addr;
} GGBI0_Matrix;

typedef struct {
	u32	:24;
	u32	cmd:8;
	u32	projection:1;
	u32	:31;
} GGBI0_PopMatrix;

typedef struct {
	union {
		struct {
			u32	param:8;
			u32	len:16;
			u32	cmd:8;
		};
		struct {
			u32	nopush:1;
			u32	load:1;
			u32	projection:1;
			u32	:5;
			u32	len2:16;
			u32	cmd2:8;
		};
	};
	u32    addr;
} GGBI2_Matrix;

typedef struct {
	u32	type:8;
	u32	offset:16;
	u32	cmd:8;
	u32	value;
} GGBI0_MoveWord;

typedef struct {
	u32	offset:16;
	u32	type:8;
	u32	cmd:8;
	u32	value;
} GGBI2_MoveWord;

typedef struct {
	u32	enable_gbi0:1;
	u32	enable_gbi2:1;
	u32	:6;
	u32	tile:3;
	u32	level:3;
	u32	:10;
	u32	cmd:8;
	u32	scaleT:16;
	u32	scaleS:16;
} GTexture;

typedef struct {
	u32	tl:12;
	u32	sl:12;
	u32	cmd:8;

	u32	th:12;
	u32	sh:12;
	u32	tile:3;
	u32	pad:5;
} Gloadtile;

typedef struct {
	u32	tmem:9;
	u32	line:9;
	u32	pad0:1;
	u32	siz:2;
	u32	fmt:3;
	u32	cmd:8;

	u32	shifts:4;
	u32	masks:4;
	u32	ms:1;
	u32	cs:1;
	u32	shiftt:4;
	u32	maskt:4;
	u32	mt:1;
	u32	ct:1;
	u32	palette:4;
	u32	tile:3;
	u32	pad1:5;
} Gsettile;

typedef union {
	Gwords			words;
	GGBI0_Tri1		tri1;
	GGBI0_Ln3DTri2	ln3dtri2;
	GGBI1_Tri2		gbi1tri2;
	GGBI2_Tri1		gbi2tri1;
	GGBI2_Tri2		gbi2tri2;
	GGBI2_Line3D	gbi2line3d;
	GGBI0_Vtx		gbi0vtx;
	GGBI1_Vtx		gbi1vtx;
	GGBI2_Vtx		gbi2vtx;
	GSetImg			setimg;
	GSetColor		setcolor;
	GGBI0_Dlist		gbi0dlist;
	GGBI0_Matrix	gbi0matrix;
	GGBI0_PopMatrix	gbi0popmatrix;
	GGBI2_Matrix	gbi2matrix;
	GGBI0_MoveWord	gbi0moveword;
	GGBI2_MoveWord	gbi2moveword;
	GTexture		texture;
	Gloadtile		loadtile;
	Gsettile		settile;
	/*
	Gdma		dma;
	Gsegment	segment;
	GsetothermodeH	setothermodeH;
	GsetothermodeL	setothermodeL;
	Gtexture	texture;
	Gperspnorm	perspnorm;
	Gsetcombine	setcombine;
	Gfillrect	fillrect;
	Gsettile	settile;
	Gloadtile	loadtile;
	Gsettilesize	settilesize;
	Gloadtlut	loadtlut;
	*/
	__int64	force_structure_alignment;
} Gfx;

typedef union {
	struct {
		u32	w0;
		u32	w1;
		u32	w2;
		u32	w3;
	};
	struct {
		u32	yl:12;	/* Y coordinate of upper left	*/
		u32	xl:12;	/* X coordinate of upper left	*/
		u32	cmd:8;	/* command			*/

		u32	yh:12;	/* Y coordinate of lower right	*/
		u32	xh:12;	/* X coordinate of lower right	*/
		u32	tile:3;	/* Tile descriptor index	*/
		u32	pad1:5;	/* Padding			*/

		u32	t:16;	/* T texture coord at top left	*/
		u32	s:16;	/* S texture coord at top left	*/

		u32	dtdy:16;/* Change in T per change in Y	*/
		u32	dsdx:16;/* Change in S per change in X	*/
	};
} Gtexrect;

#endif
