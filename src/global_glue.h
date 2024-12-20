/*
	global_glue.h

	Copyright (C) 2003 Bernd Schmidt, Philip Cummins, Paul C. Pratt

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

#ifndef GLOBGLUE_H
#define GLOBGLUE_H

#include "sys_dependencies.h"
#include "UTIL/dbglog.h"
#include "ticks.h"

#define IncludeExtnPbufs 1

// various globals
extern bool SpeedStopped;
extern bool RunInBackground;
extern bool WantFullScreen;
extern bool WantMagnify;
extern bool RequestInsertDisk;
extern uint8_t RequestIthDisk;
extern bool ControlKeyPressed;

extern void MemOverlay_ChangeNtfy(void);

#if (CurEmMd == kEmMd_II) || (CurEmMd == kEmMd_IIx)
extern void Addr32_ChangeNtfy(void);
#endif

/*
	representation of pointer into memory of emulated computer.
*/
typedef uint32_t CPTR;

/*
	mapping of address space to real memory
*/

extern uint8_t * get_real_address0(uint32_t L, bool WritableMem, CPTR addr,
	uint32_t *actL);

/*
	accessing addresses that don't map to
	real memory, i.e. memory mapped devices
*/

extern bool AddrSpac_Init(void);
extern void VIAorSCCinterruptChngNtfy(void);

constexpr uint32_t kLn2CycleScale = 6;
constexpr uint32_t kNumSubTicks = 16;
constexpr uint32_t kCycleScale = (1 << kLn2CycleScale);

constexpr uint32_t CyclesScaledPerTick = (130240UL * ClockMult * kCycleScale);
constexpr uint32_t CyclesScaledPerSubTick = (CyclesScaledPerTick / kNumSubTicks);

#if WantCycByPriOp
constexpr uint32_t RdAvgXtraCyc = /* 0 */ (kCycleScale + kCycleScale / 4);
constexpr uint32_t WrAvgXtraCyc = /* 0 */ (kCycleScale + kCycleScale / 4);
#endif

extern bool FindKeyEvent(int *VirtualKey, bool *KeyDown);

/* minivmac extensions */

enum minivmac_extension {
	ExtnDat_checkval = 0,
	ExtnDat_extension = 2,
	ExtnDat_commnd = 4,
	ExtnDat_result = 6,
	ExtnDat_params = 8
};

constexpr uint32_t kCmndVersion = 0;
constexpr uint32_t ExtnDat_version = 8;

enum {
	kExtnFindExtn, /* must be first */

	kExtnDisk,
	kExtnSony,
#if EmVidCard
	kExtnVideo,
#endif
#if IncludeExtnPbufs
	kExtnParamBuffers,
#endif
#if IncludeExtnHostTextClipExchange
	kExtnHostTextClipExchange,
#endif

	kNumExtns
};

#define kcom_callcheck 0x5B17

extern uint32_t disk_icon_addr;

extern void Memory_Reset(void);

extern void Extn_Reset(void);

extern void customreset(void);

struct ATTer {
	struct ATTer *Next;
	uint32_t cmpmask;
	uint32_t cmpvalu;
	uint32_t Access;
	uint32_t usemask; /* Should be one less than a power of two. */
	uint8_t * usebase;
	uint8_t MMDV;
	uint8_t Ntfy;
	uint16_t Pad0;
	uint32_t Pad1; /* make 32 byte structure, on 32 bit systems */
};
typedef struct ATTer ATTer;
typedef ATTer *ATTep;

enum kATTA {
	kATTA_readreadybit = 0,
	kATTA_writereadybit,
	kATTA_mmdvbit,
	kATTA_ntfybit
};

constexpr uint32_t kATTA_readwritereadymask = \
	((1 << kATTA_readreadybit) | (1 << kATTA_writereadybit));
constexpr uint32_t kATTA_readreadymask = (1 << kATTA_readreadybit);
constexpr uint32_t kATTA_writereadymask = (1 << kATTA_writereadybit);
constexpr uint32_t kATTA_mmdvmask = (1 << kATTA_mmdvbit);
constexpr uint32_t kATTA_ntfymask = (1 << kATTA_ntfybit);

extern uint32_t MMDV_Access(ATTep p, uint32_t Data,
	bool WriteMem, bool ByteSize, CPTR addr);
extern bool MemAccessNtfy(ATTep pT);

#endif
