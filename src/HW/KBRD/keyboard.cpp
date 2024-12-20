/*
	KBRDEMDV.c

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
	KeyBoaRD EMulated DeVice

	Emulation of the keyboard in the Mac Plus.

	This code adapted from "Keyboard.c" in vMac by Philip Cummins.
*/

#include "sys_dependencies.h"
#include "UI/os_glue.h"
#include "hw_config.h"
#include "global_glue.h"

#include "HW/KBRD/keyboard.h"

#ifdef _VIA_Debug
#include <stdio.h>
#endif

#if EmClassicKbrd

/*
	ReportAbnormalID unused 0x0B03 - 0x0BFF
*/

extern void KYBD_ShiftOutData(uint8_t v);
extern uint8_t KYBD_ShiftInData(void);

enum {
	kKybdStateIdle,
	kKybdStateRecievingCommand,
	kKybdStateRecievedCommand,
	kKybdStateRecievingEndCommand,

	kKybdStates
};

static int KybdState = kKybdStateIdle;

static bool HaveKeyBoardResult = false;
static uint8_t KeyBoardResult;

static void GotKeyBoardData(uint8_t v)
{
	if (KybdState != kKybdStateIdle) {
		HaveKeyBoardResult = true;
		KeyBoardResult = v;
	} else {
		KYBD_ShiftOutData(v);
		VIA1_iCB2 = 1;
	}
}

static uint8_t InstantCommandData = 0x7B;

static bool AttemptToFinishInquiry(void)
{
	int i;
	bool KeyDown;
	uint8_t Keyboard_Data;

	if (FindKeyEvent(&i, &KeyDown)) {
		if (i < 64) {
			Keyboard_Data = i << 1;
			if (! KeyDown) {
				Keyboard_Data += 128;
			}
		} else {
			Keyboard_Data = 121;
			InstantCommandData = (i - 64) << 1;
			if (! KeyDown) {
				InstantCommandData += 128;
			}
		}
		GotKeyBoardData(Keyboard_Data);
		return true;
	} else {
		return false;
	}
}

static const int MaxKeyboardWait = 16; /* in 60ths of a second */
	/*
		Code in the mac rom will reset the keyboard if
		it hasn't been heard from in 32/60th of a second.
		So time out and send something before that
		to keep connection.
	*/

static int InquiryCommandTimer = 0;

void DoKybd_ReceiveCommand(void)
{
	if (KybdState != kKybdStateRecievingCommand) {
		ReportAbnormalID(0x0B01,
			"KybdState != kKybdStateRecievingCommand");
	} else {
		uint8_t in = KYBD_ShiftInData();

		KybdState = kKybdStateRecievedCommand;

		switch (in) {
			case 0x10 : /* Inquiry Command */
				if (! AttemptToFinishInquiry()) {
					InquiryCommandTimer = MaxKeyboardWait;
				}
				break;
			case 0x14 : /* Instant Command */
				GotKeyBoardData(InstantCommandData);
				InstantCommandData = 0x7B;
				break;
			case 0x16 : /* Model Command */
				GotKeyBoardData(0x0b /* 0x01 */);
					/* Test value, means Model 0, no extra devices */
				/*
					Fixed by Hoshi Takanori -
						it uses the proper keyboard type now
				*/
				break;
			case 0x36 : /* Test Command */
				GotKeyBoardData(0x7D);
				break;
			case 0x00:
				GotKeyBoardData(0);
				break;
			default :
				/* Debugger(); */
				GotKeyBoardData(0);
				break;
		}
	}
}

void DoKybd_ReceiveEndCommand(void)
{
	if (KybdState != kKybdStateRecievingEndCommand) {
		ReportAbnormalID(0x0B02,
			"KybdState != kKybdStateRecievingEndCommand");
	} else {
		KybdState = kKybdStateIdle;
#ifdef _VIA_Debug
		fprintf(stderr, "enter DoKybd_ReceiveEndCommand\n");
#endif
		if (HaveKeyBoardResult) {
#ifdef _VIA_Debug
			fprintf(stderr, "HaveKeyBoardResult: %d\n", KeyBoardResult);
#endif
			HaveKeyBoardResult = false;
			KYBD_ShiftOutData(KeyBoardResult);
			VIA1_iCB2 = 1;
		}
	}
}

void Kybd_DataLineChngNtfy(void)
{
	switch (KybdState) {
		case kKybdStateIdle:
			if (VIA1_iCB2 == 0) {
				KybdState = kKybdStateRecievingCommand;
#ifdef _VIA_Debug
				fprintf(stderr, "posting kICT_Kybd_ReceiveCommand\n");
#endif
				ICT::add(kICT_Kybd_ReceiveCommand,
					6800UL * kCycleScale / 64 * ClockMult);

				if (InquiryCommandTimer != 0) {
					InquiryCommandTimer = 0; /* abort Inquiry */
				}
			}
			break;
		case kKybdStateRecievedCommand:
			if (VIA1_iCB2 == 1) {
				KybdState = kKybdStateRecievingEndCommand;
#ifdef _VIA_Debug
				fprintf(stderr,
					"posting kICT_Kybd_ReceiveEndCommand\n");
#endif
				ICT::add(kICT_Kybd_ReceiveEndCommand,
					6800UL * kCycleScale / 64 * ClockMult);
			}
			break;
	}
}

void KeyBoard_Update(void)
{
	if (InquiryCommandTimer != 0) {
		if (AttemptToFinishInquiry()) {
			InquiryCommandTimer = 0;
		} else {
			--InquiryCommandTimer;
			if (InquiryCommandTimer == 0) {
				GotKeyBoardData(0x7B);
			}
		}
	}
}
#endif