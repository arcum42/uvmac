/*
	HW/VIA/VIAEMDEV.h

	Copyright (C) 2004 Philip Cummins, Paul C. Pratt

	You can redistribute this file and/or modify it under the terms
	of version 2 of the GNU General Public License as published by
	the Free Software Foundation.  You should have received a copy
	of the license along with this file; see the file COPYING.

	This file is distributed in the hope that it will be useful,
	but WITHOUT ANY WARRANTY; without even the implied warranty of
	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
	license for more details.
*/

#ifndef VIAEMDEV_H
#define VIAEMDEV_H

EXPORTPROC VIA1_Zap(void);
EXPORTPROC VIA1_Reset(void);

EXPORTFUNC uint32_t VIA1_Access(uint32_t Data, bool WriteMem, CPTR addr);

EXPORTPROC VIA1_ExtraTimeBegin(void);
EXPORTPROC VIA1_ExtraTimeEnd(void);
#ifdef VIA1_iCA1_PulseNtfy
EXPORTPROC VIA1_iCA1_PulseNtfy(void);
#endif
#ifdef VIA1_iCA2_PulseNtfy
EXPORTPROC VIA1_iCA2_PulseNtfy(void);
#endif
#ifdef VIA1_iCB1_PulseNtfy
EXPORTPROC VIA1_iCB1_PulseNtfy(void);
#endif
#ifdef VIA1_iCB2_PulseNtfy
EXPORTPROC VIA1_iCB2_PulseNtfy(void);
#endif
EXPORTPROC VIA1_DoTimer1Check(void);
EXPORTPROC VIA1_DoTimer2Check(void);

EXPORTFUNC uint16_t VIA1_GetT1InvertTime(void);

EXPORTPROC VIA1_ShiftInData(uint8_t v);
EXPORTFUNC uint8_t VIA1_ShiftOutData(void);

#endif