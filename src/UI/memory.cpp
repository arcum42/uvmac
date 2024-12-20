#include "sys_dependencies.h"

#include "HW/hardware.h"
#include "UTIL/powers.h"

extern void MacMsg(char *briefMsg, char *longMsg, bool fatal);
extern void Extn_Access(uint32_t Data, CPTR addr);
extern bool InterruptButton;

#define MaxATTListN 32

uimr ReserveAllocOffset;
uint8_t *ReserveAllocBigBlock = nullptr;

enum
{
	kMMDV_VIA1,
#if EmVIA2
	kMMDV_VIA2,
#endif
	kMMDV_SCC,
	kMMDV_Extn,
#if EmASC
	kMMDV_ASC,
#endif
	kMMDV_SCSI,
	kMMDV_IWM,

	kNumMMDVs
};

enum
{
#if CurEmMd >= kEmMd_SE
	kMAN_OverlayOff,
#endif

	kNumMANs
};

static void SetLongs(uint32_t *p, long n)
{
	for (long i = n; --i >= 0;)
	{
		*p++ = (uint32_t)-1;
	}
}

void ReserveAllocOneBlock(uint8_t **p, uimr n, uint8_t align, bool FillOnes)
{
	ReserveAllocOffset = CeilPow2Mult(ReserveAllocOffset, align);
	if (nullptr == ReserveAllocBigBlock)
	{
		*p = nullptr;
	}
	else
	{
		*p = ReserveAllocBigBlock + ReserveAllocOffset;
		if (FillOnes)
		{
			SetLongs((uint32_t *)*p, n / 4);
		}
	}
	ReserveAllocOffset += n;
}

void EmulationReserveAlloc(void)
{
	ReserveAllocOneBlock(&RAM, kRAM_Size + RAMSafetyMarginFudge, 5, false);
#if EmVidCard
	ReserveAllocOneBlock(&VidROM, kVidROM_Size, 5, false);
#endif
#if IncludeVidMem
	ReserveAllocOneBlock(&VidMem, kVidMemRAM_Size + RAMSafetyMarginFudge, 5, true);
#endif
#if SmallGlobals
	MINEM68K_ReserveAlloc();
#endif
}

extern uint8_t *ROM;

#include "UI/video_sdl2.h"
#include "UI/sound_sdl2.h"

extern tpSoundSamp TheSoundBuffer;

void ReserveAllocAll(void)
{
#if dbglog_HAVE
	dbglog_ReserveAlloc();
#endif
	ReserveAllocOneBlock(&ROM, kROM_Size, 5, false);

	ReserveAllocOneBlock(&screencomparebuff,
						 vMacScreenNumBytes, 5, true);

	ReserveAllocOneBlock(&CLUT_final, CLUT_finalsz, 5, false);
	ReserveAllocOneBlock((uint8_t **)&TheSoundBuffer,
						 dbhBufferSize, 5, false);

	EmulationReserveAlloc();
}

bool AllocMemory(void)
{
	uimr n;
	bool IsOk = false;

	ReserveAllocOffset = 0;
	ReserveAllocBigBlock = nullptr;
	ReserveAllocAll();
	n = ReserveAllocOffset;
	ReserveAllocBigBlock = (uint8_t *)calloc(1, n);
	if (nullptr == ReserveAllocBigBlock)
	{
		MacMsg(kStrOutOfMemTitle, kStrOutOfMemMessage, true);
	}
	else
	{
		ReserveAllocOffset = 0;
		ReserveAllocAll();
		if (n != ReserveAllocOffset)
		{
			/* oops, program error */
		}
		else
		{
			IsOk = true;
		}
	}

	return IsOk;
}

void UnallocMemory(void)
{
	if (nullptr != ReserveAllocBigBlock)
	{
		free((char *)ReserveAllocBigBlock);
	}
}

static ATTer ATTListA[MaxATTListN];
static uint16_t LastATTel;

static void AddToATTList(ATTep p)
{
	uint16_t NewLast = LastATTel + 1;
	if (NewLast >= MaxATTListN)
	{
		ReportAbnormalID(0x1101, "MaxATTListN not big enough");
	}
	else
	{
		ATTListA[LastATTel] = *p;
		LastATTel = NewLast;
	}
}

static void InitATTList(void)
{
	LastATTel = 0;
}

static void FinishATTList(void)
{
	{
		/* add guard */
		ATTer r;

		r.cmpmask = 0;
		r.cmpvalu = 0;
		r.usemask = 0;
		r.usebase = nullptr;
		r.Access = 0;
		AddToATTList(&r);
	}

	{
		uint16_t i = LastATTel;
		ATTep p = &ATTListA[LastATTel];
		ATTep h = nullptr;

		while (0 != i)
		{
			--i;
			--p;
			p->Next = h;
			h = p;
		}

#if 0 /* verify list. not for final version */
		{
			ATTep q1;
			ATTep q2;
			for (q1 = h; nullptr != q1->Next; q1 = q1->Next) {
				if ((q1->cmpvalu & ~ q1->cmpmask) != 0) {
					ReportAbnormalID(0x1102, "ATTListA bad entry");
				}
				for (q2 = q1->Next; nullptr != q2->Next; q2 = q2->Next) {
					uint32_t common_mask = (q1->cmpmask) & (q2->cmpmask);
					if ((q1->cmpvalu & common_mask) ==
						(q2->cmpvalu & common_mask))
					{
						ReportAbnormalID(0x1103, "ATTListA Conflict");
					}
				}
			}
		}
#endif

		SetHeadATTel(h); // Mac II crashes here.
	}
}

#if (CurEmMd == kEmMd_II) || (CurEmMd == kEmMd_IIx)
static void SetUp_RAM24(void)
{
	ATTer r;
	uint32_t bankbit = 0x00100000 << (((VIA2_iA7 << 1) | VIA2_iA6) << 1);

#if kRAMa_Size == kRAMb_Size
	if (kRAMa_Size == bankbit)
	{
		/* properly set up balanced RAM */
		r.cmpmask = 0x00FFFFFF & ~((1 << kRAM_ln2Spc) - 1);
		r.cmpvalu = 0;
		r.usemask = ((1 << kRAM_ln2Spc) - 1) & (kRAM_Size - 1);
		r.usebase = RAM;
		r.Access = kATTA_readwritereadymask;
		AddToATTList(&r);
	}
	else
#endif
	{
		bankbit &= 0x00FFFFFF; /* if too large, always use RAMa */

		if (0 != bankbit)
		{
#if kRAMb_Size != 0
			r.cmpmask = bankbit | (0x00FFFFFF & ~((1 << kRAM_ln2Spc) - 1));
			r.cmpvalu = bankbit;
			r.usemask = ((1 << kRAM_ln2Spc) - 1) & (kRAMb_Size - 1);
			r.usebase = kRAMa_Size + RAM;
			r.Access = kATTA_readwritereadymask;
			AddToATTList(&r);
#endif
		}

		{
			r.cmpmask = bankbit | (0x00FFFFFF & ~((1 << kRAM_ln2Spc) - 1));
			r.cmpvalu = 0;
			r.usemask = ((1 << kRAM_ln2Spc) - 1) & (kRAMa_Size - 1);
			r.usebase = RAM;
			r.Access = kATTA_readwritereadymask;
			AddToATTList(&r);
		}
	}
}
#endif

#if (CurEmMd == kEmMd_II) || (CurEmMd == kEmMd_IIx)
static void SetUp_io(void)
{
	ATTer r;

	if (Addr32)
	{
		r.cmpmask = 0xFF01E000;
		r.cmpvalu = 0x50000000;
	}
	else
	{
		r.cmpmask = 0x00F1E000;
		r.cmpvalu = 0x00F00000;
	}
	r.usebase = nullptr;
	r.Access = kATTA_mmdvmask;
	r.MMDV = kMMDV_VIA1;
	AddToATTList(&r);

	if (Addr32)
	{
		r.cmpmask = 0xFF01E000;
		r.cmpvalu = 0x50000000 | 0x2000;
	}
	else
	{
		r.cmpmask = 0x00F1E000;
		r.cmpvalu = 0x00F00000 | 0x2000;
	}
	r.usebase = nullptr;
	r.Access = kATTA_mmdvmask;
	r.MMDV = kMMDV_VIA2;
	AddToATTList(&r);

	if (Addr32)
	{
		r.cmpmask = 0xFF01E000;
		r.cmpvalu = 0x50000000 | 0x4000;
	}
	else
	{
		r.cmpmask = 0x00F1E000;
		r.cmpvalu = 0x00F00000 | 0x4000;
	}
	r.usebase = nullptr;
	r.Access = kATTA_mmdvmask;
	r.MMDV = kMMDV_SCC;
	AddToATTList(&r);

	if (Addr32)
	{
		r.cmpmask = 0xFF01E000;
		r.cmpvalu = 0x50000000 | 0x0C000;
	}
	else
	{
		r.cmpmask = 0x00F1E000;
		r.cmpvalu = 0x00F00000 | 0x0C000;
	}
	r.usebase = nullptr;
	r.Access = kATTA_mmdvmask;
	r.MMDV = kMMDV_Extn;
	AddToATTList(&r);

	if (Addr32)
	{
		r.cmpmask = 0xFF01E000;
		r.cmpvalu = 0x50000000 | 0x10000;
	}
	else
	{
		r.cmpmask = 0x00F1E000;
		r.cmpvalu = 0x00F00000 | 0x10000;
	}
	r.usebase = nullptr;
	r.Access = kATTA_mmdvmask;
	r.MMDV = kMMDV_SCSI;
	AddToATTList(&r);

	if (Addr32)
	{
		r.cmpmask = 0xFF01E000;
		r.cmpvalu = 0x50000000 | 0x14000;
	}
	else
	{
		r.cmpmask = 0x00F1E000;
		r.cmpvalu = 0x00F00000 | 0x14000;
	}
	r.usebase = nullptr;
	r.Access = kATTA_mmdvmask;
	r.MMDV = kMMDV_ASC;
	AddToATTList(&r);

	if (Addr32)
	{
		r.cmpmask = 0xFF01E000;
		r.cmpvalu = 0x50000000 | 0x16000;
	}
	else
	{
		r.cmpmask = 0x00F1E000;
		r.cmpvalu = 0x00F00000 | 0x16000;
	}
	r.usebase = nullptr;
	r.Access = kATTA_mmdvmask;
	r.MMDV = kMMDV_IWM;
	AddToATTList(&r);

#if 0
		case 14:
			/*
				fail, nothing supposed to be here,
				but rom accesses it anyway
			*/
			{
				uint32_t addr2 = addr & 0x1FFFF;

				if ((addr2 != 0x1DA00) && (addr2 != 0x1DC00)) {
					ReportAbnormalID(0x1104, "another unknown access");
				}
			}
			get_fail_realblock(p);
			break;
#endif
}
#endif

#if (CurEmMd == kEmMd_II) || (CurEmMd == kEmMd_IIx)
static void SetUp_address24(void)
{
	ATTer r;

#if 0
	if (MemOverlay) {
		ReportAbnormalID(0x1105, "Overlay with 24 bit addressing");
	}
#endif

	if (MemOverlay)
	{
		r.cmpmask = Overlay_ROM_CmpZeroMask |
					(0x00FFFFFF & ~((1 << kRAM_ln2Spc) - 1));
		r.cmpvalu = kRAM_Base;
		r.usemask = kROM_Size - 1;
		r.usebase = ROM;
		r.Access = kATTA_readreadymask;
		AddToATTList(&r);
	}
	else
	{
		SetUp_RAM24();
	}

	r.cmpmask = kROM_cmpmask;
	r.cmpvalu = kROM_Base;
	r.usemask = kROM_Size - 1;
	r.usebase = ROM;
	r.Access = kATTA_readreadymask;
	AddToATTList(&r);

	r.cmpmask = 0x00FFFFFF & ~(0x100000 - 1);
	r.cmpvalu = 0x900000;
	r.usemask = (kVidMemRAM_Size - 1) & (0x100000 - 1);
	r.usebase = VidMem;
	r.Access = kATTA_readwritereadymask;
	AddToATTList(&r);
#if kVidMemRAM_Size >= 0x00200000
	r.cmpmask = 0x00FFFFFF & ~(0x100000 - 1);
	r.cmpvalu = 0xA00000;
	r.usemask = (0x100000 - 1);
	r.usebase = VidMem + (1 << 20);
	r.Access = kATTA_readwritereadymask;
	AddToATTList(&r);
#endif
#if kVidMemRAM_Size >= 0x00400000
	r.cmpmask = 0x00FFFFFF & ~(0x100000 - 1);
	r.cmpvalu = 0xB00000;
	r.usemask = (0x100000 - 1);
	r.usebase = VidMem + (2 << 20);
	r.Access = kATTA_readwritereadymask;
	AddToATTList(&r);
	r.cmpmask = 0x00FFFFFF & ~(0x100000 - 1);
	r.cmpvalu = 0xC00000;
	r.usemask = (0x100000 - 1);
	r.usebase = VidMem + (3 << 20);
	r.Access = kATTA_readwritereadymask;
	AddToATTList(&r);
#endif

	SetUp_io();
}
#endif

#if (CurEmMd == kEmMd_II) || (CurEmMd == kEmMd_IIx)
static void SetUp_address32(void)
{
	ATTer r;

	if (MemOverlay)
	{
		r.cmpmask = ~((1 << 30) - 1);
		r.cmpvalu = 0;
		r.usemask = kROM_Size - 1;
		r.usebase = ROM;
		r.Access = kATTA_readreadymask;
		AddToATTList(&r);
	}
	else
	{
		uint32_t bankbit =
			0x00100000 << (((VIA2_iA7 << 1) | VIA2_iA6) << 1);
#if kRAMa_Size == kRAMb_Size
		if (kRAMa_Size == bankbit)
		{
			/* properly set up balanced RAM */
			r.cmpmask = ~((1 << 30) - 1);
			r.cmpvalu = 0;
			r.usemask = kRAM_Size - 1;
			r.usebase = RAM;
			r.Access = kATTA_readwritereadymask;
			AddToATTList(&r);
		}
		else
#endif
		{
#if kRAMb_Size != 0
			r.cmpmask = bankbit | ~((1 << 30) - 1);
			r.cmpvalu = bankbit;
			r.usemask = kRAMb_Size - 1;
			r.usebase = kRAMa_Size + RAM;
			r.Access = kATTA_readwritereadymask;
			AddToATTList(&r);
#endif

			r.cmpmask = bankbit | ~((1 << 30) - 1);
			r.cmpvalu = 0;
			r.usemask = kRAMa_Size - 1;
			r.usebase = RAM;
			r.Access = kATTA_readwritereadymask;
			AddToATTList(&r);
		}
	}

	r.cmpmask = ~((1 << 28) - 1);
	r.cmpvalu = 0x40000000;
	r.usemask = kROM_Size - 1;
	r.usebase = ROM;
	r.Access = kATTA_readreadymask;
	AddToATTList(&r);

#if 0
	/* haven't persuaded emulated computer to look here yet. */
	/* NuBus super space */
	r.cmpmask = ~ ((1 << 28) - 1);
	r.cmpvalu = 0x90000000;
	r.usemask = kVidMemRAM_Size - 1;
	r.usebase = VidMem;
	r.Access = kATTA_readwritereadymask;
	AddToATTList(&r);
#endif

	/* Standard NuBus space */
	r.cmpmask = ~((1 << 20) - 1);
	r.cmpvalu = 0xF9F00000;
	r.usemask = kVidROM_Size - 1;
	r.usebase = VidROM;
	r.Access = kATTA_readreadymask;
	AddToATTList(&r);
#if 0
	r.cmpmask = ~ 0x007FFFFF;
	r.cmpvalu = 0xF9000000;
	r.usemask = 0x007FFFFF & (kVidMemRAM_Size - 1);
	r.usebase = VidMem;
	r.Access = kATTA_readwritereadymask;
	AddToATTList(&r);
#endif

	r.cmpmask = ~0x000FFFFF;
	r.cmpvalu = 0xF9900000;
	r.usemask = 0x000FFFFF & (kVidMemRAM_Size - 1);
	r.usebase = VidMem;
	r.Access = kATTA_readwritereadymask;
	AddToATTList(&r);
/* kludge to allow more than 1M of Video Memory */
#if kVidMemRAM_Size >= 0x00200000
	r.cmpmask = ~0x000FFFFF;
	r.cmpvalu = 0xF9A00000;
	r.usemask = 0x000FFFFF & (kVidMemRAM_Size - 1);
	r.usebase = VidMem + (1 << 20);
	r.Access = kATTA_readwritereadymask;
	AddToATTList(&r);
#endif
#if kVidMemRAM_Size >= 0x00400000
	r.cmpmask = ~0x000FFFFF;
	r.cmpvalu = 0xF9B00000;
	r.usemask = 0x000FFFFF & (kVidMemRAM_Size - 1);
	r.usebase = VidMem + (2 << 20);
	r.Access = kATTA_readwritereadymask;
	AddToATTList(&r);
	r.cmpmask = ~0x000FFFFF;
	r.cmpvalu = 0xF9C00000;
	r.usemask = 0x000FFFFF & (kVidMemRAM_Size - 1);
	r.usebase = VidMem + (3 << 20);
	r.Access = kATTA_readwritereadymask;
	AddToATTList(&r);
#endif

	SetUp_io();

#if 0
	if ((addr >= 0x58000000) && (addr < 0x58000004)) {
		/* test hardware. fail */
	}
#endif
}
#endif

#if (CurEmMd == kEmMd_II) || (CurEmMd == kEmMd_IIx)
static void SetUp_address(void)
{
	if (Addr32)
	{
		SetUp_address32();
	}
	else
	{
		SetUp_address24();
	}
}
#endif

/*
	unlike in the real Mac Plus, Mini vMac
	will allow misaligned memory access,
	since it is easier to allow it than
	it is to correctly simulate a bus error
	and back out of the current instruction.
*/

#ifndef ln2mtb
#define AddToATTListWithMTB AddToATTList
#else
static void AddToATTListWithMTB(ATTep p)
{
	/*
		Test of memory mapping system.
	*/
	ATTer r;

	r.Access = p->Access;
	r.cmpmask = p->cmpmask | (1 << ln2mtb);
	r.usemask = p->usemask & ~(1 << ln2mtb);

	r.cmpvalu = p->cmpvalu + (1 << ln2mtb);
	r.usebase = p->usebase;
	AddToATTList(&r);

	r.cmpvalu = p->cmpvalu;
	r.usebase = p->usebase + (1 << ln2mtb);
	AddToATTList(&r);
}
#endif

#if (CurEmMd != kEmMd_II) && (CurEmMd != kEmMd_IIx)
static void SetUp_RAM24(void)
{
	ATTer r;

#if (0 == kRAMb_Size) || (kRAMa_Size == kRAMb_Size)
	r.cmpmask = 0x00FFFFFF & ~((1 << kRAM_ln2Spc) - 1);
	r.cmpvalu = kRAM_Base;
	r.usemask = kRAM_Size - 1;
	r.usebase = RAM;
	r.Access = kATTA_readwritereadymask;
	AddToATTListWithMTB(&r);
#else
	/* unbalanced memory */

#if 0 != (0x00FFFFFF & kRAMa_Size)
	/* condition should always be true if configuration file right */
	r.cmpmask = 0x00FFFFFF & (kRAMa_Size | ~ ((1 << kRAM_ln2Spc) - 1));
	r.cmpvalu = kRAM_Base + kRAMa_Size;
	r.usemask = kRAMb_Size - 1;
	r.usebase = kRAMa_Size + RAM;
	r.Access = kATTA_readwritereadymask;
	AddToATTListWithMTB(&r);
#endif

	r.cmpmask = 0x00FFFFFF & (kRAMa_Size | ~((1 << kRAM_ln2Spc) - 1));
	r.cmpvalu = kRAM_Base;
	r.usemask = kRAMa_Size - 1;
	r.usebase = RAM;
	r.Access = kATTA_readwritereadymask;
	AddToATTListWithMTB(&r);
#endif
}
#endif

#if (CurEmMd != kEmMd_II) && (CurEmMd != kEmMd_IIx)
static void SetUp_address(void)
{
	ATTer r;

	if (MemOverlay)
	{
		r.cmpmask = Overlay_ROM_CmpZeroMask |
					(0x00FFFFFF & ~((1 << kRAM_ln2Spc) - 1));
		r.cmpvalu = kRAM_Base;
		r.usemask = kROM_Size - 1;
		r.usebase = ROM;
		r.Access = kATTA_readreadymask;
		AddToATTListWithMTB(&r);
	}
	else
	{
		SetUp_RAM24();
	}

	r.cmpmask = kROM_cmpmask;
	r.cmpvalu = kROM_Base;
#if (CurEmMd >= kEmMd_SE)
	if (MemOverlay)
	{
		r.usebase = nullptr;
		r.Access = kATTA_ntfymask;
		r.Ntfy = kMAN_OverlayOff;
		AddToATTList(&r);
	}
	else
#endif
	{
		r.usemask = kROM_Size - 1;
		r.usebase = ROM;
		r.Access = kATTA_readreadymask;
		AddToATTListWithMTB(&r);
	}

	if (MemOverlay)
	{
		r.cmpmask = 0x00E00000;
		r.cmpvalu = kRAM_Overlay_Base;
#if (0 == kRAMb_Size) || (kRAMa_Size == kRAMb_Size)
		r.usemask = kRAM_Size - 1;
		/* note that cmpmask and usemask overlap for 4M */
		r.usebase = RAM;
		r.Access = kATTA_readwritereadymask;
#else
		/* unbalanced memory */
		r.usemask = kRAMb_Size - 1;
		r.usebase = kRAMa_Size + RAM;
		r.Access = kATTA_readwritereadymask;
#endif
		AddToATTListWithMTB(&r);
	}

#if IncludeVidMem
	r.cmpmask = 0x00FFFFFF & ~((1 << kVidMem_ln2Spc) - 1);
	r.cmpvalu = kVidMem_Base;
	r.usemask = kVidMemRAM_Size - 1;
	r.usebase = VidMem;
	r.Access = kATTA_readwritereadymask;
	AddToATTList(&r);
#endif

	r.cmpmask = 0x00FFFFFF & ~((1 << kVIA1_ln2Spc) - 1);
	r.cmpvalu = kVIA1_Block_Base;
	r.usebase = nullptr;
	r.Access = kATTA_mmdvmask;
	r.MMDV = kMMDV_VIA1;
	AddToATTList(&r);

	r.cmpmask = 0x00FFFFFF & ~((1 << kSCC_ln2Spc) - 1);
	r.cmpvalu = kSCCRd_Block_Base;
	r.usebase = nullptr;
	r.Access = kATTA_mmdvmask;
	r.MMDV = kMMDV_SCC;
	AddToATTList(&r);

	r.cmpmask = 0x00FFFFFF & ~((1 << kExtn_ln2Spc) - 1);
	r.cmpvalu = kExtn_Block_Base;
	r.usebase = nullptr;
	r.Access = kATTA_mmdvmask;
	r.MMDV = kMMDV_Extn;
	AddToATTList(&r);

#if CurEmMd == kEmMd_PB100
	r.cmpmask = 0x00FFFFFF & ~((1 << kASC_ln2Spc) - 1);
	r.cmpvalu = kASC_Block_Base;
	r.usebase = nullptr;
	r.Access = kATTA_mmdvmask;
	r.MMDV = kMMDV_ASC;
	AddToATTList(&r);
#endif

	r.cmpmask = 0x00FFFFFF & ~((1 << kSCSI_ln2Spc) - 1);
	r.cmpvalu = kSCSI_Block_Base;
	r.usebase = nullptr;
	r.Access = kATTA_mmdvmask;
	r.MMDV = kMMDV_SCSI;
	AddToATTList(&r);

	r.cmpmask = 0x00FFFFFF & ~((1 << kIWM_ln2Spc) - 1);
	r.cmpvalu = kIWM_Block_Base;
	r.usebase = nullptr;
	r.Access = kATTA_mmdvmask;
	r.MMDV = kMMDV_IWM;
	AddToATTList(&r);
}
#endif

static void SetUpMemBanks(void)
{
	InitATTList();

	SetUp_address();

	FinishATTList();
}

#if 0
static void get_fail_realblock(ATTep p)
{
	p->cmpmask = 0;
	p->cmpvalu = 0xFFFFFFFF;
	p->usemask = 0;
	p->usebase = nullptr;
	p->Access = 0;
}
#endif

uint32_t MMDV_Access(ATTep p, uint32_t Data,
					 bool WriteMem, bool ByteSize, CPTR addr)
{
	switch (p->MMDV)
	{
	case kMMDV_VIA1:
		if (!ByteSize)
		{
#if (CurEmMd == kEmMd_II) || (CurEmMd == kEmMd_IIx)
			if (WriteMem && (addr == 0xF40006))
			{
				/* for weirdness on shutdown in System 6 */
#if 0
					VIA1_Access((Data >> 8) & 0x00FF, WriteMem,
							(addr >> 9) & kVIA1_Mask);
					VIA1_Access((Data) & 0x00FF, WriteMem,
							(addr >> 9) & kVIA1_Mask);
#endif
			}
			else
#endif
			{
				ReportAbnormalID(0x1106, "access VIA1 word");
			}
		}
		else if ((addr & 1) != 0)
		{
			ReportAbnormalID(0x1107, "access VIA1 odd");
		}
		else
		{
#if CurEmMd != kEmMd_PB100
#if (CurEmMd == kEmMd_II) || (CurEmMd == kEmMd_IIx)
			if ((addr & 0x000001FE) != 0x00000000)
#else
			if ((addr & 0x000FE1FE) != 0x000FE1FE)
#endif
			{
				ReportAbnormalID(0x1108,
								 "access VIA1 nonstandard address");
			}
#endif
			Data = VIA1_Access(Data, WriteMem,
							   (addr >> 9) & kVIA1_Mask);
		}

		break;
#if EmVIA2
	case kMMDV_VIA2:
		if (!ByteSize)
		{
			if ((!WriteMem) && ((0x3e00 == (addr & 0x1FFFF)) || (0x3e02 == (addr & 0x1FFFF))))
			{
				/* for weirdness at offset 0x71E in ROM */
				Data =
					(VIA2_Access(Data, WriteMem,
								 (addr >> 9) & kVIA2_Mask)
					 << 8) |
					VIA2_Access(Data, WriteMem,
								(addr >> 9) & kVIA2_Mask);
			}
			else
			{
				ReportAbnormalID(0x1109, "access VIA2 word");
			}
		}
		else if ((addr & 1) != 0)
		{
			if (0x3FFF == (addr & 0x1FFFF))
			{
				/*
					for weirdness at offset 0x7C4 in ROM.
					looks like bug.
				*/
				Data = VIA2_Access(Data, WriteMem,
								   (addr >> 9) & kVIA2_Mask);
			}
			else
			{
				ReportAbnormalID(0x110A, "access VIA2 odd");
			}
		}
		else
		{
			if ((addr & 0x000001FE) != 0x00000000)
			{
				ReportAbnormalID(0x110B,
								 "access VIA2 nonstandard address");
			}
			Data = VIA2_Access(Data, WriteMem,
							   (addr >> 9) & kVIA2_Mask);
		}
		break;
#endif
	case kMMDV_SCC:

#if (CurEmMd >= kEmMd_SE) && !((CurEmMd == kEmMd_II) || (CurEmMd == kEmMd_IIx))

		if ((addr & 0x00100000) == 0)
		{
			ReportAbnormalID(0x110C,
							 "access SCC unassigned address");
		}
		else
#endif
			if (!ByteSize)
		{
			ReportAbnormalID(0x110D, "Attemped Phase Adjust");
		}
		else
#if !((CurEmMd == kEmMd_II) || (CurEmMd == kEmMd_IIx))
			if (WriteMem != ((addr & 1) != 0))
		{
			if (WriteMem)
			{
#if CurEmMd >= kEmMd_512Ke
#if CurEmMd != kEmMd_PB100
				ReportAbnormalID(0x110E, "access SCC even/odd");
				/*
					This happens on boot with 64k ROM.
				*/
#endif
#endif
			}
			else
			{
				SCC_Reset();
			}
		}
		else
#endif
#if (CurEmMd != kEmMd_PB100) && !((CurEmMd == kEmMd_II) || (CurEmMd == kEmMd_IIx))

			if (WriteMem != (addr >= kSCCWr_Block_Base))
		{
			ReportAbnormalID(0x110F, "access SCC wr/rd base wrong");
		}
		else
#endif
		{
#if CurEmMd != kEmMd_PB100
#if (CurEmMd == kEmMd_II) || (CurEmMd == kEmMd_IIx)
			if ((addr & 0x1FF9) != 0x00000000)
#else
			if ((addr & 0x001FFFF8) != 0x001FFFF8)
#endif
			{
				ReportAbnormalID(0x1110,
								 "access SCC nonstandard address");
			}
#endif
			Data = SCC_Access(Data, WriteMem,
							  (addr >> 1) & kSCC_Mask);
		}
		break;
	case kMMDV_Extn:
		if (ByteSize)
		{
			ReportAbnormalID(0x1111, "access Sony byte");
		}
		else if ((addr & 1) != 0)
		{
			ReportAbnormalID(0x1112, "access Sony odd");
		}
		else if (!WriteMem)
		{
			ReportAbnormalID(0x1113, "access Sony read");
		}
		else
		{
			Extn_Access(Data, (addr >> 1) & 0x0F);
		}
		break;
#if EmASC
	case kMMDV_ASC:
		if (!ByteSize)
		{
#if (CurEmMd == kEmMd_II) || (CurEmMd == kEmMd_IIx)
			if (WriteMem)
			{
				(void)ASC_Access((Data >> 8) & 0x00FF,
								 WriteMem, addr & kASC_Mask);
				Data = ASC_Access((Data) & 0x00FF,
								  WriteMem, (addr + 1) & kASC_Mask);
			}
			else
			{
				Data =
					(ASC_Access((Data >> 8) & 0x00FF,
								WriteMem, addr & kASC_Mask)
					 << 8) |
					ASC_Access((Data) & 0x00FF,
							   WriteMem, (addr + 1) & kASC_Mask);
			}
#else
			ReportAbnormalID(0x1114, "access ASC word");
#endif
		}
		else
		{
			Data = ASC_Access(Data, WriteMem, addr & kASC_Mask);
		}
		break;
#endif
	case kMMDV_SCSI:
		if (!ByteSize)
		{
			ReportAbnormalID(0x1115, "access SCSI word");
		}
		else
#if !((CurEmMd == kEmMd_II) || (CurEmMd == kEmMd_IIx))
			if (WriteMem != ((addr & 1) != 0))
		{
			ReportAbnormalID(0x1116, "access SCSI even/odd");
		}
		else
#endif
		{
#if (CurEmMd == kEmMd_II) || (CurEmMd == kEmMd_IIx)
			if ((addr & 0x1F8F) != 0x00000000)
			{
				ReportAbnormalID(0x1117,
								 "access SCSI nonstandard address");
			}
#endif
			Data = SCSI_Access(Data, WriteMem, (addr >> 4) & 0x07);
		}

		break;
	case kMMDV_IWM:
#if (CurEmMd >= kEmMd_SE) && !((CurEmMd == kEmMd_II) || (CurEmMd == kEmMd_IIx))

		if ((addr & 0x00100000) == 0)
		{
			ReportAbnormalID(0x1118,
							 "access IWM unassigned address");
		}
		else
#endif
			if (!ByteSize)
		{
#if ExtraAbnormalReports
			ReportAbnormalID(0x1119, "access IWM word");
			/*
				This happens when quitting 'Glider 3.1.2'.
				perhaps a bad handle is being disposed of.
			*/
#endif
		}
		else
#if (CurEmMd == kEmMd_II) || (CurEmMd == kEmMd_IIx)
			if ((addr & 1) != 0)
		{
			ReportAbnormalID(0x111A, "access IWM odd");
		}
		else
#else
			if ((addr & 1) == 0)
		{
			ReportAbnormalID(0x111B, "access IWM even");
		}
		else
#endif
		{
#if (CurEmMd != kEmMd_PB100) && !((CurEmMd == kEmMd_II) || (CurEmMd == kEmMd_IIx))

			if ((addr & 0x001FE1FF) != 0x001FE1FF)
			{
				ReportAbnormalID(0x111C,
								 "access IWM nonstandard address");
			}
#endif
			Data = IWM_Access(Data, WriteMem,
							  (addr >> 9) & kIWM_Mask);
		}

		break;
	}

	return Data;
}

bool MemAccessNtfy(ATTep pT)
{
	bool v = false;

	switch (pT->Ntfy)
	{
#if CurEmMd >= kEmMd_SE
	case kMAN_OverlayOff:
		pT->Access = kATTA_readreadymask;

		MemOverlay = 0;
		SetUpMemBanks();

		v = true;

		break;
#endif
	}

	return v;
}

void MemOverlay_ChangeNtfy(void)
{
#if CurEmMd <= kEmMd_Plus
	SetUpMemBanks();
#elif (CurEmMd == kEmMd_II) || (CurEmMd == kEmMd_IIx)
	SetUpMemBanks();
#endif
}

#if (CurEmMd == kEmMd_II) || (CurEmMd == kEmMd_IIx)
void Addr32_ChangeNtfy(void)
{
	SetUpMemBanks();
}
#endif

static ATTep get_address_realblock1(bool WriteMem, CPTR addr)
{
	ATTep p;

Label_Retry:
	p = FindATTel(addr);
	if (0 != (p->Access &
			  (WriteMem ? kATTA_writereadymask : kATTA_readreadymask)))
	{
		/* ok */
	}
	else
	{
		if (0 != (p->Access & kATTA_ntfymask))
		{
			if (MemAccessNtfy(p))
			{
				goto Label_Retry;
			}
		}
		p = nullptr; /* fail */
	}

	return p;
}

uint8_t *get_real_address0(uint32_t L, bool WritableMem, CPTR addr,
						   uint32_t *actL)
{
	uint32_t bankleft;
	uint8_t *p;
	ATTep q;

	q = get_address_realblock1(WritableMem, addr);
	if (nullptr == q)
	{
		*actL = 0;
		p = nullptr;
	}
	else
	{
		uint32_t m2 = q->usemask & ~q->cmpmask;
		uint32_t m3 = m2 & ~(m2 + 1);
		p = q->usebase + (addr & q->usemask);
		bankleft = (m3 + 1) - (addr & m3);
		if (bankleft >= L)
		{
			/* this block is big enough (by far the most common case) */
			*actL = L;
		}
		else
		{
			*actL = bankleft;
		}
	}

	return p;
}

static uint8_t CurIPL = 0;

void VIAorSCCinterruptChngNtfy(void)
{
#if (CurEmMd == kEmMd_II) || (CurEmMd == kEmMd_IIx)
	uint8_t NewIPL;

	if (InterruptButton)
	{
		NewIPL = 7;
	}
	else if (SCCInterruptRequest)
	{
		NewIPL = 4;
	}
	else if (VIA2_InterruptRequest)
	{
		NewIPL = 2;
	}
	else if (VIA1_InterruptRequest)
	{
		NewIPL = 1;
	}
	else
	{
		NewIPL = 0;
	}
#else
	uint8_t VIAandNotSCC = VIA1_InterruptRequest & ~SCCInterruptRequest;
	uint8_t NewIPL = VIAandNotSCC | (SCCInterruptRequest << 1) | (InterruptButton << 2);
#endif
	if (NewIPL != CurIPL)
	{
		CurIPL = NewIPL;
		m68k_IPLchangeNtfy();
	}
}

bool AddrSpac_Init(void)
{
	int i;

	for (i = 0; i < kNumWires; i++)
	{
		Wires[i] = 1;
	}

	MINEM68K_Init(
		&CurIPL);
	return true;
}

void Memory_Reset(void)
{
	MemOverlay = 1;
	SetUpMemBanks();
}

#if (CurEmMd == kEmMd_II) || (CurEmMd == kEmMd_IIx)
extern void PowerOff_ChangeNtfy(void);
void PowerOff_ChangeNtfy(void)
{
	if (!VIA2_iB2)
	{
		ForceMacOff = true;
	}
}
#endif