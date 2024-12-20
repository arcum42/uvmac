/*
	MOUSEMDV.c

	Copyright (C) 2006 Philip Cummins, Paul C. Pratt

	You can redistribute this file and/or modify it under the terms
	of version 2 of the GNU General Public License as published by
	the Free Software Foundation.  You should have received a copy
	of the license along with this file; see the file COPYING.

	This file is distributed in the hope that it will be useful,
	but WITHOUT ANY WARRANTY; without even the implied warranty of
	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
	license for more details.
*/

/*
	MOUSe EMulated DeVice

	Emulation of the mouse in the Mac Plus.

	This code descended from "Mouse-MacOS.c" in Richard F. Bannister's
	Macintosh port of vMac, by Philip Cummins.
*/

#include "sys_dependencies.h"
#include "UTIL/endian.h"

#include "global_glue.h"
#include "UI/os_glue.h"
#include "UI/event_queue.h"

#include "HW/SCC/serial_comm.h"
#include "HW/M68K/cpu_68k.h"
#include "HW/MOUSE/mouse.h"
#include "HW/RAM/ram.h"

uint16_t CurMouseV = 0;
uint16_t CurMouseH = 0;

void Mouse_Update(void)
{
#if HaveMasterEvtQLock
	if (0 != MasterEvtQLock) {
		--MasterEvtQLock;
	}
#endif

	/*
		Check mouse position first. After mouse button or key event,
		can't process another mouse position until following tick,
		otherwise button or key will be in wrong place.
	*/

	/*
		if start doing this too soon after boot,
		will mess up memory check
	*/
	if (Mouse_Enabled()) {
		EvtQEl *p;

		if (
#if HaveMasterEvtQLock
			(0 == MasterEvtQLock) &&
#endif
			(nullptr != (p = EvtQ.OutP())))
		{
#if EmClassicKbrd
#if EnableMouseMotion
			if (EvtQElKindMouseDelta == p->kind) {

				if ((p->u.pos.h != 0) || (p->u.pos.v != 0)) {
					put_ram_word(0x0828,
						get_ram_word(0x0828) + p->u.pos.v);
					put_ram_word(0x082A,
						get_ram_word(0x082A) + p->u.pos.h);
					put_ram_byte(0x08CE, get_ram_byte(0x08CF));
						/* Tell MacOS to redraw the Mouse */
				}
				EvtQ.OutDone();
			} else
#endif
#endif
			if (EvtQElKindMousePos == p->kind) {
				uint32_t NewMouse = (p->u.pos.v << 16) | p->u.pos.h;

				if (get_ram_long(0x0828) != NewMouse) {
					put_ram_long(0x0828, NewMouse);
						/* Set Mouse Position */
					put_ram_long(0x082C, NewMouse);
#if EmClassicKbrd
					put_ram_byte(0x08CE, get_ram_byte(0x08CF));
						/* Tell MacOS to redraw the Mouse */
#else
					put_ram_long(0x0830, NewMouse);
					put_ram_byte(0x08CE, 0xFF);
						/* Tell MacOS to redraw the Mouse */
#endif
				}
				EvtQ.OutDone();
			}
		}
	}

#if EmClassicKbrd
	{
		EvtQEl *p;

		if (
#if HaveMasterEvtQLock
			(0 == MasterEvtQLock) &&
#endif
			(nullptr != (p = EvtQ.OutP())))
		{
			if (EvtQElKindMouseButton == p->kind) {
				MouseBtnUp = p->u.press.down ? 0 : 1;
				EvtQ.OutDone();
				MasterEvtQLock = 4;
			}
		}
	}
#endif
}

void Mouse_EndTickNotify(void)
{
	if (Mouse_Enabled()) {
		/* tell platform specific code where the mouse went */
		CurMouseV = get_ram_word(0x082C);
		CurMouseH = get_ram_word(0x082E);
	}
}
