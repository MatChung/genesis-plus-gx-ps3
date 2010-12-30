#include <vector>
#include <math.h>
#include <sys/paths.h>
#include <unistd.h> 

#include "cell.h"
#include "menu.h"
#include "cellframework/logger/Logger.h"
#include "cellframework/input/cellInput.h"
#include "cellframework/audio/audioport.hpp"
#include "cellframework/utility/OSKUtil.h"
#include "ps3video.h"
#include "conf/conffile.h"
#include "GenesisPlus.h"

PS3Graphics* Graphics;
CellInputFacade* CellInput;
Audio::Stream<int16_t> *CellAudio;
ConfigFile	*currentconfig = NULL;
OSKUtil *oskutil;

//define struct
struct SSettings Settings;

#define VIDEO_WIDTH  320 
#define VIDEO_HEIGHT 240
#define SAMPLERATE_48KHZ 48000
#define SAMPLERATE_48_3KHZ 48300

// mode the main loop is in
Emulator_Modes mode_switch = MODE_MENU;

// is a ROM running
bool emulation_running;

// is emulator loaded?
bool emulator_loaded = false;

// needs settings loaded
bool load_settings = true;

// need to load the current rom
bool need_load_rom = true;

// current rom being emulated
char* current_rom = NULL;

#define SYS_CONFIG_FILE "/dev_hdd0/game/GENP00001/USRDIR/genesisplus.conf"
SYS_PROCESS_PARAM(1001, 0x10000);

static bool runbios = 0;

#define SB_SIZE 7680
unsigned char soundbuffer[7680];

extern "C"
{

#include "genplusgx/shared.h"

static int config_load(void)
{
	/* open configuration file */
	char fname[MAXPATHLEN];
	sprintf (fname, "/dev_hdd0/game/GENP00001/config.bin");
	FILE *fp = fopen(fname, "rb");
	if (fp)
	{
		/* read version */
		char version[16];
		fread(version, 16, 1, fp); 
		fclose(fp);

		if (strncmp(version,CONFIG_VERSION,16))
		{
			return 0;
		}

		/* read file */
		fp = fopen(fname, "rb");
		if (fp)
		{
			fread(&config, sizeof(config), 1, fp);
			fclose(fp);
			return 1;
		}
	}
	return 0;
}

void config_save(void)
{
	/* open configuration file */
	char fname[MAXPATHLEN];
	sprintf (fname, "/dev_hdd0/game/GENP00001/config.bin");
	FILE *fp = fopen(fname, "wb");
	if (fp)
	{ 
		/* write file */
		fwrite(&config, sizeof(config), 1, fp);
		fclose(fp);
	}
}

void config_default()
{
	/* version TAG */
	strncpy(config.version,CONFIG_VERSION,16);

	int i;

	/* sound options */
	config.psg_preamp     = 150;
	config.fm_preamp      = 100;
	config.hq_fm          = 1;
	config.psgBoostNoise  = 0;
	config.filter         = 1;
	config.low_freq       = 200;
	config.high_freq      = 8000;
	config.lg             = 1.0;
	config.mg             = 1.0;
	config.hg             = 1.0;
	config.lp_range       = 60;
	config.rolloff        = 0.995;
	config.dac_bits       = 14;

	/* system options */
	config.region_detect  = 0;
	config.force_dtack    = 0;
	config.addr_error     = 1;
	config.tmss           = 0;
	config.lock_on        = Settings.ExtraCart;
	config.romtype        = 0;
	config.hot_swap       = 0;

	/* video options */
	config.xshift   = 0;
	config.yshift   = 0;
	config.xscale   = 0;
	config.yscale   = 0;
	config.aspect   = 1;
	config.overscan = 0;

	/* controllers options */
	config.gun_cursor[0]  = 1;
	config.gun_cursor[1]  = 1;
	config.invert_mouse   = 0;
	config.s_device = 0;

	
	for (i=0;i<MAX_INPUTS;i++)
	{
		if(Settings.SixButtonPad)
		{
			config.input[i].padtype = DEVICE_6BUTTON;
		}
		else
		{
			config.input[i].padtype = DEVICE_3BUTTON;
		}
		config.input[i].device	= -1;
		config.input[i].port	= i%4;
	}

	input.system[0]       = SYSTEM_GAMEPAD;
	input.system[1]       = SYSTEM_GAMEPAD;
}


/***************************************************************************
 * Genesis Plus Virtual Machine
 *
 ***************************************************************************/
static void load_bios(void)
{
	LOG_DBG("load_bios()\n");
	// clear BIOS detection flag
	config.tmss &= ~2;

	// open BIOS file
	LOG_DBG("Settings.BIOS = %s\n", Settings.BIOS.c_str());
	FILE *fp = fopen(Settings.BIOS.c_str(), "rb");
	if (fp == NULL)
	{
		return;
	}

	// read file
	fread(bios_rom, 1, 0x800, fp);
	fclose(fp);

	// check ROM file
	if (!strncmp((char *)(bios_rom + 0x120),"GENESIS OS", 10))
	{
		// valid BIOS detected
		config.tmss |= 2;
	}
}

static void init_machine()
{
	LOG_DBG("init_machine()\n");

	/* allocate cart.rom here (10 MBytes) */
	cart.rom = (unsigned char*)memalign(32, MAXROMSIZE);
	
	/* BIOS support */
	load_bios();

	/* allocate global work bitmap */
	memset (&bitmap, 0, sizeof (bitmap));
	bitmap.width  = 320;
	bitmap.height = 224;
	bitmap.depth  = 32;
    	bitmap.granularity = 4;
    	bitmap.pitch = (bitmap.width * bitmap.granularity);
    	bitmap.data   = (unsigned char *)memalign(1024,bitmap.width*bitmap.height*bitmap.granularity);
	bitmap.viewport.w = 256;
	bitmap.viewport.h = 224;
	bitmap.viewport.x = 0;
	bitmap.viewport.y = 0;
    	bitmap.remap = 1;

	/* force video update */
	bitmap.viewport.changed = 3;

}

void ChangePathAndExt(char *file,char* ext,char *out)
{
	LOG_DBG("ChangePathAndExt(%s, %s, %s)\n", file, ext, out);
	int nLen,pos; 
	char *myfiles;
	// Get Size !!! 
	nLen = strlen (file); 
	pos = nLen - 1;

	if ((nLen > 0) && (nLen < 256))
	{ 
		//extract file name only
		while (pos >= 0)
		{
			if (file[pos] == '/')
			{
				pos++;
				myfiles = (char*)file + pos;
				LOG_DBG("file=%s\n", myfiles);
				break;
			}
			pos--;
		}
		while (nLen >= 0)
		{ 
			// Check for extension character remove extention !!! 
			if (myfiles [nLen] == '.')
			{
				myfiles [nLen] = '\0'; 
				LOG_DBG("file no ext =%s\n", myfiles);
				break; 
			} 
			nLen --; 
		} 
	// Create output file name and with new extension 
	sprintf (out, "%s/%s%s",out,myfiles,ext);
	LOG_DBG("Final path %s \n",out);
	} 
}

void LoadSRAM()
{
	LOG_DBG("LoadSRAM()\n");
	char * currentsram;
	currentsram = new char [999];
	strcpy(currentsram, Settings.PS3PathSRAM.c_str());
	LOG_DBG("Settings.PS3PathSRAM.size() = %d\n", Settings.PS3PathSRAM.size());
	LOG_DBG("Settings.PS3PathSRAM = %s\n", Settings.PS3PathSRAM.c_str());
	LOG_DBG("SRAM path: %s \n",currentsram);
	ChangePathAndExt(current_rom,".srm",currentsram);
	FILE *f = fopen(currentsram,"rb");
	if (f!= NULL)
	{
		fread(sram.sram,0x10000,1, f);
		fclose(f);
		LOG_DBG("Out from SRAM\n");
	}
}



void SaveSRAM()
{
	LOG_DBG("SaveSRAM()\n");
	char * currentsram;
	currentsram = new char [999];
	strcpy(currentsram, Settings.PS3PathSRAM.c_str());
	LOG_DBG("Settings.PS3PathSRAM.size() = %d\n", Settings.PS3PathSRAM.size());
	LOG_DBG("Settings.PS3PathSRAM = %s\n", Settings.PS3PathSRAM.c_str());
	LOG_DBG("SRAM path: %s \n",currentsram);
	ChangePathAndExt(current_rom,".srm",currentsram);
	FILE *f = fopen(currentsram,"wb");
	if (f!= NULL)
	{
		fwrite(&sram.sram,0x10000,1, f);
		LOG_DBG("Writing of SRAM done!\n");
		fclose(f);
		LOG_DBG("Out from SRAM\n");
	}
}

void reloadrom(int size, char *name)
{
	LOG_DBG("reloadrom(%d, %s)\n", size, current_rom);
	bool hotswap = config.hot_swap && cart.romsize;

	/* load ROM file */
	LOG_DBG("cart.romsize = %d\n", size);
	cart.romsize = size;
	LOG_DBG("load_rom(%s)\n", name);
	load_rom(name);
	
	LOG_DBG("hotswap: %d\n", hotswap);
	LOG_DBG("region detect: %d\n", config.region_detect);
	if(hotswap)
	{
		cart_hw_init();
		cart_hw_reset(1);
	}

	/* initialize audio back-end */
	/* 60hz video mode requires synchronization with Video Interrupt.    */
	/* Framerate is 59.94 fps in interlaced/progressive modes, ~59.825 fps in non-interlaced mode */
	LOG_DBG("vdp_pal: %d\n", vdp_pal);
	LOG_DBG("config.tv_mode: %d\n", config.tv_mode);
	LOG_DBG("config.render: %d\n", config.render);
	float framerate;
	if (vdp_pal)
	{
		framerate = 49.82;
	}
	else
	{
		framerate = ((config.tv_mode == 0) || (config.tv_mode == 2)) ? (1000000.0/16715.0) : 60.0;
	}

	LOG_DBG("audio_init(%d, %f)\n", SAMPLERATE_48_3KHZ, framerate);
	LOG_DBG("framerate: %f\n", framerate);
	audio_init(SAMPLERATE_48_3KHZ, framerate);

	/* System Power ON */
	LOG_DBG("system_init()\n");
	system_init();
	LOG_DBG("system_reset()\n");
	system_reset();

	/* load SRAM */
	LoadSRAM();

	//FIXME: TODO
	/* load State */
}


void LoadState()
{
	LOG_DBG("LoadState()\n");
	char * currentsavestate;
	currentsavestate = new char [999];
	strcpy(currentsavestate, Settings.PS3PathSaveStates.c_str());
	LOG_DBG("Settings.PS3PathSaveStates.size() = %d\n", Settings.PS3PathSaveStates.size());
	LOG_DBG("Settings.PS3PathSaveStates = %s\n", Settings.PS3PathSaveStates.c_str());
	LOG_DBG("Savestate path: %s \n",currentsavestate);
	ChangePathAndExt(current_rom,".sav",currentsavestate);
        FILE *f = fopen(currentsavestate,"rb");
        if (f)
        {
		LOG_DBG("f: %s\n", currentsavestate);
		LOG_DBG("Enter in loadstate\n");
          	uint8 *buf =(uint8 *)memalign(32,STATE_SIZE);
          	fread(buf, STATE_SIZE, 1, f);
          	state_load(buf);
          	fclose(f);
		free(buf);
		LOG_DBG("Out from loadstate\n");
        }
}

void SaveState()
{
	LOG_DBG("SaveState()\n");
	char * currentsavestate;
	currentsavestate = new char [999];
	strcpy(currentsavestate, Settings.PS3PathSaveStates.c_str());
	LOG_DBG("Settings.PS3PathSaveStates.size() = %d\n", Settings.PS3PathSaveStates.size());
	LOG_DBG("Settings.PS3PathSaveStates = %s\n", Settings.PS3PathSaveStates.c_str());
	LOG_DBG("Savestate path: %s \n",currentsavestate);
	ChangePathAndExt(current_rom,".sav",currentsavestate);
        FILE *f = fopen(currentsavestate,"wb");
        if (f)
        {
		LOG("f: %s\n", currentsavestate);
		printf("Enter in savestate\n");
		uint8 *buf =(uint8 *)memalign(32,STATE_SIZE);
		int state_size = state_save(buf);
		LOG_DBG("Writing savestate...\n");
		fwrite(buf, state_size, 1, f);
		LOG_DBG("Writing of savestate done!\n");
		fclose(f);
		free(buf);
		LOG_DBG("Out from savestate\n");
        }
}

int special_button_mappings(int controllerno, int specialbuttonmap)
{
	if((specialbuttonmap != INPUT_QUIT) &&
	(specialbuttonmap != INPUT_SAVESTATE) &&
	(specialbuttonmap != INPUT_LOADSTATE) &&
	(specialbuttonmap != INPUT_SOFTRESET) &&
	(specialbuttonmap != INPUT_HARDRESET))
	{
		input.pad[controllerno] |= specialbuttonmap;
	}
	else
	{
		switch(specialbuttonmap)
		{
			case INPUT_SOFTRESET:
				gen_softreset(0);
				break;
			case INPUT_HARDRESET:
				system_reset();
				break;
			case INPUT_QUIT:
				Emulator_StopROMRunning();
				Emulator_SwitchMode(MODE_MENU);
				break;
			case INPUT_SAVESTATE:
				SaveState();
				break;
			case INPUT_LOADSTATE:
				LoadState();
				break;
			default:
				break;
		}
	}
}

int ps3_update_input(void)
{
	uint32_t buttons;
	uint8_t pads_connected = CellInput->NumberPadsConnected();

	for(uint8_t i=0;i < pads_connected; ++i)
	{
		if(CellInput->UpdateDevice(i) != CELL_PAD_OK)
		{
			continue;
		}

		/* clear key status */
    		input.pad[i] = 0;

    		if (CellInput->IsButtonPressed(i, CTRL_UP) | CellInput->IsAnalogPressedUp(i, CTRL_LSTICK))
		{
			special_button_mappings(i,Settings.DPad_Up);
		}
		else if (CellInput->IsButtonPressed(i,CTRL_DOWN) | CellInput->IsAnalogPressedDown(i, CTRL_LSTICK))
		{
			special_button_mappings(i,Settings.DPad_Down);
		}
    		if (CellInput->IsButtonPressed(i,CTRL_LEFT) | CellInput->IsAnalogPressedLeft(i, CTRL_LSTICK))
		{
			special_button_mappings(i,Settings.DPad_Left);
		}
    		else if (CellInput->IsButtonPressed(i,CTRL_RIGHT) | CellInput->IsAnalogPressedRight(i, CTRL_LSTICK))
		{
			special_button_mappings(i,Settings.DPad_Right);
		}
    		if (CellInput->IsButtonPressed(i,CTRL_SQUARE))
		{
			special_button_mappings(i,Settings.ButtonSquare);
		}
		if (CellInput->IsButtonPressed(i,CTRL_CROSS))
		{
			special_button_mappings(i,Settings.ButtonCross);
		}
    		if (CellInput->IsButtonPressed(i,CTRL_CIRCLE))
		{
			special_button_mappings(i,Settings.ButtonCircle);
		}
    		if (CellInput->IsButtonPressed(i,CTRL_TRIANGLE))
		{
			special_button_mappings(i,Settings.ButtonTriangle);
		}
    		if (CellInput->IsButtonPressed(i,CTRL_START))
		{
			special_button_mappings(i,Settings.ButtonStart);
		}
    		if (CellInput->IsButtonPressed(i,CTRL_SELECT))
		{
			special_button_mappings(i,Settings.ButtonSelect);
		}
		if (CellInput->IsButtonPressed(i,CTRL_L1))
		{
			special_button_mappings(i,Settings.ButtonL1);
		}
		if (CellInput->IsButtonPressed(i,CTRL_L2))
		{
			special_button_mappings(i,Settings.ButtonL2);
		}
		if (CellInput->IsButtonPressed(i,CTRL_L3))
		{
			special_button_mappings(i,Settings.ButtonL3);
		}
		if (CellInput->IsButtonPressed(i,CTRL_R1))
		{
			special_button_mappings(i,Settings.ButtonR1);
		}
		if (CellInput->IsButtonPressed(i,CTRL_R2))
		{
			special_button_mappings(i,Settings.ButtonR2);
		}
		if (CellInput->IsButtonPressed(i,CTRL_R3))
		{
			special_button_mappings(i,Settings.ButtonR3);
		}
		if ((CellInput->IsButtonPressed(i,CTRL_L3) && CellInput->IsButtonPressed(i,CTRL_R3)))
		{
			special_button_mappings(i,Settings.ButtonR3_ButtonL3);
		}
		if ((CellInput->IsButtonPressed(i,CTRL_R2) && CellInput->WasButtonPressed(i,CTRL_R3)))    
		{
			special_button_mappings(i,Settings.ButtonR2_ButtonR3);
		}
		if ((CellInput->IsButtonPressed(i,CTRL_L2) && CellInput->WasButtonPressed(i,CTRL_R2)))
		{
			special_button_mappings(i,Settings.ButtonL2_ButtonR2);
		}
		if ((CellInput->IsButtonPressed(i,CTRL_L2) && CellInput->WasButtonPressed(i,CTRL_R3)))
		{
			special_button_mappings(i,Settings.ButtonL2_ButtonR3);
		}
		if ((CellInput->IsButtonPressed(i,CTRL_L2) && CellInput->WasButtonPressed(i,CTRL_L3)))
		{
			special_button_mappings(i,Settings.ButtonL2_ButtonL3);
		}
		if (CellInput->IsButtonPressed(i,CTRL_L2) && CellInput->WasAnalogPressedRight(i,CTRL_RSTICK))
		{
			special_button_mappings(i,Settings.ButtonL2_AnalogR_Right);
		}
		if (CellInput->IsButtonPressed(i,CTRL_L2) && CellInput->WasAnalogPressedLeft(i,CTRL_RSTICK))
		{
			special_button_mappings(i,Settings.ButtonL2_AnalogR_Left);
		}
		if (CellInput->IsButtonPressed(i,CTRL_L2) && CellInput->WasAnalogPressedUp(i,CTRL_RSTICK))
		{
			special_button_mappings(i,Settings.ButtonL2_AnalogR_Up);
		}
		if (CellInput->IsButtonPressed(i,CTRL_L2) && CellInput->WasAnalogPressedDown(i,CTRL_RSTICK))
		{
			special_button_mappings(i,Settings.ButtonL2_AnalogR_Down);
		}
		if (CellInput->IsButtonPressed(i,CTRL_R2) && CellInput->WasAnalogPressedRight(i,CTRL_RSTICK))
		{
			special_button_mappings(i,Settings.ButtonR2_AnalogR_Right);
		}
		if (CellInput->IsButtonPressed(i,CTRL_R2) && CellInput->WasAnalogPressedLeft(i,CTRL_RSTICK))
		{
			special_button_mappings(i,Settings.ButtonR2_AnalogR_Left);
		}
		if (CellInput->IsButtonPressed(i,CTRL_R2) && CellInput->WasAnalogPressedUp(i,CTRL_RSTICK))
		{
			special_button_mappings(i,Settings.ButtonR2_AnalogR_Up);
		}
		if (CellInput->IsButtonPressed(i,CTRL_R2) && CellInput->WasAnalogPressedDown(i,CTRL_RSTICK))
		{
			special_button_mappings(i,Settings.ButtonR2_AnalogR_Down);
		}
		if (Settings.AnalogR_Down_Type ? CellInput->IsAnalogPressedDown(i,CTRL_RSTICK) : CellInput->WasAnalogPressedDown(i,CTRL_RSTICK))
		{
			special_button_mappings(i,Settings.AnalogR_Down);
		}
		if (Settings.AnalogR_Up_Type ? CellInput->IsAnalogPressedUp(i,CTRL_RSTICK) : CellInput->WasAnalogPressedUp(i,CTRL_RSTICK))
		{
			special_button_mappings(i,Settings.AnalogR_Up);
		}
		if (Settings.AnalogR_Left_Type ? CellInput->IsAnalogPressedLeft(i,CTRL_RSTICK) : CellInput->WasAnalogPressedLeft(i,CTRL_RSTICK))
		{
			special_button_mappings(i,Settings.AnalogR_Left);
		}
		if (Settings.AnalogR_Right_Type ? CellInput->IsAnalogPressedRight(i,CTRL_RSTICK) : CellInput->WasAnalogPressedRight(i,CTRL_RSTICK))
		{
			special_button_mappings(i,Settings.AnalogR_Right);
		}
	}

	return 1;
}
//end extern C
}

float Emulator_GetFontSize()
{
	return Settings.PS3FontSize/100.0;
}

bool Emulator_Init()
{
	LOG_DBG("Emulator_Init()\n");
	config_default();
	init_machine();

	int size = cart.romsize;
	cart.romsize = 0;
	struct stat st;
	stat(current_rom, &st);
	reloadrom(st.st_size, current_rom);

	mode_switch = MODE_EMULATION;

	emulator_loaded = true;
	need_load_rom = false;
	return 1;
}

bool Emulator_Initialize()
{
	LOG_DBG("Emulator_Initialize()\n");
	emulator_loaded = Emulator_Init();
}

bool Emulator_IsInitialized()
{
	return emulator_loaded;
}

bool Emulator_IsROMLoaded()
{
	return current_rom != NULL && need_load_rom == false;
}

bool Emulator_ROMRunning()
{
	return emulation_running;
}

char * Emulator_GetROM()
{
	return current_rom;
}

void Emulator_SwitchMode(Emulator_Modes m)
{
	mode_switch = m;
}

void Emulator_SetControllerMode()
{
}

void Emulator_Implementation_ButtonMappingSettings(bool map_button_option_enum)
{
	switch(map_button_option_enum)
	{
		case MAP_BUTTONS_OPTION_SETTER:
			currentconfig->SetInt("PS3ButtonMappings::DPad_Up",Settings.DPad_Up);
			currentconfig->SetInt("PS3ButtonMappings::DPad_Down",Settings.DPad_Down);
			currentconfig->SetInt("PS3ButtonMappings::DPad_Left",Settings.DPad_Left);
			currentconfig->SetInt("PS3ButtonMappings::DPad_Right",Settings.DPad_Right);
			currentconfig->SetInt("PS3ButtonMappings::ButtonCircle",Settings.ButtonCircle);
			currentconfig->SetInt("PS3ButtonMappings::ButtonCross",Settings.ButtonCross);
			currentconfig->SetInt("PS3ButtonMappings::ButtonTriangle",Settings.ButtonTriangle);
			currentconfig->SetInt("PS3ButtonMappings::ButtonSquare",Settings.ButtonSquare);
			currentconfig->SetInt("PS3ButtonMappings::ButtonSelect",Settings.ButtonSelect);
			currentconfig->SetInt("PS3ButtonMappings::ButtonStart",Settings.ButtonStart);
			currentconfig->SetInt("PS3ButtonMappings::ButtonL1",Settings.ButtonL1);
			currentconfig->SetInt("PS3ButtonMappings::ButtonR1",Settings.ButtonR1);
			currentconfig->SetInt("PS3ButtonMappings::ButtonL2",Settings.ButtonL2);
			currentconfig->SetInt("PS3ButtonMappings::ButtonR2",Settings.ButtonR2);
			currentconfig->SetInt("PS3ButtonMappings::ButtonL2_ButtonL3",Settings.ButtonL2_ButtonL3);
			currentconfig->SetInt("PS3ButtonMappings::ButtonL2_ButtonR3",Settings.ButtonL2_ButtonR3);
			currentconfig->SetInt("PS3ButtonMappings::ButtonR3",Settings.ButtonR3);
			currentconfig->SetInt("PS3ButtonMappings::ButtonL3",Settings.ButtonL3);
			currentconfig->SetInt("PS3ButtonMappings::ButtonL2_ButtonR2",Settings.ButtonL2_ButtonR2);
			currentconfig->SetInt("PS3ButtonMappings::ButtonL2_AnalogR_Right",Settings.ButtonL2_AnalogR_Right);
			currentconfig->SetInt("PS3ButtonMappings::ButtonL2_AnalogR_Left",Settings.ButtonL2_AnalogR_Left);
			currentconfig->SetInt("PS3ButtonMappings::ButtonL2_AnalogR_Up",Settings.ButtonL2_AnalogR_Up);
			currentconfig->SetInt("PS3ButtonMappings::ButtonL2_AnalogR_Down",Settings.ButtonL2_AnalogR_Down);
			currentconfig->SetInt("PS3ButtonMappings::ButtonR2_AnalogR_Right",Settings.ButtonR2_AnalogR_Right);
			currentconfig->SetInt("PS3ButtonMappings::ButtonR2_AnalogR_Left",Settings.ButtonR2_AnalogR_Left);
			currentconfig->SetInt("PS3ButtonMappings::ButtonR2_AnalogR_Up",Settings.ButtonR2_AnalogR_Up);
			currentconfig->SetInt("PS3ButtonMappings::ButtonR2_AnalogR_Down",Settings.ButtonR2_AnalogR_Down);
			currentconfig->SetInt("PS3ButtonMappings::ButtonR2_ButtonR3",Settings.ButtonR2_ButtonR3);
			currentconfig->SetInt("PS3ButtonMappings::ButtonR3_ButtonL3",Settings.ButtonR3_ButtonL3);
			currentconfig->SetInt("PS3ButtonMappings::AnalogR_Up",Settings.AnalogR_Up);
			currentconfig->SetInt("PS3ButtonMappings::AnalogR_Down",Settings.AnalogR_Down);
			currentconfig->SetInt("PS3ButtonMappings::AnalogR_Left",Settings.AnalogR_Left);
			currentconfig->SetInt("PS3ButtonMappings::AnalogR_Right",Settings.AnalogR_Right);

			currentconfig->SetBool("PS3ButtonMappings::AnalogR_Up_Type",Settings.AnalogR_Up_Type);
			currentconfig->SetBool("PS3ButtonMappings::AnalogR_Down_Type",Settings.AnalogR_Down_Type);
			currentconfig->SetBool("PS3ButtonMappings::AnalogR_Left_Type",Settings.AnalogR_Left_Type);
			currentconfig->SetBool("PS3ButtonMappings::AnalogR_Right_Type",Settings.AnalogR_Right_Type);
			break;
		case MAP_BUTTONS_OPTION_GETTER:
			Settings.DPad_Up		= currentconfig->GetInt("PS3ButtonMappings::DPad_Up",INPUT_UP);
			Settings.DPad_Down		= currentconfig->GetInt("PS3ButtonMappings::DPad_Down",INPUT_DOWN);
			Settings.DPad_Left		= currentconfig->GetInt("PS3ButtonMappings::DPad_Left",INPUT_LEFT);
			Settings.DPad_Right		= currentconfig->GetInt("PS3ButtonMappings::DPad_Right",INPUT_RIGHT);
			Settings.ButtonCircle		= currentconfig->GetInt("PS3ButtonMappings::ButtonCircle",INPUT_C);
			Settings.ButtonCross		= currentconfig->GetInt("PS3ButtonMappings::ButtonCross",INPUT_B);
			Settings.ButtonTriangle		= currentconfig->GetInt("PS3ButtonMappings::ButtonTriangle",INPUT_X);
			Settings.ButtonSquare		= currentconfig->GetInt("PS3ButtonMappings::ButtonSquare",INPUT_A);
			Settings.ButtonSelect		= currentconfig->GetInt("PS3ButtonMappings::ButtonSelect",INPUT_MODE);
			Settings.ButtonStart		= currentconfig->GetInt("PS3ButtonMappings::ButtonStart",INPUT_START);
			Settings.ButtonL1		= currentconfig->GetInt("PS3ButtonMappings::ButtonL1",INPUT_Y);
			Settings.ButtonR1		= currentconfig->GetInt("PS3ButtonMappings::ButtonR1",INPUT_Z);
			Settings.ButtonL2		= currentconfig->GetInt("PS3ButtonMappings::ButtonL2",INPUT_NONE);
			Settings.ButtonR2		= currentconfig->GetInt("PS3ButtonMappings::ButtonR2",INPUT_NONE);
			Settings.ButtonL2_ButtonL3	= currentconfig->GetInt("PS3ButtonMappings::ButtonL2_ButtonL3",INPUT_NONE);
			Settings.ButtonL2_ButtonR3	= currentconfig->GetInt("PS3ButtonMappings::ButtonL2_ButtonR3",INPUT_LOADSTATE);
			Settings.ButtonR3		= currentconfig->GetInt("PS3ButtonMappings::ButtonR3",INPUT_NONE);
			Settings.ButtonL3		= currentconfig->GetInt("PS3ButtonMappings::ButtonL3",INPUT_NONE);
			Settings.ButtonL2_ButtonR2	= currentconfig->GetInt("PS3ButtonMappings::ButtonL2_ButtonR2",INPUT_NONE);
			Settings.ButtonL2_AnalogR_Right = currentconfig->GetInt("PS3ButtonMappings::ButtonL2_AnalogR_Right",INPUT_NONE);
			Settings.ButtonL2_AnalogR_Left	= currentconfig->GetInt("PS3ButtonMappings::ButtonL2_AnalogR_Left",INPUT_NONE);
			Settings.ButtonL2_AnalogR_Up	= currentconfig->GetInt("PS3ButtonMappings::ButtonL2_AnalogR_Up",INPUT_NONE);
			Settings.ButtonL2_AnalogR_Down	= currentconfig->GetInt("PS3ButtonMappings::ButtonL2_AnalogR_Down",INPUT_NONE);
			Settings.ButtonR2_AnalogR_Right	= currentconfig->GetInt("PS3ButtonMappings::ButtonR2_AnalogR_Right",INPUT_NONE);
			Settings.ButtonR2_AnalogR_Left	= currentconfig->GetInt("PS3ButtonMappings::ButtonR2_AnalogR_Left",INPUT_NONE);
			Settings.ButtonR2_AnalogR_Up	= currentconfig->GetInt("PS3ButtonMappings::ButtonR2_AnalogR_Up",INPUT_NONE);
			Settings.ButtonR2_AnalogR_Down	= currentconfig->GetInt("PS3ButtonMappings::ButtonR2_AnalogR_Down",INPUT_NONE);
			Settings.ButtonR2_ButtonR3	= currentconfig->GetInt("PS3ButtonMappings::ButtonR2_ButtonR3",INPUT_SAVESTATE);
			Settings.ButtonR3_ButtonL3	= currentconfig->GetInt("PS3ButtonMappings::ButtonR3_ButtonL3",INPUT_QUIT);
			Settings.AnalogR_Up		= currentconfig->GetInt("PS3ButtonMappings::AnalogR_Up",INPUT_NONE);
			Settings.AnalogR_Down		= currentconfig->GetInt("PS3ButtonMappings::AnalogR_Down",INPUT_NONE);
			Settings.AnalogR_Left		= currentconfig->GetInt("PS3ButtonMappings::AnalogR_Left",INPUT_NONE);
			Settings.AnalogR_Right		= currentconfig->GetInt("PS3ButtonMappings::AnalogR_Right",INPUT_NONE);

			Settings.AnalogR_Up_Type	= currentconfig->GetBool("PS3ButtonMappings::AnalogR_Up_Type",false);
			Settings.AnalogR_Down_Type	= currentconfig->GetBool("PS3ButtonMappings::AnalogR_Down_Type",false);
			Settings.AnalogR_Left_Type	= currentconfig->GetBool("PS3ButtonMappings::AnalogR_Left_Type",false);
			Settings.AnalogR_Right_Type	= currentconfig->GetBool("PS3ButtonMappings::AnalogR_Right_Type",false);
			break;
		case MAP_BUTTONS_OPTION_DEFAULT:
			Settings.DPad_Up			= INPUT_UP;
			Settings.DPad_Down			= INPUT_DOWN;
			Settings.DPad_Left			= INPUT_LEFT;
			Settings.DPad_Right			= INPUT_RIGHT;
			Settings.ButtonCircle			= INPUT_C;
			Settings.ButtonCross			= INPUT_B;
			Settings.ButtonTriangle			= INPUT_X;
			Settings.ButtonSquare			= INPUT_A;
			Settings.ButtonSelect			= INPUT_MODE;
			Settings.ButtonStart			= INPUT_START;
			Settings.ButtonL1			= INPUT_Y;
			Settings.ButtonR1			= INPUT_Z;
			Settings.ButtonL2			= INPUT_NONE;
			Settings.ButtonR2			= INPUT_NONE;
			Settings.ButtonL2_ButtonL3		= INPUT_NONE;
			Settings.ButtonL2_ButtonR3		= INPUT_LOADSTATE;	
			Settings.ButtonR3			= INPUT_NONE;
			Settings.ButtonL3			= INPUT_NONE;
			Settings.ButtonL2_ButtonR2		= INPUT_NONE;
			Settings.ButtonL2_AnalogR_Right		= INPUT_NONE;
			Settings.ButtonL2_AnalogR_Left		= INPUT_NONE;
			Settings.ButtonL2_AnalogR_Up		= INPUT_NONE;
			Settings.ButtonL2_AnalogR_Down		= INPUT_NONE;
			Settings.ButtonR2_AnalogR_Right		= INPUT_NONE;
			Settings.ButtonR2_AnalogR_Left		= INPUT_NONE;
			Settings.ButtonR2_AnalogR_Up		= INPUT_NONE;
			Settings.ButtonR2_AnalogR_Down		= INPUT_NONE;
			Settings.ButtonR2_ButtonR3		= INPUT_SAVESTATE;
			Settings.ButtonR3_ButtonL3		= INPUT_QUIT;
			Settings.AnalogR_Up			= INPUT_NONE;
			Settings.AnalogR_Down			= INPUT_NONE;
			Settings.AnalogR_Left			= INPUT_NONE;
			Settings.AnalogR_Right			= INPUT_NONE;
			Settings.AnalogR_Up_Type		= false;
			Settings.AnalogR_Down_Type		= false;
			Settings.AnalogR_Left_Type		= false;
			Settings.AnalogR_Right_Type		= false;
			break;
	}
}

bool Emulator_SaveSettings()
{
	if (currentconfig != NULL)
	{
		currentconfig->SetBool("PS3General::DisplayFrameRate",Settings.DisplayFrameRate);
		currentconfig->SetBool("PS3General::KeepAspect",Settings.PS3KeepAspect);
		currentconfig->SetBool("PS3General::Smooth", Settings.PS3Smooth);
		currentconfig->SetBool("PS3General::OverscanEnabled", Settings.PS3OverscanEnabled);
		currentconfig->SetInt("PS3General::OverscanAmount",Settings.PS3OverscanAmount);
		currentconfig->SetInt("PS3General::PS3FontSize",Settings.PS3FontSize);
		currentconfig->SetBool("PS3General::PS3PALTemporalMode60Hz",Settings.PS3PALTemporalMode60Hz);
		currentconfig->SetString("PS3General::Shader",Graphics->GetFragmentShaderPath());
		currentconfig->SetString("PS3Paths::PathSaveStates",Settings.PS3PathSaveStates);
		currentconfig->SetString("PS3Paths::PathROMDirectory",Settings.PS3PathROMDirectory);
		currentconfig->SetString("PS3Paths::PathSRAM",Settings.PS3PathSRAM);
		currentconfig->SetString("RSound::RSoundServerIPAddress",Settings.RSoundServerIPAddress);
		currentconfig->SetBool("RSound::RSoundEnabled",Settings.RSoundEnabled);
		currentconfig->SetInt("GenesisPlus::SixButtonPad",Settings.SixButtonPad);
		currentconfig->SetString("GenesisPlus::BIOS",Settings.BIOS);
		Emulator_Implementation_ButtonMappingSettings(MAP_BUTTONS_OPTION_SETTER);
		return currentconfig->SaveTo(SYS_CONFIG_FILE);
	}

	return false;
}

void Emulator_OSKStart(const wchar_t* msg, const wchar_t* init)
{
	oskutil->Start(msg,init);
}

const char * Emulator_OSKOutputString()
{
	return oskutil->OutputString();
}

void Emulator_Shutdown()
{
	Emulator_SaveSettings();
	sys_process_exit(0);
}

static bool try_load_config_file (const char *fname, ConfigFile &conf)
{
	LOG_DBG("try_load_config_file(%s)\n", fname);
	FILE * fp;

	fp = fopen(fname, "r");
	if (fp)
	{
		fprintf(stdout, "Reading config file %s.\n", fname);
		conf.LoadFile(new fReader(fp));
		fclose(fp);
	}

	return (false);
}

bool Emulator_InitSettings()
{
	LOG_DBG("Emulator_InitSettings()\n");

	if (currentconfig == NULL)
	{
		currentconfig = new ConfigFile();
	}

	memset((&Settings), 0, (sizeof(Settings)));

	currentconfig->Clear();

	try_load_config_file(SYS_CONFIG_FILE, *currentconfig);

	//PS3 - General settings
	if (currentconfig->Exists("PS3General::KeepAspect"))
	{
		Settings.PS3KeepAspect		=	currentconfig->GetBool("PS3General::KeepAspect");
	}
	else
	{
		Settings.PS3KeepAspect		=	true;
	}
	Graphics->SetAspectRatio(Settings.PS3KeepAspect);

	if (currentconfig->Exists("PS3General::Smooth"))
	{
		Settings.PS3Smooth		=	currentconfig->GetBool("PS3General::Smooth");
	}
	else
	{
		Settings.PS3Smooth		=	false;
	}
	Graphics->SetSmooth(Settings.PS3Smooth);

	if (currentconfig->Exists("PS3General::OverscanEnabled"))
	{
		Settings.PS3OverscanEnabled	= currentconfig->GetBool("PS3General::OverscanEnabled");
	}
	else
	{
		Settings.PS3OverscanEnabled	= false;
	}
	if (currentconfig->Exists("PS3General::OverscanAmount"))
	{
		Settings.PS3OverscanAmount	= currentconfig->GetInt("PS3General::OverscanAmount");
	}
	else
	{
		Settings.PS3OverscanAmount	= 0;
	}
	Graphics->SetOverscan(Settings.PS3OverscanEnabled, (float)Settings.PS3OverscanAmount/100);

	if (currentconfig->Exists("GenesisPlus::SixButtonPad"))
	{
		Settings.SixButtonPad = currentconfig->GetBool("GenesisPlus::SixButtonPad");
	}
	else
	{
		Settings.SixButtonPad = FALSE;
	}

	if (currentconfig->Exists("PS3General::DisplayFrameRate"))
	{
		Settings.DisplayFrameRate = currentconfig->GetBool("PS3General::DisplayFrameRate");
	}
	else
	{
		Settings.DisplayFrameRate = false;
	}

	if (currentconfig->Exists("PS3General::Shader"))
	{
		Graphics->LoadFragmentShader(currentconfig->GetString("PS3General::Shader"));
	}
	else
	{
		Graphics->LoadFragmentShader(DEFAULT_SHADER_FILE);
	}
	if (currentconfig->Exists("GenesisPlus::BIOS"))
	{
		Settings.BIOS = currentconfig->GetString("GenesisPlus::BIOS");
	}
	else
	{
		Settings.BIOS = "/dev_hdd0/game/GENP00001/USRDIR/bios.bin";
	}
	if (currentconfig->Exists("PS3General::PS3PALTemporalMode60Hz"))
	{
		Settings.PS3PALTemporalMode60Hz = currentconfig->GetBool("PS3General::PS3PALTemporalMode60Hz");
		Graphics->SetPAL60Hz(Settings.PS3PALTemporalMode60Hz);
	}
	else
	{
		Settings.PS3PALTemporalMode60Hz = false;
		Graphics->SetPAL60Hz(Settings.PS3PALTemporalMode60Hz);
	}
	//RSound Settings
	if(currentconfig->Exists("RSound::RSoundEnabled"))
	{
		Settings.RSoundEnabled		= currentconfig->GetBool("RSound::RSoundEnabled");
	}
	else
	{
		Settings.RSoundEnabled		= false;
	}
	if(currentconfig->Exists("RSound::RSoundServerIPAddress"))
	{
		Settings.RSoundServerIPAddress	= currentconfig->GetString("RSound::RSoundServerIPAddress");
	}
	else
	{
		Settings.RSoundServerIPAddress = "0.0.0.0";
	}
	if(currentconfig->Exists("PS3General::PS3FontSize"))
	{
		Settings.PS3FontSize		= currentconfig->GetInt("PS3General::PS3FontSize");
	}
	else
	{
		Settings.PS3FontSize		= 100;
	}

	// PS3 Path Settings
	if (currentconfig->Exists("PS3Paths::PathSaveStates"))
	{
		Settings.PS3PathSaveStates		= currentconfig->GetString("PS3Paths::PathSaveStates");
	}
	else
	{
		Settings.PS3PathSaveStates		= "/dev_hdd0/game/GENP00001/USRDIR/";
	}

	if (currentconfig->Exists("PS3Paths::PathSRAM"))
	{
		Settings.PS3PathSRAM		= currentconfig->GetString("PS3Paths::PathSRAM");
	}
	else
	{
		Settings.PS3PathSRAM		= "/dev_hdd0/game/GENP00001/USRDIR/";
	}

	/*
	if (currentconfig->Exists("PS3Paths::PathScreenshots"))
	{
		Settings.PS3PathScreenshots		= currentconfig->GetString("PS3Paths::PathScreenshots");
	}
	*/

	if (currentconfig->Exists("PS3Paths::PathROMDirectory"))
	{
		Settings.PS3PathROMDirectory		= currentconfig->GetString("PS3Paths::PathROMDirectory");
	}
	else
	{
		Settings.PS3PathROMDirectory		= "/\0";
	}

	if (currentconfig->Exists("GenesisPlus::ActionReplayROMPath"))
	{
		Settings.GameGenieROMPath = currentconfig->GetString("GenesisPlus::ActionReplayROMPath");
	}
	else
	{
		Settings.ActionReplayROMPath.assign("/dev_hdd0/game/GENP00001/USRDIR/areplay.bin");
	}
	if (currentconfig->Exists("GenesisPlus::GameGenieROMPath"))
	{
		Settings.GameGenieROMPath = currentconfig->GetString("GenesisPlus::GameGenieROMPath");
	}
	else
	{
		Settings.GameGenieROMPath.assign("/dev_hdd0/game/GENP00001/USRDIR/ggenie.bin");
	}
	if (currentconfig->Exists("GenesisPlus::SKROMPath"))
	{
		Settings.SKROMPath = currentconfig->GetString("GenesisPlus::SKROMPath");
	}
	else
	{
		Settings.SKROMPath.assign("/dev_hdd0/game/GENP00001/USRDIR/sk.bin");
	}
	if (currentconfig->Exists("GenesisPlus::SKUpmemROMPath"))
	{
		Settings.SKUpmemROMPath = currentconfig->GetString("GenesisPlus::SKUpmemROMPath");
	}
	else
	{
		Settings.SKUpmemROMPath.assign("/dev_hdd0/game/GENP00001/USRDIR/sk2chip.bin");
	}
	if (currentconfig->Exists("GenesisPlus::ExtraCart"))
	{
		Settings.ExtraCart = currentconfig->GetBool("GenesisPlus::ExtraCart");
	}
	else
	{
		Settings.ExtraCart = false;
	}
	Emulator_SetExtraCartPaths();

	Emulator_Implementation_ButtonMappingSettings(MAP_BUTTONS_OPTION_GETTER);

	LOG_DBG("SUCCESS - Emulator_InitSettings()\n");
	return true;
}

void Emulator_SetExtraCartPaths()
{
		LOG_DBG("Emulator_SetExtraCartPaths()\n");
		sprintf(AR_ROM,"%s",Settings.ActionReplayROMPath.c_str());
		LOG_DBG("AR_ROM: %s\n", AR_ROM);
		sprintf(GG_ROM,"%s",Settings.GameGenieROMPath.c_str());
		LOG_DBG("GG_ROM: %s\n", GG_ROM);
		sprintf(SK_ROM,"%s",Settings.SKROMPath.c_str());
		LOG_DBG("SK_ROM: %s\n", SK_ROM);
		sprintf(SK_UPMEM,"%s",Settings.SKUpmemROMPath.c_str());
		LOG_DBG("SK_UPMEM: %s\n", SK_UPMEM);
}



void Emulator_RequestLoadROM(char* rom, bool forceReload)
{
	LOG_DBG("Emulator_RequestLoadROM(%s, %d\n", rom, forceReload);
	if (forceReload || current_rom == NULL || strcmp(rom, current_rom) != 0)
	{
		if (current_rom != NULL)
		{
			free(current_rom);
		}

		current_rom = strdup(rom);
		need_load_rom = true;
	}
	else
	{
		need_load_rom = false;
	}
}

void Emulator_StopROMRunning()
{
	emulation_running = false;
}

void Emulator_StartROMRunning()
{
	Emulator_SwitchMode(MODE_EMULATION);
}

void sysutil_exit_callback (uint64_t status, uint64_t param, void *userdata) {
	(void) param;
	(void) userdata;

	switch (status) {
		case CELL_SYSUTIL_REQUEST_EXITGAME:
			MenuStop();
			Emulator_StopROMRunning();
			Emulator_SwitchMode(MODE_EXIT);
			break;
		case CELL_SYSUTIL_DRAWING_BEGIN:
		case CELL_SYSUTIL_DRAWING_END:
			break;
		case CELL_SYSUTIL_OSKDIALOG_LOADED:
			break;
		case CELL_SYSUTIL_OSKDIALOG_FINISHED:
			oskutil->Stop();
			break;
		case CELL_SYSUTIL_OSKDIALOG_UNLOADED:
			oskutil->Close();
			break;
		default:
			break;
	}
}

static void PlaySound()
{
   int size = audio_update() * 2;
   CellAudio->write((const int16_t* )soundbuffer, size);
}

void CreateFolder(char* folders)
{
	if(mkdir(folders,0777))
	{
		gl_dprintf(0.09f,0.05f,Emulator_GetFontSize(),"ERROR - Could not create folder: %s \nPlease check your GenesisConf.ini\n",folders);
		sys_timer_sleep(5);
		sys_process_exit(0);
	}
}

void Emulator_Start()
{
	if(need_load_rom)
	{
		LOG_DBG("need_load_rom: %d\n", need_load_rom);
		if(Emulator_Initialize())
		{
			need_load_rom = false;
		}
	}
	//bring down dbgfont
	Graphics->DeinitDbgFont();
	if (Graphics->GetCurrentResolution() == CELL_VIDEO_OUT_RESOLUTION_576)
	{
		if(Graphics->CheckResolution(CELL_VIDEO_OUT_RESOLUTION_576))
		{
			if(vdp_pal)
			{
				if(Graphics->GetPAL60Hz())
				{
					//PAL60 is ON, turn it off for PAL
					Graphics->SetPAL60Hz(false);
					Settings.PS3PALTemporalMode60Hz = false;
					Graphics->SwitchResolution(Graphics->GetCurrentResolution(), Settings.PS3PALTemporalMode60Hz);
				}
			}
			else
			{
				if(!Graphics->GetPAL60Hz())
				{
					//PAL60 is OFF, turn it on for NTSC
					Graphics->SetPAL60Hz(true);
					Settings.PS3PALTemporalMode60Hz = true;
					Graphics->SwitchResolution(Graphics->GetCurrentResolution(), Settings.PS3PALTemporalMode60Hz);
				}
			}
					
		}
	}
	CellAudio->unpause();
	while(emulation_running)
	{
		system_frame(0);
		Graphics->Draw(bitmap.viewport.w,bitmap.viewport.h,bitmap.data);
		Graphics->Swap();

		PlaySound();

		//check interlaced mode change
		if (bitmap.viewport.changed & 4)
		{
			//stub
			bitmap.viewport.changed &= ~4;
		}
		cellSysutilCheckCallback();
	}
	SaveSRAM();
	Graphics->InitDbgFont();
	emulation_running = true;
}

void Emulator_ToggleSound()
{
	LOG_DBG("Emulator_ToggleSound()\n");
	if(CellAudio)
	{
		delete CellAudio;
	}
	if((Settings.RSoundEnabled) && (strlen(Settings.RSoundServerIPAddress) > 0))
	{
		CellAudio = new Audio::RSound<int16_t>(Settings.RSoundServerIPAddress, 2, SAMPLERATE_48KHZ);
		// If we couldn't connect, fall back to normal audio...
		if (!CellAudio->alive())
		{
			delete CellAudio;
			CellAudio = new Audio::AudioPort<int16_t>(2, SAMPLERATE_48KHZ);
			Settings.RSoundEnabled = false;
			Graphics->Clear();
			cellDbgFontPuts(0.09f, 0.4f, 1.0f, 0xffffffff, "Couldn't connect to RSound server.\nFalling back to regular audio...");
			Graphics->FlushDbgFont();
			Graphics->Swap();
			sys_timer_usleep(3000000);
		}
	}
	else
	{
   		CellAudio = new Audio::AudioPort<int16_t>(2, SAMPLERATE_48KHZ);
	}
}

int main() {
	struct stat st;

	sys_spu_initialize(6, 1); 
	cellSysmoduleLoadModule(CELL_SYSMODULE_FS);
	cellSysmoduleLoadModule(CELL_SYSMODULE_IO);

	cellSysutilRegisterCallback(0, sysutil_exit_callback, NULL); 
	LOG_INIT();
	LOG_DBG("LOG INIT\n");

	mode_switch = MODE_MENU;

	Graphics = new PS3Graphics();
	LOG_DBG("Graphics->Init()\n");
	Graphics->Init();

	CellInput = new CellInputFacade();
	LOG_DBG("CellInput->Init()\n");
	CellInput->Init();

	oskutil = new OSKUtil();

	// FIXME: Is this necessary?
	if (Graphics->InitCg() != CELL_OK)
	{
		LOG_DBG("Failed to InitCg: %d\n", __LINE__);
		exit(0);
	}

	LOG_DBG("Graphics->InitDbgFont()\n");
	Graphics->InitDbgFont();

	Emulator_ToggleSound();

	emulation_running = true;

	/*
	if (ini_parse("/dev_hdd0/game/GENP00001/USRDIR/GenesisConf.ini", handler, &Iniconfig) < 0)
	{
		gl_dprintf(0.09f,0.05f,Emulator_GetFontSize(),"Could not load /dev_hdd0/game/GENP00001/GenesisConf.ini\n");
		sys_timer_sleep(5);
		gl_dprintf(0.09f,0.05f,Emulator_GetFontSize(),"Now exiting to XMB...\n");
		sys_timer_sleep(5);
		sys_process_exit(0);
	}
	*/

	//REPLACEMENT
	if (load_settings)
	{
		Emulator_InitSettings();
		load_settings =  false;
	}
	

	/*
	//main path - Check if not present - create all folders and exit
	if(stat(Iniconfig.rompath,&st) != 0)
	{
		gl_dprintf(0.09f,0.05f,Emulator_GetFontSize(),"Creating generic folder tree for Genesisplus...\n");
		sys_timer_sleep(5);
		CreateFolder(Iniconfig.rompath);
		CreateFolder(Iniconfig.savpath);
		CreateFolder(Iniconfig.cartpath);
		CreateFolder(Iniconfig.sram_path);
		CreateFolder(Iniconfig.biospath);
		gl_dprintf(0.09f,0.05f,Emulator_GetFontSize(),"Generic folder tree done! Will now exit to XMB...\nPlease put all your ROMs inside %s\n",Iniconfig.rompath);
		sys_timer_sleep(5);
		sys_process_exit(0);
	}
	*/

	////Set Bios
	//sprintf(Iniconfig.biospath,"%s/bios.bin",Iniconfig.biospath);

	while(1)
	{
		switch(mode_switch)
		{
			case MODE_MENU:
				MenuMainLoop();
				break;
			case MODE_EMULATION:
				Emulator_Start();
				CellAudio->pause();
				break;
			case MODE_EXIT:
				Emulator_Shutdown();
		}
	}

	return 0;
}
