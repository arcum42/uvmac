/*
	Adapted from code in vMac.

	Copyright (C) 2009 Bernd Schmidt, Philip Cummins, Paul C. Pratt

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
	Hardware.
*/

#include "sys_dependencies.h"

#include "UI/os_glue.h"
#include "global_glue.h"

#include "HW/hardware.h"

device_config_t hw_config;
DevMethods_t DEVICES[DEV_MAX];

// Sets up commands that can be called for all the various devices, and iterators over them.
// Should probably be converted to two classes, one for an individual device, and one for all devices, with a way of enabling and disabling them.

void setup_hw_config()
{
	hw_config.classic_keyboard = EmClassicKbrd;
	hw_config.adb = EmADB;
	hw_config.rtc = EmRTC;
	hw_config.pmu = EmPMU;
	hw_config.via2 = EmVIA2;
	hw_config.use68020 = Use68020;
	hw_config.fpu = EmFPU;
	hw_config.mmu = EmMMU;
	hw_config.asc = EmASC;
	hw_config.video_card = EmVidCard;
	hw_config.video_memory = IncludeVidMem;
}

const DevMethods_t null_device = 
{
	.init = nullptr,
	.reset = nullptr,
	.starttick = nullptr,
	.endtick = nullptr,
	.subtick = nullptr,
	.timebegin = nullptr,
	.timeend = nullptr
};

const DevMethods_t rtc_device = 
{
	.init = EmRTC ? RTC_Init : nullptr,
	.reset = nullptr,
	.starttick = nullptr,
	.endtick = nullptr,
	.subtick = nullptr,
	.timebegin = nullptr,
	.timeend = nullptr
};

const DevMethods_t rom_device = 
{
	.init = ROM_Init,
	.reset = nullptr,
	.starttick = nullptr,
	.endtick = nullptr,
	.subtick = nullptr,
	.timebegin = nullptr,
	.timeend = nullptr
};

const DevMethods_t memory_device = 
{
	.init = AddrSpac_Init,
	.reset = Memory_Reset,
	.starttick = nullptr,
	.endtick = nullptr,
	.subtick = nullptr,
	.timebegin = nullptr,
	.timeend = nullptr
};

const DevMethods_t ict_device = 
{
	.init = nullptr,
	.reset = ICT::zap,
	.starttick = nullptr,
	.endtick = nullptr,
	.subtick = nullptr,
	.timebegin = nullptr,
	.timeend = nullptr
};

const DevMethods_t iwm_device = 
{
	.init = nullptr,
	.reset = IWM_Reset,
	.starttick = nullptr,
	.endtick = nullptr,
	.subtick = nullptr,
	.timebegin = nullptr,
	.timeend = nullptr,
};

const DevMethods_t scc_device = 
{
	.init = nullptr,
	.reset = SCC_Reset,
	.starttick = nullptr,
	.endtick = nullptr,
	.subtick = nullptr,
	.timebegin = nullptr,
	.timeend = nullptr
};

const DevMethods_t scsi_device = 
{
	.init = nullptr,
	.reset = SCSI_Reset,
	.starttick = nullptr,
	.endtick = nullptr,
	.subtick = nullptr,
	.timebegin = nullptr,
	.timeend = nullptr,
};

const DevMethods_t via1_device = 
{
	.init = VIA1_Zap,
	.reset = VIA1_Reset,
	.starttick = nullptr,
	.endtick = nullptr,
	.subtick = nullptr,
	.timebegin = VIA1_ExtraTimeBegin,
	.timeend = VIA1_ExtraTimeEnd
};

const DevMethods_t via2_device = 
{
	.init = nullptr,
	.reset = EmVIA2 ? VIA2_Zap : nullptr,
	.starttick = nullptr,
	.endtick = nullptr,
	.subtick = nullptr,
	.timebegin = EmVIA2 ? VIA2_ExtraTimeBegin : nullptr,
	.timeend = EmVIA2 ? VIA2_ExtraTimeEnd : nullptr
};

const DevMethods_t sony_disk_device = 
{
	.init = nullptr,
	.reset = Sony_Reset,
	.starttick = Sony_Update,
	.endtick = nullptr,
	.subtick = nullptr,
	.timebegin = nullptr,
	.timeend = nullptr
};

const DevMethods_t extn_device = 
{
	.init = nullptr,
	.reset = Extn_Reset,
	.starttick = nullptr,
	.endtick = nullptr,
	.subtick = nullptr,
	.timebegin = nullptr,
	.timeend = nullptr,
};

const DevMethods_t cpu_device = 
{
	.init = nullptr,
	.reset = m68k_reset,
	.starttick = nullptr,
	.endtick = nullptr,
	.subtick = nullptr,
	.timebegin = nullptr,
	.timeend = nullptr
};

const DevMethods_t mouse_device = 
{
	.init = nullptr,
	.reset = nullptr,
	.starttick = Mouse_Update,
	.endtick = Mouse_EndTickNotify,
	.subtick = nullptr,
	.timebegin = nullptr,
	.timeend = nullptr
};

const DevMethods_t legacy_keyboard_device = 
{
	.init = nullptr,
	.reset = nullptr,
	.starttick = EmClassicKbrd ? KeyBoard_Update : nullptr,
	.endtick = nullptr,
	.subtick = nullptr,
	.timebegin = nullptr,
	.timeend = nullptr
};

const DevMethods_t adb_device = 
{
	.init = nullptr,
	.reset = nullptr,
	.starttick = EmADB ? ADB_Update : nullptr,
	.endtick = nullptr,
	.subtick = nullptr,
	.timebegin = nullptr,
	.timeend = nullptr
};

const DevMethods_t localtalk_device = 
{
	.init = nullptr,
	.reset = nullptr,
	.starttick = /*EmLocalTalk ? LocalTalkTick : */ nullptr,
	.endtick = nullptr,
    .subtick = nullptr,
	.timebegin = nullptr,
	.timeend = nullptr
};

const DevMethods_t videocard_device = 
{
	.init = EmVidCard ? Vid_Init : nullptr,
	.reset = nullptr,
	.starttick = EmVidCard ? Vid_Update : nullptr,
	.endtick = nullptr,
	.subtick = nullptr,
	.timebegin = nullptr,
	.timeend = nullptr
};

const DevMethods_t asc_device = 
{
	.init = nullptr,
	.reset = nullptr,
	.starttick = nullptr,
	.endtick = nullptr,
	.subtick = EmASC ? ASC_SubTick : nullptr,
	.timebegin = nullptr,
	.timeend = nullptr
};

const DevMethods_t sound_device = 
{
	.init = nullptr,
	.reset = nullptr,
	.starttick = nullptr,
	.endtick = nullptr,
	.subtick = (CurEmMd != kEmMd_PB100) ? MacSound_SubTick : nullptr,
	.timebegin = nullptr,
	.timeend = nullptr
};

const DevMethods_t screen_device = 
{
	.init = nullptr,
	.reset = nullptr,
	.starttick = Sixtieth_PulseNtfy, // VBlank interrupt
	.endtick = Screen_EndTickNotify,
	.subtick = nullptr,
	.timebegin = nullptr,
	.timeend = nullptr
};

void devices_setup()
{
	setup_hw_config();

	if (hw_config.rtc)
		DEVICES[DEV_RTC] = rtc_device;
	else
		DEVICES[DEV_RTC] = null_device;
	
	DEVICES[DEV_ROM] = rom_device;
	DEVICES[DEV_MEMORY] = memory_device;
	DEVICES[DEV_ICT] = ict_device;
	DEVICES[DEV_IWM] = iwm_device;
	DEVICES[DEV_SCC] = scc_device;
	DEVICES[DEV_SCSI] = scsi_device;
	DEVICES[DEV_VIA1] = via1_device;

	if (hw_config.via2)
		DEVICES[DEV_VIA2] = via2_device;
	else
		DEVICES[DEV_VIA2] = null_device;
	
	DEVICES[DEV_SONY] = sony_disk_device;
	DEVICES[DEV_EXTN] = extn_device;
	DEVICES[DEV_68K] = cpu_device;
	DEVICES[DEV_MOUSE] = mouse_device;

	if (hw_config.classic_keyboard)
		DEVICES[DEV_KEYBOARD] = legacy_keyboard_device;
	else
		DEVICES[DEV_KEYBOARD] = null_device;

	if (hw_config.adb)
		DEVICES[DEV_ADB] = adb_device;
	else
		DEVICES[DEV_ADB] = null_device;

	if (EmLocalTalk)
		DEVICES[DEV_LOCALTALK] = localtalk_device;
	else
		DEVICES[DEV_LOCALTALK] = null_device;

	if (hw_config.video_card)
		DEVICES[DEV_VIDEO] = videocard_device;
	else
		DEVICES[DEV_VIDEO] = null_device;

	if (hw_config.asc)
		DEVICES[DEV_ASC] = asc_device;
	else
		DEVICES[DEV_ASC] = null_device;

	if (CurEmMd != kEmMd_PB100)
		DEVICES[DEV_SOUND] = sound_device;
	else
		DEVICES[DEV_SOUND] = null_device;

	DEVICES[DEV_SCREEN] = screen_device;
}

void devices_init(void) 
{
	for(auto &device : DEVICES) {
		if (device.init != nullptr) { device.init(); }
	}
	devices_reset();
}

void devices_reset(void) 
{
	for(auto &device : DEVICES) {
		if (device.reset != nullptr) { device.reset(); }
	} 
}

void devices_starttick(void) 
{
	for(auto &device : DEVICES) {
		if (device.starttick != nullptr) { device.starttick(); }
	} 
}
void devices_endtick(void) 
{
	for(auto &device : DEVICES) {
		if (device.endtick != nullptr) { device.endtick(); }
	} 
}

void devices_subtick(int value) 
{
	for(auto &device : DEVICES) {
		if (device.subtick != nullptr) { device.subtick(value); }
	} 
}

void devices_timebegin() 
{
	for(auto &device : DEVICES) {
		if (device.timebegin != nullptr) { device.timebegin(); }
	} 
}

void devices_timeend() 
{
	for(auto &device : DEVICES) {
		if (device.timeend != nullptr) { device.timeend(); }
	} 
}

void customreset(void)
{
	IWM_Reset();
	SCC_Reset();
	SCSI_Reset();
	VIA1_Reset();
#if EmVIA2
	VIA2_Reset();
#endif
	Sony_Reset();
	Extn_Reset();
#if CurEmMd <= kEmMd_Plus
	WantMacReset = true;
	/*
		kludge, code in Finder appears
		to do RESET and not expect
		to come back. Maybe asserting
		the RESET somehow causes
		other hardware components to
		later reset the 68000.
	*/
#endif
}