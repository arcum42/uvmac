/*
	video.h

	Copyright (C) 2008 Paul C. Pratt

	You can redistribute this file and/or modify it under the terms
	of version 2 of the GNU General Public License as published by
	the Free Software Foundation.  You should have received a copy
	of the license along with this file; see the file COPYING.

	This file is distributed in the hope that it will be useful,
	but WITHOUT ANY WARRANTY; without even the implied warranty of
	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
	license for more details.
*/

#pragma once

#ifndef VIDEMDEV_H
#define VIDEMDEV_H

extern bool Vid_Init(void);
extern uint16_t Vid_Reset(void);
extern void Vid_Update(void);

extern void ExtnVideo_Access(CPTR p);

//#define CLUT_size (1 << (1 << vMacScreenDepth))
#define CLUT_size 256 // total guesstimate

extern uint16_t CLUT_reds[CLUT_size];
extern uint16_t CLUT_greens[CLUT_size];
extern uint16_t CLUT_blues[CLUT_size];

#endif
