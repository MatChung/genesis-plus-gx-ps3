/*
 * menu.cpp
 *
 *  Created on: Oct 10, 2010
 *      Author: halsafar
 */
#include <string.h>
#include <stack>
#include <vector>

#include "cellframework/input/cellInput.h"
#include "cellframework/fileio/FileBrowser.h"
#include "cellframework/logger/Logger.h"
#include <cell/pad.h>
#include <cell/audio.h>
#include <cell/sysmodule.h>
#include <cell/cell_fs.h>
#include <cell/dbgfont.h>
#include <sysutil/sysutil_sysparam.h>

#include "ps3video.h"
#include "menu.h"

#include "GenesisPlus.h"
#include "conf/conffile.h"

#define MIN(x,y) ((x) < (y) ? (x) : (y))
#define MAX(x,y) ((x) > (y) ? (x) : (y))

#define NUM_ENTRY_PER_PAGE 22
#define NUM_ENTRY_PER_SETTINGS_PAGE 18

// is the menu running
bool menuRunning = false;

// menu to render
typedef void (*curMenuPtr)();

//
std::stack<curMenuPtr> menuStack;

// main file browser->for rom browser
FileBrowser* browser = NULL;

// tmp file browser->for everything else
FileBrowser* tmpBrowser = NULL;

int16_t currently_selected_setting = 		FIRST_GENERAL_SETTING;
int16_t currently_selected_genesis_setting =	FIRST_GENESIS_SETTING;
int16_t currently_selected_path_setting =	FIRST_PATH_SETTING;
int16_t currently_selected_controller_setting = FIRST_CONTROLS_SETTING;

#define FILEBROWSER_DELAY	100000
#define SETTINGS_DELAY		150000	

void MenuStop()
{
	menuRunning = false;
}


bool MenuIsRunning()
{
	return menuRunning;
}

void UpdateBrowser(FileBrowser* b)
{
	if (CellInput->WasButtonHeld(0,CTRL_DOWN) | CellInput->IsAnalogPressedDown(0,CTRL_LSTICK))
	{
		if(b->GetCurrentEntryIndex() < b->GetCurrentDirectoryInfo().numEntries-1)
		{
			b->IncrementEntry();
			if(CellInput->IsButtonPressed(0,CTRL_DOWN))
			{
				sys_timer_usleep(FILEBROWSER_DELAY);
			}
		}
	}
	if (CellInput->WasButtonHeld(0,CTRL_UP) | CellInput->IsAnalogPressedUp(0,CTRL_LSTICK))
	{
		if(b->GetCurrentEntryIndex() > 0)
		{
			b->DecrementEntry();
			if(CellInput->IsButtonPressed(0,CTRL_UP))
			{
				sys_timer_usleep(FILEBROWSER_DELAY);
			}
		}
	}
	if (CellInput->WasButtonHeld(0,CTRL_RIGHT) | CellInput->IsAnalogPressedRight(0,CTRL_LSTICK))
	{
		b->GotoEntry(MIN(b->GetCurrentEntryIndex()+5, b->GetCurrentDirectoryInfo().numEntries-1));
		if(CellInput->IsButtonPressed(0,CTRL_RIGHT))
		{
			sys_timer_usleep(FILEBROWSER_DELAY);
		}
	}
	if (CellInput->WasButtonHeld(0, CTRL_LEFT) | CellInput->IsAnalogPressedLeft(0,CTRL_LSTICK))
	{
		if (b->GetCurrentEntryIndex() <= 5)
		{
			b->GotoEntry(0);
			if(CellInput->IsButtonPressed(0,CTRL_LEFT))
			{
				sys_timer_usleep(FILEBROWSER_DELAY);
			}
		}
		else
		{
			b->GotoEntry(b->GetCurrentEntryIndex()-5);
			if(CellInput->IsButtonPressed(0,CTRL_LEFT))
			{
				sys_timer_usleep(FILEBROWSER_DELAY);
			}
		}
	}
	if (CellInput->WasButtonHeld(0,CTRL_R1))
	{
		b->GotoEntry(MIN(b->GetCurrentEntryIndex()+NUM_ENTRY_PER_PAGE, b->GetCurrentDirectoryInfo().numEntries-1));
		if(CellInput->IsButtonPressed(0,CTRL_R1))
		{
			sys_timer_usleep(FILEBROWSER_DELAY);
		}
	}
	if (CellInput->WasButtonHeld(0,CTRL_L1))
	{
		if (b->GetCurrentEntryIndex() <= NUM_ENTRY_PER_PAGE)
		{
			b->GotoEntry(0);
		}
		else
		{
			b->GotoEntry(b->GetCurrentEntryIndex()-NUM_ENTRY_PER_PAGE);
		}
		if(CellInput->IsButtonPressed(0,CTRL_L1))
		{
			sys_timer_usleep(FILEBROWSER_DELAY);
		}
	}

	if (CellInput->WasButtonPressed(0, CTRL_CIRCLE))
	{
		// don't let people back out past root
		if (b->DirectoryStackCount() > 1)
		{
			b->PopDirectory();
		}
	}
}

void RenderBrowser(FileBrowser* b)
{
	uint32_t file_count = b->GetCurrentDirectoryInfo().numEntries;
	int current_index = b->GetCurrentEntryIndex();

	if (file_count <= 0)
	{
		printf("1: filecount <= 0");
		cellDbgFontPuts		(0.09f,	0.05f,	Emulator_GetFontSize(),	RED,	"No Roms founds!!!\n");
	}
	else if (current_index > file_count)
	{
		printf("2: current_index >= file_count");
	}
	else
	{
		int page_number = current_index / NUM_ENTRY_PER_PAGE;
		int page_base = page_number * NUM_ENTRY_PER_PAGE;
		float currentX = 0.09f;
		float currentY = 0.09f;
		float ySpacing = 0.035f;
		for (int i = page_base; i < file_count && i < page_base + NUM_ENTRY_PER_PAGE; ++i)
		{
			currentY = currentY + ySpacing;
			cellDbgFontPuts(currentX, currentY, Emulator_GetFontSize(),
					i == current_index ? RED : (*b)[i]->d_type == CELL_FS_TYPE_DIRECTORY ? GREEN : WHITE,
					(*b)[i]->d_name);
			Graphics->FlushDbgFont();
		}
	}
	Graphics->FlushDbgFont();
}

void do_extracartChoice()
{
	if (tmpBrowser == NULL)
	{
		tmpBrowser = new FileBrowser("/\0");
	}
	string path;

	if (CellInput->UpdateDevice(0) == CELL_PAD_OK)
	{
		UpdateBrowser(tmpBrowser);

		if (CellInput->WasButtonPressed(0,CTRL_CROSS))
		{
			if(tmpBrowser->IsCurrentADirectory())
			{
				tmpBrowser->PushDirectory(	tmpBrowser->GetCurrentDirectoryInfo().dir + "/" + tmpBrowser->GetCurrentEntry()->d_name,
										CELL_FS_TYPE_REGULAR | CELL_FS_TYPE_DIRECTORY,
										"bin|BIN");
			}
			else if (tmpBrowser->IsCurrentAFile())
			{
				path = tmpBrowser->GetCurrentDirectoryInfo().dir + "/" + tmpBrowser->GetCurrentEntry()->d_name;
				switch(currently_selected_genesis_setting)
				{
					case SETTING_GENESIS_ACTIONREPLAY_ROMPATH:
						Settings.ActionReplayROMPath.assign(path);
						LOG_DBG("Settings.ActionReplayROMPath: %s\n", Settings.ActionReplayROMPath.c_str());
						break;
					case SETTING_GENESIS_GAMEGENIE_ROMPATH:
						Settings.GameGenieROMPath.assign(path);
						LOG_DBG("Settings.GameGenieROMPath: %s\n", Settings.GameGenieROMPath.c_str());
						break;	
					case SETTING_GENESIS_SK_ROMPATH:
						Settings.SKROMPath.assign(path);
						LOG_DBG("Settings.SKROMPath: %s\n", Settings.SKROMPath.c_str());
						break;	
					case SETTING_GENESIS_SK_UPMEM_ROMPATH:
						Settings.SKUpmemROMPath.assign(path);
						LOG_DBG("Settings.SKUpmemROMPath: %s\n", Settings.SKUpmemROMPath.c_str());
						break;	
					case SETTING_GENESIS_BIOS_ROMPATH:
						Settings.BIOS.assign(path);
						LOG_DBG("Settings.BIOS: %s\n", Settings.BIOS.c_str());
						break;	
				}
				Emulator_SetExtraCartPaths();
				menuStack.pop();

			}
		}

		if (CellInput->WasButtonHeld(0, CTRL_TRIANGLE))
		{
			switch(currently_selected_genesis_setting)
			{
				case SETTING_GENESIS_ACTIONREPLAY_ROMPATH:
					Settings.ActionReplayROMPath.assign("/dev_hdd0/game/GENP00001/USRDIR/areplay.bin");
					LOG_DBG("Settings.ActionReplayROMPath: %s\n", Settings.ActionReplayROMPath.c_str());
					break;
				case SETTING_GENESIS_GAMEGENIE_ROMPATH:
					Settings.GameGenieROMPath.assign("/dev_hdd0/game/GENP00001/USRDIR/ggenie.bin");
					LOG_DBG("Settings.GameGenieROMPath: %s\n", Settings.GameGenieROMPath.c_str());
					break;	
				case SETTING_GENESIS_SK_ROMPATH:
					Settings.SKROMPath.assign("/dev_hdd0/game/GENP00001/USRDIR/sk.bin");
					LOG_DBG("Settings.SKROMPath: %s\n", Settings.SKROMPath.c_str());
					break;	
				case SETTING_GENESIS_SK_UPMEM_ROMPATH:
					Settings.SKUpmemROMPath.assign("/dev_hdd0/game/GENP00001/USRDIR/sk2chip.bin");
					LOG_DBG("Settings.SKUpmemROMPath: %s\n", Settings.SKUpmemROMPath.c_str());
					break;	
				case SETTING_GENESIS_BIOS_ROMPATH:
					Settings.BIOS.assign("/dev_hdd0/game/GENP00001/USRDIR/bios.bin");
					LOG_DBG("Settings.BIOS: %s\n", Settings.BIOS.c_str());
					break;	
			}
			Emulator_SetExtraCartPaths();
			menuStack.pop();
		}
	}

	cellDbgFontPrintf(0.09f, 0.09f, Emulator_GetFontSize(), YELLOW, "PATH: %s", tmpBrowser->GetCurrentDirectoryInfo().dir.c_str());
	cellDbgFontPuts	(0.09f,	0.05f,	Emulator_GetFontSize(),	RED,	"EXTRA CARTRIDGE SELECTION");
	cellDbgFontPuts(0.05f, 0.92f, Emulator_GetFontSize(), YELLOW,
	"X - Select directory/select ROM /\\ - return to settings");
	cellDbgFontPrintf(0.09f, 0.89f, 0.86f, LIGHTBLUE, "%s",
	"INFO - Select an extra cartridge ROM from the menu by pressing the X button.");
	Graphics->FlushDbgFont();

	RenderBrowser(tmpBrowser);
}

void do_shaderChoice()
{
	if (tmpBrowser == NULL)
	{
		tmpBrowser = new FileBrowser("/dev_hdd0/game/GENP00001/USRDIR/shaders/\0");
	}

	string path;

	if (CellInput->UpdateDevice(0) == CELL_PAD_OK)
	{
		UpdateBrowser(tmpBrowser);

		if (CellInput->WasButtonPressed(0,CTRL_CROSS))
		{
			if(tmpBrowser->IsCurrentADirectory())
			{
				tmpBrowser->PushDirectory(	tmpBrowser->GetCurrentDirectoryInfo().dir + "/" + tmpBrowser->GetCurrentEntry()->d_name,
						CELL_FS_TYPE_REGULAR | CELL_FS_TYPE_DIRECTORY, "cg");
			}
			else if (tmpBrowser->IsCurrentAFile())
			{
				path = tmpBrowser->GetCurrentDirectoryInfo().dir + "/" + tmpBrowser->GetCurrentEntry()->d_name;

				//load shader
				Graphics->LoadFragmentShader(path);
				menuStack.pop();
			}
		}

		if (CellInput->WasButtonHeld(0, CTRL_TRIANGLE))
		{
			menuStack.pop();
		}
	}

	cellDbgFontPrintf(0.09f, 0.09f, Emulator_GetFontSize(), YELLOW, "PATH: %s", tmpBrowser->GetCurrentDirectoryInfo().dir.c_str());
	cellDbgFontPuts	(0.09f,	0.05f,	Emulator_GetFontSize(),	RED,	"SHADER SELECTION");
	cellDbgFontPuts(0.09f, 0.92f, Emulator_GetFontSize(), YELLOW,
	"X - Select shader               /\\ - return to settings");
	cellDbgFontPrintf(0.09f, 0.89f, 0.86f, LIGHTBLUE, "%s",
	"INFO - Select a shader from the menu by pressing the X button. ");
	Graphics->FlushDbgFont();

	RenderBrowser(tmpBrowser);
}

void DisplayHelpMessage(int currentsetting)
{
	switch(currentsetting)
	{
		case SETTING_PATH_SAVESTATES_DIRECTORY:
			cellDbgFontPrintf(0.09f, 0.80f, Emulator_GetFontSize(), WHITE, "%s", "Set the default path where all the savestate files will be saved.\n");
			break;
		case SETTING_PATH_DEFAULT_ROM_DIRECTORY:
			cellDbgFontPrintf(0.09f, 0.83f, 0.86f, LIGHTBLUE, "%s", "INFO - Set the default ROM startup directory. NOTE: You will have to\nrestart the emulator for this change to have any effect.\n");
			break;
		case SETTING_PATH_SRAM_DIRECTORY:
			cellDbgFontPrintf(0.09f, 0.80f, Emulator_GetFontSize(), WHITE, "%s", "Set the default SRAM (SaveRAM) directory path.\n");
			break;
		case SETTING_SHADER:
			cellDbgFontPrintf(0.09f, 0.80f, Emulator_GetFontSize(), WHITE, "%s", "Select a pixel shader. NOTE: Some shaders might be too slow at 1080p.\nIf you experience any slowdown, try another shader.");
			break;
		case SETTING_GENESIS_FPS:
			cellDbgFontPrintf(0.09f, 0.83f, 0.86f, LIGHTBLUE, "%s", Settings.DisplayFrameRate ? "INFO - Display Framerate is set to 'Enabled' - an FPS counter is displayed onscreen.\nNOTE: Not working yet." : "INFO - Display Framerate is set to 'Disabled'.");
			break;
		case SETTING_GENESIS_PAD:
			cellDbgFontPrintf(0.09f, 0.80f, Emulator_GetFontSize(), WHITE, "%s", Settings.SixButtonPad ? "Controller type is set to '6 buttons' - certain games will now have\n6-button pad support." : "Controller type is set to '3 buttons'.");
			break;
		case SETTING_CHANGE_RESOLUTION:
			cellDbgFontPrintf(0.09f, 0.83f, 0.86f, LIGHTBLUE, "%s", "INFO - Change the display resolution - press X to confirm.");
			#ifndef PS3_SDK_3_41
				cellDbgFontPrintf(0.09f, 0.86f, 0.86f, RED, "%s", "WARNING - This setting might not work correctly on 1.92 FW.");
			#endif
			break;
/*
		case SETTING_PAL60_MODE:
			cellDbgFontPrintf(0.09f, 0.80f, Emulator_GetFontSize(), WHITE, "%s", Settings.PS3PALTemporalMode60Hz ? "PAL 60Hz mode is enabled - 60Hz NTSC games will run correctly at 576p PAL\nresolution. NOTE: This is configured on-the-fly." : "PAL 60Hz mode disabled - 50Hz PAL games will run correctly at 576p PAL\nresolution. NOTE: This is configured on-the-fly.");
			break;
*/
		case SETTING_HW_TEXTURE_FILTER:
			cellDbgFontPrintf(0.09f, 0.80f, Emulator_GetFontSize(), WHITE, "%s", Settings.PS3Smooth ? "Hardware filtering is set to 'Bilinear filtering'." : "Hardware filtering is set to 'Point filtering' - most shaders\nlook much better on this setting.");
			break;
		case SETTING_KEEP_ASPECT_RATIO:
			cellDbgFontPrintf(0.09f, 0.80f, Emulator_GetFontSize(), WHITE, "%s", Settings.PS3KeepAspect ? "Aspect ratio is set to 'Scaled' - screen will have black borders\nleft and right on widescreen TVs/monitors." : "Aspect ratio is set to 'Stretched' - widescreen TVs/monitors will have\na full stretched picture.");
			break;
		case SETTING_RSOUND_ENABLED:
			cellDbgFontPrintf(0.09f, 0.80f, Emulator_GetFontSize(), WHITE, "%s", Settings.RSoundEnabled ? "Sound is set to 'RSound' - the sound will be streamed over the network\nto the RSound audio server." : "Sound is set to 'Normal' - normal audio output will be used.");
			break;
		case SETTING_RSOUND_SERVER_IP_ADDRESS:
			cellDbgFontPuts(0.09f, 0.80f, Emulator_GetFontSize(), WHITE, "Enter the IP address of the RSound audio server. IP address must be\nan IPv4 32-bits address, eg: '192.168.0.7'.");
			break;
		case SETTING_FONT_SIZE:
			cellDbgFontPuts(0.09f, 0.80f, Emulator_GetFontSize(), WHITE, "Increase or decrease the font size in the menu.");
			break;
		case SETTING_HW_OVERSCAN_AMOUNT:
			cellDbgFontPuts(0.09f, 0.83f, 0.86f, LIGHTBLUE, "INFO - Adjust or decrease overscan. Set this to higher than 0.000\nif the screen doesn't fit on your TV/monitor.");
			break;
		case SETTING_DEFAULT_ALL:
			cellDbgFontPuts(0.09f, 0.80f, Emulator_GetFontSize(), WHITE, "Set all 'General' settings back to their default values.");
			break;
		case SETTING_GENESIS_DEFAULT_ALL:
			cellDbgFontPuts(0.09f, 0.80f, Emulator_GetFontSize(), WHITE, "Set all 'Genesis' settings back to their default values.");
			break;
		case SETTING_PATH_DEFAULT_ALL:
			cellDbgFontPuts(0.09f, 0.80f, Emulator_GetFontSize(), WHITE, "Set all 'Path' settings back to their default values.");
			break;
		case SETTING_GENESIS_EXCART:
			cellDbgFontPrintf(0.09f, 0.80f, Emulator_GetFontSize(), WHITE, "%s", Settings.ExtraCart ? "Extra Cartridge is set to 'Enabled'. NOTE: this will not do anything\nif you have not specified the path to the lock-on ROM carts." : "Extra Cartridge is set to 'Disabled'.");
			break;
		case SETTING_GENESIS_ACTIONREPLAY_ROMPATH:
			cellDbgFontPuts(0.09f, 0.80f, Emulator_GetFontSize(), WHITE, "To enable Action Replay functionality, you need to have the file\nareplay.bin somewhere on your HDD/USB device. Select the file here.");
			break;
		case SETTING_GENESIS_GAMEGENIE_ROMPATH:
			cellDbgFontPuts(0.09f, 0.80f, Emulator_GetFontSize(), WHITE, "To enable Game Genie functionality, you need to have the file ggenie.bin\nsomewhere on your HDD/USB device. Select the file here.");
			break;
		case SETTING_GENESIS_SK_ROMPATH:
			cellDbgFontPuts(0.09f, 0.80f, Emulator_GetFontSize(), WHITE, "To enable S&K lock-on functionality, you need to have the file sk.bin\nsomewhere on your HDD/USB device. Select the file here.");
			break;
		case SETTING_GENESIS_SK_UPMEM_ROMPATH:
			cellDbgFontPuts(0.09f, 0.80f, Emulator_GetFontSize(), WHITE, "To enable S&KUPMEM lock-on functionality, you need to have the file\nsk2chip.bin somewhere on your HDD/USB device. Select the file here.");
			break;
		case SETTING_GENESIS_BIOS_ROMPATH:
			cellDbgFontPuts(0.09f, 0.80f, Emulator_GetFontSize(), WHITE, "For increased compatibility, you need to load with the Genesis BIOS file.\nSelect the file (bios.bin) here.");
			break;
	}
}

void do_pathChoice()
{
	if (tmpBrowser == NULL)
	{
		tmpBrowser = new FileBrowser("/\0");
	}
	string path;

	if (CellInput->UpdateDevice(0) == CELL_PAD_OK)
	{
		UpdateBrowser(tmpBrowser);
		if (CellInput->WasButtonPressed(0,CTRL_SQUARE))
		{
			if(tmpBrowser->IsCurrentADirectory())
			{
				path = tmpBrowser->GetCurrentDirectoryInfo().dir + "/" + tmpBrowser->GetCurrentEntry()->d_name + "/";
				switch(currently_selected_path_setting)
				{
					case SETTING_PATH_SAVESTATES_DIRECTORY:
						Settings.PS3PathSaveStates = path;
						break;
					case SETTING_PATH_SRAM_DIRECTORY:
						Settings.PS3PathSRAM = path;
						break;
					case SETTING_PATH_DEFAULT_ROM_DIRECTORY:
						Settings.PS3PathROMDirectory = path;
						break;
				}
				menuStack.pop();
			}
		}
		if (CellInput->WasButtonHeld(0, CTRL_TRIANGLE))
		{
			path = USRDIR;
			switch(currently_selected_path_setting)
			{
				case SETTING_PATH_SAVESTATES_DIRECTORY:
					Settings.PS3PathSaveStates = path;
					break;
				case SETTING_PATH_SRAM_DIRECTORY:
					Settings.PS3PathSRAM = path;
					break;
				case SETTING_PATH_DEFAULT_ROM_DIRECTORY:
					Settings.PS3PathROMDirectory = path;
					break;
			}
			menuStack.pop();
		}
		if (CellInput->WasButtonPressed(0,CTRL_CROSS))
		{
			if(tmpBrowser->IsCurrentADirectory())
			{
				tmpBrowser->PushDirectory(tmpBrowser->GetCurrentDirectoryInfo().dir + "/" + tmpBrowser->GetCurrentEntry()->d_name, CELL_FS_TYPE_REGULAR | CELL_FS_TYPE_DIRECTORY, "empty");
			}
		}
	}
			
	cellDbgFontPrintf(0.09f, 0.09f, Emulator_GetFontSize(), YELLOW, "PATH: %s", tmpBrowser->GetCurrentDirectoryInfo().dir.c_str());
	cellDbgFontPuts	(0.09f,	0.05f,	Emulator_GetFontSize(),	RED,	"DIRECTORY SELECTION");
	cellDbgFontPuts(0.09f, 0.93f, Emulator_GetFontSize(), YELLOW,"X - Enter directory             /\\ - return to settings");
	cellDbgFontPrintf(0.09f, 0.89f, 0.86f, LIGHTBLUE, "%s",
	"INFO - Browse to a directory and assign it as the path by pressing SQUARE button.");
	Graphics->FlushDbgFont();
			
	RenderBrowser(tmpBrowser);
}

char * Menu_PrintMappedButton(int mappedbutton)
{
	switch(mappedbutton)
	{
		case INPUT_A:
			return "Button A";
			break;
		case INPUT_B:
			return "Button B";
			break;
		case INPUT_C:
			return "Button C";
			break;
		case INPUT_X:
			return "Button X";
			break;
		case INPUT_Y:
			return "Button Y";
			break;
		case INPUT_Z:
			return "Button Z";
			break;
		case INPUT_START:
			return "Button Start";
			break;
		case INPUT_MODE:
			return "Button Mode";
			break;
		case INPUT_LEFT:
			return "D-Pad Left";
			break;
		case INPUT_RIGHT:
			return "D-Pad Right";
			break;
		case INPUT_UP:
			return "D-Pad Up";
			break;
		case INPUT_DOWN:
			return "D-Pad Down";
			break;
		case INPUT_NONE:
			return "None";
			break;
		case INPUT_QUIT:
			return "Exit to menu";
			break;
		case INPUT_SAVESTATE:
			return "Save State";
			break;
		case INPUT_LOADSTATE:
			return "Load State";
			break;
		case INPUT_SOFTRESET:
			return "Software Reset";
			break;
		case INPUT_HARDRESET:
			return "Reset";
			break;
		default:
			return "Unknown";
			break;

	}
}

//bool next: true is next, false is previous
int Menu_GetAdjacentButtonmap(int buttonmap, bool next)
{
	switch(buttonmap)
	{
		case INPUT_UP:
			return next ? INPUT_DOWN : INPUT_NONE;
			break;
		case INPUT_DOWN:
			return next ? INPUT_LEFT : INPUT_UP;
			break;
		case INPUT_LEFT:
			return next ? INPUT_RIGHT : INPUT_DOWN;
			break;
		case INPUT_RIGHT:
			return next ? INPUT_A : INPUT_LEFT;
			break;
		case INPUT_A:
			return next ? INPUT_B : INPUT_RIGHT;
			break;
		case INPUT_B:
			return next ? INPUT_C : INPUT_A;
			break;
		case INPUT_C:
			return next ? INPUT_X : INPUT_B;
			break;
		case INPUT_X:
			return next ? INPUT_Y : INPUT_C;
			break;
		case INPUT_Y:
			return next ? INPUT_Z : INPUT_X;
			break;
		case INPUT_Z:
			return next ? INPUT_START : INPUT_Y;
			break;
		case INPUT_START:
			return next ? INPUT_MODE : INPUT_Z;
			break;
		case INPUT_MODE:
			return next ? INPUT_HARDRESET : INPUT_START;
			break;
		case INPUT_HARDRESET:
			return next ? INPUT_SOFTRESET : INPUT_MODE;
			break;
		case INPUT_SOFTRESET:
			return next ? INPUT_SAVESTATE : INPUT_HARDRESET;
			break;
		case INPUT_SAVESTATE:
			return next ? INPUT_LOADSTATE : INPUT_SOFTRESET;
			break;
		case INPUT_LOADSTATE:
			return next ? INPUT_QUIT : INPUT_SAVESTATE;
			break;
		case INPUT_QUIT:
			return next ? INPUT_NONE : INPUT_LOADSTATE;
			break;
		case INPUT_NONE:
			return next ? INPUT_UP : INPUT_QUIT;
			break;
		default:
			return INPUT_NONE;
			break;
	}
}

void do_controls_settings()
{
	if(CellInput->UpdateDevice(0) == CELL_OK)
	{
			// back to ROM menu if CIRCLE is pressed
			if (CellInput->WasButtonPressed(0, CTRL_L1) | CellInput->WasButtonPressed(0, CTRL_CIRCLE))
			{
				menuStack.pop();
				return;
			}

			if (CellInput->WasButtonHeld(0, CTRL_DOWN) | CellInput->IsAnalogPressedDown(0, CTRL_LSTICK))	// down to next setting
			{
				currently_selected_controller_setting++;
				if (currently_selected_controller_setting >= MAX_NO_OF_CONTROLS_SETTINGS)
				{
					currently_selected_controller_setting = FIRST_CONTROLS_SETTING;
				}
				if(CellInput->IsButtonPressed(0,CTRL_DOWN) | CellInput->IsAnalogPressedDown(0, CTRL_LSTICK))
				{
					sys_timer_usleep(SETTINGS_DELAY);
				}
			}

			if (CellInput->IsButtonPressed(0,CTRL_L2) && CellInput->IsButtonPressed(0,CTRL_R2))
			{
				// if a rom is loaded then resume it
				if (Emulator_IsROMLoaded())
				{
					MenuStop();
					Emulator_StartROMRunning();
				}
				return;
			}

			if (CellInput->WasButtonHeld(0, CTRL_UP) | CellInput->IsAnalogPressedUp(0, CTRL_LSTICK))	// up to previous setting
			{
					currently_selected_controller_setting--;
					if (currently_selected_controller_setting < FIRST_CONTROLS_SETTING)
					{
						currently_selected_controller_setting = MAX_NO_OF_CONTROLS_SETTINGS-1;
					}
					if(CellInput->IsButtonPressed(0,CTRL_UP) | CellInput->IsAnalogPressedUp(0, CTRL_LSTICK))
					{
						sys_timer_usleep(SETTINGS_DELAY);
					}
			}
					switch(currently_selected_controller_setting)
					{
						case SETTING_CONTROLS_DPAD_UP:
						if(CellInput->WasButtonHeld(0, CTRL_LEFT) | CellInput->IsAnalogPressedLeft(0, CTRL_LSTICK))
						{
							Settings.DPad_Up = Menu_GetAdjacentButtonmap(Settings.DPad_Up, false);
							if(CellInput->IsButtonPressed(0,CTRL_LEFT) | CellInput->IsAnalogPressedLeft(0,CTRL_LSTICK))
							{
								sys_timer_usleep(SETTINGS_DELAY);
							}
						}
						if(CellInput->WasButtonHeld(0, CTRL_RIGHT) | CellInput->IsAnalogPressedRight(0, CTRL_LSTICK) | CellInput->WasButtonPressed(0, CTRL_CROSS))
						{
							Settings.DPad_Up = Menu_GetAdjacentButtonmap(Settings.DPad_Up, true);
							if(CellInput->IsButtonPressed(0,CTRL_RIGHT) | CellInput->IsAnalogPressedRight(0,CTRL_LSTICK))
							{
								sys_timer_usleep(SETTINGS_DELAY);
							}
						}
						if(CellInput->IsButtonPressed(0, CTRL_START))
						{
							Settings.DPad_Up = INPUT_UP;
						}
							break;
						case SETTING_CONTROLS_DPAD_DOWN:
						if(CellInput->WasButtonHeld(0, CTRL_LEFT) | CellInput->IsAnalogPressedLeft(0, CTRL_LSTICK))
						{
							Settings.DPad_Down = Menu_GetAdjacentButtonmap(Settings.DPad_Down, false);
							if(CellInput->IsButtonPressed(0,CTRL_LEFT) | CellInput->IsAnalogPressedLeft(0,CTRL_LSTICK))
							{
								sys_timer_usleep(SETTINGS_DELAY);
							}
						}
						if(CellInput->WasButtonHeld(0, CTRL_RIGHT) | CellInput->IsAnalogPressedRight(0, CTRL_LSTICK) | CellInput->WasButtonPressed(0, CTRL_CROSS))
						{
							Settings.DPad_Down = Menu_GetAdjacentButtonmap(Settings.DPad_Down, true);
							if(CellInput->IsButtonPressed(0,CTRL_RIGHT) | CellInput->IsAnalogPressedRight(0,CTRL_LSTICK))
							{
								sys_timer_usleep(SETTINGS_DELAY);
							}
						}
						if(CellInput->IsButtonPressed(0, CTRL_START))
						{
							Settings.DPad_Down = INPUT_DOWN;
						}
							break;
						case SETTING_CONTROLS_DPAD_LEFT:
						if(CellInput->WasButtonHeld(0, CTRL_LEFT) | CellInput->IsAnalogPressedLeft(0, CTRL_LSTICK))
						{
							Settings.DPad_Left = Menu_GetAdjacentButtonmap(Settings.DPad_Left, false);
							if(CellInput->IsButtonPressed(0,CTRL_LEFT) | CellInput->IsAnalogPressedLeft(0,CTRL_LSTICK))
							{
								sys_timer_usleep(SETTINGS_DELAY);
							}
						}
						if(CellInput->WasButtonHeld(0, CTRL_RIGHT) | CellInput->IsAnalogPressedRight(0, CTRL_LSTICK) | CellInput->WasButtonPressed(0, CTRL_CROSS))
						{
							Settings.DPad_Left = Menu_GetAdjacentButtonmap(Settings.DPad_Left, true);
							if(CellInput->IsButtonPressed(0,CTRL_RIGHT) | CellInput->IsAnalogPressedRight(0,CTRL_LSTICK))
							{
								sys_timer_usleep(SETTINGS_DELAY);
							}
						}
						if(CellInput->IsButtonPressed(0, CTRL_START))
						{
							Settings.DPad_Left = INPUT_LEFT;
						}
							break;
						case SETTING_CONTROLS_DPAD_RIGHT:
						if(CellInput->WasButtonHeld(0, CTRL_LEFT) | CellInput->IsAnalogPressedLeft(0, CTRL_LSTICK))
						{
							Settings.DPad_Right = Menu_GetAdjacentButtonmap(Settings.DPad_Right, false);
							if(CellInput->IsButtonPressed(0,CTRL_LEFT) | CellInput->IsAnalogPressedLeft(0,CTRL_LSTICK))
							{
								sys_timer_usleep(SETTINGS_DELAY);
							}
						}
						if(CellInput->WasButtonHeld(0, CTRL_RIGHT) | CellInput->IsAnalogPressedRight(0, CTRL_LSTICK) | CellInput->WasButtonPressed(0, CTRL_CROSS))
						{
							Settings.DPad_Right = Menu_GetAdjacentButtonmap(Settings.DPad_Right, true);
							if(CellInput->IsButtonPressed(0,CTRL_RIGHT) | CellInput->IsAnalogPressedRight(0,CTRL_LSTICK))
							{
								sys_timer_usleep(SETTINGS_DELAY);
							}
						}
						if(CellInput->IsButtonPressed(0, CTRL_START))
						{
							Settings.DPad_Right = INPUT_RIGHT;
						}
							break;
						case SETTING_CONTROLS_BUTTON_CIRCLE:
						if(CellInput->WasButtonHeld(0, CTRL_LEFT) | CellInput->IsAnalogPressedLeft(0, CTRL_LSTICK))
						{
							Settings.ButtonCircle = Menu_GetAdjacentButtonmap(Settings.ButtonCircle, false);
							if(CellInput->IsButtonPressed(0,CTRL_LEFT) | CellInput->IsAnalogPressedLeft(0,CTRL_LSTICK))
							{
								sys_timer_usleep(SETTINGS_DELAY);
							}
						}
						if(CellInput->WasButtonHeld(0, CTRL_RIGHT) | CellInput->IsAnalogPressedRight(0, CTRL_LSTICK) | CellInput->WasButtonPressed(0, CTRL_CROSS))
						{
							Settings.ButtonCircle = Menu_GetAdjacentButtonmap(Settings.ButtonCircle, true);
							if(CellInput->IsButtonPressed(0,CTRL_RIGHT) | CellInput->IsAnalogPressedRight(0,CTRL_LSTICK))
							{
								sys_timer_usleep(SETTINGS_DELAY);
							}
						}
						if(CellInput->IsButtonPressed(0, CTRL_START))
						{
							Settings.ButtonCircle = INPUT_C;
						}
							break;
						case SETTING_CONTROLS_BUTTON_CROSS:
						if(CellInput->WasButtonHeld(0, CTRL_LEFT) | CellInput->IsAnalogPressedLeft(0, CTRL_LSTICK))
						{
							Settings.ButtonCross = Menu_GetAdjacentButtonmap(Settings.ButtonCross, false);
							if(CellInput->IsButtonPressed(0,CTRL_LEFT) | CellInput->IsAnalogPressedLeft(0,CTRL_LSTICK))
							{
								sys_timer_usleep(SETTINGS_DELAY);
							}
						}
						if(CellInput->WasButtonHeld(0, CTRL_RIGHT) | CellInput->IsAnalogPressedRight(0, CTRL_LSTICK) | CellInput->WasButtonPressed(0, CTRL_CROSS))
						{
							Settings.ButtonCross = Menu_GetAdjacentButtonmap(Settings.ButtonCross, true);
							if(CellInput->IsButtonPressed(0,CTRL_RIGHT) | CellInput->IsAnalogPressedRight(0,CTRL_LSTICK))
							{
								sys_timer_usleep(SETTINGS_DELAY);
							}
						}
						if(CellInput->IsButtonPressed(0, CTRL_START))
						{
							Settings.ButtonCross = INPUT_B;
						}
							break;
						case SETTING_CONTROLS_BUTTON_TRIANGLE:
						if(CellInput->WasButtonHeld(0, CTRL_LEFT) | CellInput->IsAnalogPressedLeft(0, CTRL_LSTICK))
						{
							Settings.ButtonTriangle = Menu_GetAdjacentButtonmap(Settings.ButtonTriangle, false);
							if(CellInput->IsButtonPressed(0,CTRL_LEFT) | CellInput->IsAnalogPressedLeft(0,CTRL_LSTICK))
							{
								sys_timer_usleep(SETTINGS_DELAY);
							}
						}
						if(CellInput->WasButtonHeld(0, CTRL_RIGHT) | CellInput->IsAnalogPressedRight(0, CTRL_LSTICK) | CellInput->WasButtonPressed(0, CTRL_CROSS))
						{
							Settings.ButtonTriangle = Menu_GetAdjacentButtonmap(Settings.ButtonTriangle, true);
							if(CellInput->IsButtonPressed(0,CTRL_RIGHT) | CellInput->IsAnalogPressedRight(0,CTRL_LSTICK))
							{
								sys_timer_usleep(SETTINGS_DELAY);
							}
						}
						if(CellInput->IsButtonPressed(0, CTRL_START))
						{
							Settings.ButtonTriangle = INPUT_X;
						}
							break;
						case SETTING_CONTROLS_BUTTON_SQUARE:
						if(CellInput->WasButtonHeld(0, CTRL_LEFT) | CellInput->IsAnalogPressedLeft(0, CTRL_LSTICK))
						{
							Settings.ButtonSquare = Menu_GetAdjacentButtonmap(Settings.ButtonSquare, false);
							if(CellInput->IsButtonPressed(0,CTRL_LEFT) | CellInput->IsAnalogPressedLeft(0,CTRL_LSTICK))
							{
								sys_timer_usleep(SETTINGS_DELAY);
							}
						}
						if(CellInput->WasButtonHeld(0, CTRL_RIGHT) | CellInput->IsAnalogPressedRight(0, CTRL_LSTICK) | CellInput->WasButtonPressed(0, CTRL_CROSS))
						{
							Settings.ButtonSquare = Menu_GetAdjacentButtonmap(Settings.ButtonSquare, true);
							if(CellInput->IsButtonPressed(0,CTRL_RIGHT) | CellInput->IsAnalogPressedRight(0,CTRL_LSTICK))
							{
								sys_timer_usleep(SETTINGS_DELAY);
							}
						}
						if(CellInput->IsButtonPressed(0, CTRL_START))
						{
							Settings.ButtonSquare = INPUT_A;
						}
							break;
						case SETTING_CONTROLS_BUTTON_SELECT:
						if(CellInput->WasButtonHeld(0, CTRL_LEFT) | CellInput->IsAnalogPressedLeft(0, CTRL_LSTICK))
						{
							Settings.ButtonSelect = Menu_GetAdjacentButtonmap(Settings.ButtonSelect, false);
							if(CellInput->IsButtonPressed(0,CTRL_LEFT) | CellInput->IsAnalogPressedLeft(0,CTRL_LSTICK))
							{
								sys_timer_usleep(SETTINGS_DELAY);
							}
						}
						if(CellInput->WasButtonHeld(0, CTRL_RIGHT) | CellInput->IsAnalogPressedRight(0, CTRL_LSTICK) | CellInput->WasButtonPressed(0, CTRL_CROSS))
						{
							Settings.ButtonSelect = Menu_GetAdjacentButtonmap(Settings.ButtonSelect, true);
							if(CellInput->IsButtonPressed(0,CTRL_RIGHT) | CellInput->IsAnalogPressedRight(0,CTRL_LSTICK))
							{
								sys_timer_usleep(SETTINGS_DELAY);
							}
						}
						if(CellInput->IsButtonPressed(0, CTRL_START))
						{
							Settings.ButtonSelect = INPUT_MODE;
						}
							break;
						case SETTING_CONTROLS_BUTTON_START:
						if(CellInput->WasButtonHeld(0, CTRL_LEFT) | CellInput->IsAnalogPressedLeft(0, CTRL_LSTICK))
						{
							Settings.ButtonStart = Menu_GetAdjacentButtonmap(Settings.ButtonStart, false);
							if(CellInput->IsButtonPressed(0,CTRL_LEFT) | CellInput->IsAnalogPressedLeft(0,CTRL_LSTICK))
							{
								sys_timer_usleep(SETTINGS_DELAY);
							}
						}
						if(CellInput->WasButtonHeld(0, CTRL_RIGHT) | CellInput->IsAnalogPressedRight(0, CTRL_LSTICK) | CellInput->WasButtonPressed(0, CTRL_CROSS))
						{
							Settings.ButtonStart = Menu_GetAdjacentButtonmap(Settings.ButtonStart, true);
							if(CellInput->IsButtonPressed(0,CTRL_RIGHT) | CellInput->IsAnalogPressedRight(0,CTRL_LSTICK))
							{
								sys_timer_usleep(SETTINGS_DELAY);
							}
						}
						if(CellInput->IsButtonPressed(0, CTRL_START))
						{
							Settings.ButtonStart = INPUT_START;
						}
							break;
						case SETTING_CONTROLS_BUTTON_L1:
						if(CellInput->WasButtonHeld(0, CTRL_LEFT) | CellInput->IsAnalogPressedLeft(0, CTRL_LSTICK))
						{
							Settings.ButtonL1 = Menu_GetAdjacentButtonmap(Settings.ButtonL1, false);
							if(CellInput->IsButtonPressed(0,CTRL_LEFT) | CellInput->IsAnalogPressedLeft(0,CTRL_LSTICK))
							{
								sys_timer_usleep(SETTINGS_DELAY);
							}
						}
						if(CellInput->WasButtonHeld(0, CTRL_RIGHT) | CellInput->IsAnalogPressedRight(0, CTRL_LSTICK) | CellInput->WasButtonPressed(0, CTRL_CROSS))
						{
							Settings.ButtonL1 = Menu_GetAdjacentButtonmap(Settings.ButtonL1, true);
							if(CellInput->IsButtonPressed(0,CTRL_RIGHT) | CellInput->IsAnalogPressedRight(0,CTRL_LSTICK))
							{
								sys_timer_usleep(SETTINGS_DELAY);
							}
						}
						if(CellInput->IsButtonPressed(0, CTRL_START))
						{
							Settings.ButtonL1 = INPUT_Y;
						}
							break;
						case SETTING_CONTROLS_BUTTON_L2:
						if(CellInput->WasButtonHeld(0, CTRL_LEFT) | CellInput->IsAnalogPressedLeft(0, CTRL_LSTICK))
						{
							Settings.ButtonL2 = Menu_GetAdjacentButtonmap(Settings.ButtonL2, false);
							if(CellInput->IsButtonPressed(0,CTRL_LEFT) | CellInput->IsAnalogPressedLeft(0,CTRL_LSTICK))
							{
								sys_timer_usleep(SETTINGS_DELAY);
							}
						}
						if(CellInput->WasButtonHeld(0, CTRL_RIGHT) | CellInput->IsAnalogPressedRight(0, CTRL_LSTICK) | CellInput->WasButtonPressed(0, CTRL_CROSS))
						{
							Settings.ButtonL2 = Menu_GetAdjacentButtonmap(Settings.ButtonL2, true);
							if(CellInput->IsButtonPressed(0,CTRL_RIGHT) | CellInput->IsAnalogPressedRight(0,CTRL_LSTICK))
							{
								sys_timer_usleep(SETTINGS_DELAY);
							}
						}
						if(CellInput->IsButtonPressed(0, CTRL_START))
						{
							Settings.ButtonL2 = INPUT_NONE;
						}
							break;
						case SETTING_CONTROLS_BUTTON_R2:
						if(CellInput->WasButtonHeld(0, CTRL_LEFT) | CellInput->IsAnalogPressedLeft(0, CTRL_LSTICK))
						{
							Settings.ButtonR2 = Menu_GetAdjacentButtonmap(Settings.ButtonR2, false);
							if(CellInput->IsButtonPressed(0,CTRL_LEFT) | CellInput->IsAnalogPressedLeft(0,CTRL_LSTICK))
							{
								sys_timer_usleep(SETTINGS_DELAY);
							}
						}
						if(CellInput->WasButtonHeld(0, CTRL_RIGHT) | CellInput->IsAnalogPressedRight(0, CTRL_LSTICK) | CellInput->WasButtonPressed(0, CTRL_CROSS))
						{
							Settings.ButtonR2 = Menu_GetAdjacentButtonmap(Settings.ButtonR2, true);
							if(CellInput->IsButtonPressed(0,CTRL_RIGHT) | CellInput->IsAnalogPressedRight(0,CTRL_LSTICK))
							{
								sys_timer_usleep(SETTINGS_DELAY);
							}
						}
						if(CellInput->IsButtonPressed(0, CTRL_START))
						{
							Settings.ButtonR2 = INPUT_NONE;
						}
							break;
						case SETTING_CONTROLS_BUTTON_L3:
						if(CellInput->WasButtonHeld(0, CTRL_LEFT) | CellInput->IsAnalogPressedLeft(0, CTRL_LSTICK))
						{
							Settings.ButtonL3 = Menu_GetAdjacentButtonmap(Settings.ButtonL3, false);
							if(CellInput->IsButtonPressed(0,CTRL_LEFT) | CellInput->IsAnalogPressedLeft(0,CTRL_LSTICK))
							{
								sys_timer_usleep(SETTINGS_DELAY);
							}
						}
						if(CellInput->WasButtonHeld(0, CTRL_RIGHT) | CellInput->IsAnalogPressedRight(0, CTRL_LSTICK) | CellInput->WasButtonPressed(0, CTRL_CROSS))
						{
							Settings.ButtonL3 = Menu_GetAdjacentButtonmap(Settings.ButtonL3, true);
							if(CellInput->IsButtonPressed(0,CTRL_RIGHT) | CellInput->IsAnalogPressedRight(0,CTRL_LSTICK))
							{
								sys_timer_usleep(SETTINGS_DELAY);
							}
						}
						if(CellInput->IsButtonPressed(0, CTRL_START))
						{
							Settings.ButtonL3 = INPUT_NONE;
						}
							break;
						case SETTING_CONTROLS_BUTTON_R3:
						if(CellInput->WasButtonHeld(0, CTRL_LEFT) | CellInput->IsAnalogPressedLeft(0, CTRL_LSTICK))
						{
							Settings.ButtonR3 = Menu_GetAdjacentButtonmap(Settings.ButtonR3, false);
							if(CellInput->IsButtonPressed(0,CTRL_LEFT) | CellInput->IsAnalogPressedLeft(0,CTRL_LSTICK))
							{
								sys_timer_usleep(SETTINGS_DELAY);
							}
						}
						if(CellInput->WasButtonHeld(0, CTRL_RIGHT) | CellInput->IsAnalogPressedRight(0, CTRL_LSTICK) | CellInput->WasButtonPressed(0, CTRL_CROSS))
						{
							Settings.ButtonR3 = Menu_GetAdjacentButtonmap(Settings.ButtonR3, true);
							if(CellInput->IsButtonPressed(0,CTRL_RIGHT) | CellInput->IsAnalogPressedRight(0,CTRL_LSTICK))
							{
								sys_timer_usleep(SETTINGS_DELAY);
							}
						}
						if(CellInput->IsButtonPressed(0, CTRL_START))
						{
							Settings.ButtonR3 = INPUT_NONE;
						}
							break;
						case SETTING_CONTROLS_BUTTON_R1:
						if(CellInput->WasButtonHeld(0, CTRL_LEFT) | CellInput->IsAnalogPressedLeft(0, CTRL_LSTICK))
						{
							Settings.ButtonR1 = Menu_GetAdjacentButtonmap(Settings.ButtonR1, false);
							if(CellInput->IsButtonPressed(0,CTRL_LEFT) | CellInput->IsAnalogPressedLeft(0,CTRL_LSTICK))
							{
								sys_timer_usleep(SETTINGS_DELAY);
							}
						}
						if(CellInput->WasButtonHeld(0, CTRL_RIGHT) | CellInput->IsAnalogPressedRight(0, CTRL_LSTICK) | CellInput->WasButtonPressed(0, CTRL_CROSS))
						{
							Settings.ButtonR1 = Menu_GetAdjacentButtonmap(Settings.ButtonR1, true);
							if(CellInput->IsButtonPressed(0,CTRL_RIGHT) | CellInput->IsAnalogPressedRight(0,CTRL_LSTICK))
							{
								sys_timer_usleep(SETTINGS_DELAY);
							}
						}
						if(CellInput->IsButtonPressed(0, CTRL_START))
						{
							Settings.ButtonR1 = INPUT_Z;
						}
							break;
						case SETTING_CONTROLS_BUTTON_L2_BUTTON_L3:
						if(CellInput->WasButtonHeld(0, CTRL_LEFT) | CellInput->IsAnalogPressedLeft(0, CTRL_LSTICK))
						{
							Settings.ButtonL2_ButtonL3 = Menu_GetAdjacentButtonmap(Settings.ButtonL2_ButtonL3, false);
							if(CellInput->IsButtonPressed(0,CTRL_LEFT) | CellInput->IsAnalogPressedLeft(0,CTRL_LSTICK))
							{
								sys_timer_usleep(SETTINGS_DELAY);
							}
						}
						if(CellInput->WasButtonHeld(0, CTRL_RIGHT) | CellInput->IsAnalogPressedRight(0, CTRL_LSTICK) | CellInput->WasButtonPressed(0, CTRL_CROSS))
						{
							Settings.ButtonL2_ButtonL3 = Menu_GetAdjacentButtonmap(Settings.ButtonL2_ButtonL3, true);
							if(CellInput->IsButtonPressed(0,CTRL_RIGHT) | CellInput->IsAnalogPressedRight(0,CTRL_LSTICK))
							{
								sys_timer_usleep(SETTINGS_DELAY);
							}
						}
						if(CellInput->IsButtonPressed(0, CTRL_START))
						{
							Settings.ButtonL2_ButtonL3 = INPUT_NONE;
						}
							break;
						case SETTING_CONTROLS_BUTTON_L2_BUTTON_R2:
						if(CellInput->WasButtonHeld(0, CTRL_LEFT) | CellInput->IsAnalogPressedLeft(0, CTRL_LSTICK))
						{
							Settings.ButtonL2_ButtonR2 = Menu_GetAdjacentButtonmap(Settings.ButtonL2_ButtonR2, false);
							if(CellInput->IsButtonPressed(0,CTRL_LEFT) | CellInput->IsAnalogPressedLeft(0,CTRL_LSTICK))
							{
								sys_timer_usleep(SETTINGS_DELAY);
							}
						}
						if(CellInput->WasButtonHeld(0, CTRL_RIGHT) | CellInput->IsAnalogPressedRight(0, CTRL_LSTICK) | CellInput->WasButtonPressed(0, CTRL_CROSS))
						{
							Settings.ButtonL2_ButtonR2 = Menu_GetAdjacentButtonmap(Settings.ButtonL2_ButtonR2, true);
							if(CellInput->IsButtonPressed(0,CTRL_RIGHT) | CellInput->IsAnalogPressedRight(0,CTRL_LSTICK))
							{
								sys_timer_usleep(SETTINGS_DELAY);
							}
						}
						if(CellInput->IsButtonPressed(0, CTRL_START))
						{
							Settings.ButtonL2_ButtonR2 = INPUT_NONE;
						}
						break;
						case SETTING_CONTROLS_BUTTON_L2_BUTTON_R3:
						if(CellInput->WasButtonHeld(0, CTRL_LEFT) | CellInput->IsAnalogPressedLeft(0, CTRL_LSTICK))
						{
							Settings.ButtonL2_ButtonR3 = Menu_GetAdjacentButtonmap(Settings.ButtonL2_ButtonR3, false);
							if(CellInput->IsButtonPressed(0,CTRL_LEFT) | CellInput->IsAnalogPressedLeft(0,CTRL_LSTICK))
							{
								sys_timer_usleep(SETTINGS_DELAY);
							}
						}
						if(CellInput->WasButtonHeld(0, CTRL_RIGHT) | CellInput->IsAnalogPressedRight(0, CTRL_LSTICK) | CellInput->WasButtonPressed(0, CTRL_CROSS))
						{
							Settings.ButtonL2_ButtonR3 = Menu_GetAdjacentButtonmap(Settings.ButtonL2_ButtonR3, true);
							if(CellInput->IsButtonPressed(0,CTRL_RIGHT) | CellInput->IsAnalogPressedRight(0,CTRL_LSTICK))
							{
								sys_timer_usleep(SETTINGS_DELAY);
							}
						}
						if(CellInput->IsButtonPressed(0, CTRL_START))
						{
							Settings.ButtonL2_ButtonR3 = INPUT_LOADSTATE;
						}
						break;
						case SETTING_CONTROLS_BUTTON_R2_BUTTON_R3:
						if(CellInput->WasButtonHeld(0, CTRL_LEFT) | CellInput->IsAnalogPressedLeft(0, CTRL_LSTICK))
						{
							Settings.ButtonR2_ButtonR3 = Menu_GetAdjacentButtonmap(Settings.ButtonR2_ButtonR3, false);
							if(CellInput->IsButtonPressed(0,CTRL_LEFT) | CellInput->IsAnalogPressedLeft(0,CTRL_LSTICK))
							{
								sys_timer_usleep(SETTINGS_DELAY);
							}
						}
						if(CellInput->WasButtonHeld(0, CTRL_RIGHT) | CellInput->IsAnalogPressedRight(0, CTRL_LSTICK) | CellInput->WasButtonPressed(0, CTRL_CROSS))
						{
							Settings.ButtonR2_ButtonR3 = Menu_GetAdjacentButtonmap(Settings.ButtonR2_ButtonR3, true);
							if(CellInput->IsButtonPressed(0,CTRL_RIGHT) | CellInput->IsAnalogPressedRight(0,CTRL_LSTICK))
							{
								sys_timer_usleep(SETTINGS_DELAY);
							}
						}
						if(CellInput->IsButtonPressed(0, CTRL_START))
						{
							Settings.ButtonR2_ButtonR3 = INPUT_SAVESTATE;
						}
							break;
						case SETTING_CONTROLS_ANALOG_R_UP:
						if(CellInput->WasButtonHeld(0, CTRL_LEFT) | CellInput->IsAnalogPressedLeft(0, CTRL_LSTICK))
						{
							Settings.AnalogR_Up = Menu_GetAdjacentButtonmap(Settings.AnalogR_Up, false);
							if(CellInput->IsButtonPressed(0,CTRL_LEFT) | CellInput->IsAnalogPressedLeft(0,CTRL_LSTICK))
							{
								sys_timer_usleep(SETTINGS_DELAY);
							}
						}
						if(CellInput->WasButtonHeld(0, CTRL_RIGHT) | CellInput->IsAnalogPressedRight(0, CTRL_LSTICK) | CellInput->WasButtonPressed(0, CTRL_CROSS))
						{
							Settings.AnalogR_Up = Menu_GetAdjacentButtonmap(Settings.AnalogR_Up, true);
							if(CellInput->IsButtonPressed(0,CTRL_RIGHT) | CellInput->IsAnalogPressedRight(0,CTRL_LSTICK))
							{
								sys_timer_usleep(SETTINGS_DELAY);
							}
						}
						if(CellInput->IsButtonPressed(0, CTRL_START))
						{
							Settings.AnalogR_Up = INPUT_NONE;
						}
						if(CellInput->WasButtonPressed(0, CTRL_SELECT))
						{
							Settings.AnalogR_Up_Type = !Settings.AnalogR_Up_Type;
						}
						break;
						case SETTING_CONTROLS_ANALOG_R_DOWN:
						if(CellInput->WasButtonHeld(0, CTRL_LEFT) | CellInput->IsAnalogPressedLeft(0, CTRL_LSTICK))
						{
							Settings.AnalogR_Down = Menu_GetAdjacentButtonmap(Settings.AnalogR_Down, false);
							if(CellInput->IsButtonPressed(0,CTRL_LEFT) | CellInput->IsAnalogPressedLeft(0,CTRL_LSTICK))
							{
								sys_timer_usleep(SETTINGS_DELAY);
							}
						}
						if(CellInput->WasButtonHeld(0, CTRL_RIGHT) | CellInput->IsAnalogPressedRight(0, CTRL_LSTICK) | CellInput->WasButtonPressed(0, CTRL_CROSS))
						{
							Settings.AnalogR_Down = Menu_GetAdjacentButtonmap(Settings.AnalogR_Down, true);
							if(CellInput->IsButtonPressed(0,CTRL_RIGHT) | CellInput->IsAnalogPressedRight(0,CTRL_LSTICK))
							{
								sys_timer_usleep(SETTINGS_DELAY);
							}
						}
						if(CellInput->IsButtonPressed(0, CTRL_START))
						{
							Settings.AnalogR_Down = INPUT_NONE;
						}
						if(CellInput->WasButtonPressed(0, CTRL_SELECT))
						{
							Settings.AnalogR_Down_Type = !Settings.AnalogR_Down_Type;
						}
						break;
						case SETTING_CONTROLS_ANALOG_R_LEFT:
						if(CellInput->WasButtonHeld(0, CTRL_LEFT) | CellInput->IsAnalogPressedLeft(0, CTRL_LSTICK))
						{
							Settings.AnalogR_Left = Menu_GetAdjacentButtonmap(Settings.AnalogR_Left, false);
							if(CellInput->IsButtonPressed(0,CTRL_LEFT) | CellInput->IsAnalogPressedLeft(0,CTRL_LSTICK))
							{
								sys_timer_usleep(SETTINGS_DELAY);
							}
						}
						if(CellInput->WasButtonHeld(0, CTRL_RIGHT) | CellInput->IsAnalogPressedRight(0, CTRL_LSTICK) | CellInput->WasButtonPressed(0, CTRL_CROSS))
						{
							Settings.AnalogR_Left = Menu_GetAdjacentButtonmap(Settings.AnalogR_Left, true);
							if(CellInput->IsButtonPressed(0,CTRL_RIGHT) | CellInput->IsAnalogPressedRight(0,CTRL_LSTICK))
							{
								sys_timer_usleep(SETTINGS_DELAY);
							}
						}
						if(CellInput->IsButtonPressed(0, CTRL_START))
						{
							Settings.AnalogR_Left = INPUT_NONE;
						}
						if(CellInput->WasButtonPressed(0, CTRL_SELECT))
						{
							Settings.AnalogR_Left_Type = !Settings.AnalogR_Left_Type;
						}
						break;
						case SETTING_CONTROLS_ANALOG_R_RIGHT:
						if(CellInput->WasButtonHeld(0, CTRL_LEFT) | CellInput->IsAnalogPressedLeft(0, CTRL_LSTICK))
						{
							Settings.AnalogR_Right = Menu_GetAdjacentButtonmap(Settings.AnalogR_Right, false);
							if(CellInput->IsButtonPressed(0,CTRL_LEFT) | CellInput->IsAnalogPressedLeft(0,CTRL_LSTICK))
							{
								sys_timer_usleep(SETTINGS_DELAY);
							}
						}
						if(CellInput->WasButtonHeld(0, CTRL_RIGHT) | CellInput->IsAnalogPressedRight(0, CTRL_LSTICK) | CellInput->WasButtonPressed(0, CTRL_CROSS))
						{
							Settings.AnalogR_Right = Menu_GetAdjacentButtonmap(Settings.AnalogR_Right, true);
							if(CellInput->IsButtonPressed(0,CTRL_RIGHT) | CellInput->IsAnalogPressedRight(0,CTRL_LSTICK))
							{
								sys_timer_usleep(SETTINGS_DELAY);
							}
						}
						if(CellInput->IsButtonPressed(0, CTRL_START))
						{
							Settings.AnalogR_Right = INPUT_NONE;
						}
						if(CellInput->WasButtonPressed(0, CTRL_SELECT))
						{
							Settings.AnalogR_Right_Type = !Settings.AnalogR_Right_Type;
						}
						break;
						case SETTING_CONTROLS_BUTTON_L2_ANALOG_R_RIGHT:
						if(CellInput->WasButtonHeld(0, CTRL_LEFT) | CellInput->IsAnalogPressedLeft(0, CTRL_LSTICK))
						{
							Settings.ButtonL2_AnalogR_Right = Menu_GetAdjacentButtonmap(Settings.ButtonL2_AnalogR_Right, false);
							if(CellInput->IsButtonPressed(0,CTRL_LEFT) | CellInput->IsAnalogPressedLeft(0,CTRL_LSTICK))
							{
								sys_timer_usleep(SETTINGS_DELAY);
							}
						}
						if(CellInput->WasButtonHeld(0, CTRL_RIGHT) | CellInput->IsAnalogPressedRight(0, CTRL_LSTICK) | CellInput->WasButtonPressed(0, CTRL_CROSS))
						{
							Settings.ButtonL2_AnalogR_Right = Menu_GetAdjacentButtonmap(Settings.ButtonL2_AnalogR_Right, true);
							if(CellInput->IsButtonPressed(0,CTRL_RIGHT) | CellInput->IsAnalogPressedRight(0,CTRL_LSTICK))
							{
								sys_timer_usleep(SETTINGS_DELAY);
							}
						}
						if(CellInput->IsButtonPressed(0, CTRL_START))
						{
							Settings.ButtonL2_AnalogR_Right = INPUT_NONE;
						}
						break;
						case SETTING_CONTROLS_BUTTON_L2_ANALOG_R_LEFT:
						if(CellInput->WasButtonHeld(0, CTRL_LEFT) | CellInput->IsAnalogPressedLeft(0, CTRL_LSTICK))
						{
							Settings.ButtonL2_AnalogR_Left = Menu_GetAdjacentButtonmap(Settings.ButtonL2_AnalogR_Left, false);
							if(CellInput->IsButtonPressed(0,CTRL_LEFT) | CellInput->IsAnalogPressedLeft(0,CTRL_LSTICK))
							{
								sys_timer_usleep(SETTINGS_DELAY);
							}
						}
						if(CellInput->WasButtonHeld(0, CTRL_RIGHT) | CellInput->IsAnalogPressedRight(0, CTRL_LSTICK) | CellInput->WasButtonPressed(0, CTRL_CROSS))
						{
							Settings.ButtonL2_AnalogR_Left = Menu_GetAdjacentButtonmap(Settings.ButtonL2_AnalogR_Left, true);
							if(CellInput->IsButtonPressed(0,CTRL_RIGHT) | CellInput->IsAnalogPressedRight(0,CTRL_LSTICK))
							{
								sys_timer_usleep(SETTINGS_DELAY);
							}
						}
						if(CellInput->IsButtonPressed(0, CTRL_START))
						{
							Settings.ButtonL2_AnalogR_Left = INPUT_NONE;
						}
						break;
						case SETTING_CONTROLS_BUTTON_L2_ANALOG_R_UP:
						if(CellInput->WasButtonHeld(0, CTRL_LEFT) | CellInput->IsAnalogPressedLeft(0, CTRL_LSTICK))
						{
							Settings.ButtonL2_AnalogR_Up = Menu_GetAdjacentButtonmap(Settings.ButtonL2_AnalogR_Up, false);
							if(CellInput->IsButtonPressed(0,CTRL_LEFT) | CellInput->IsAnalogPressedLeft(0,CTRL_LSTICK))
							{
								sys_timer_usleep(SETTINGS_DELAY);
							}
						}
						if(CellInput->WasButtonHeld(0, CTRL_RIGHT) | CellInput->IsAnalogPressedRight(0, CTRL_LSTICK) | CellInput->WasButtonPressed(0, CTRL_CROSS))
						{
							Settings.ButtonL2_AnalogR_Up = Menu_GetAdjacentButtonmap(Settings.ButtonL2_AnalogR_Up, true);
							if(CellInput->IsButtonPressed(0,CTRL_RIGHT) | CellInput->IsAnalogPressedRight(0,CTRL_LSTICK))
							{
								sys_timer_usleep(SETTINGS_DELAY);
							}
						}
						if(CellInput->IsButtonPressed(0, CTRL_START))
						{
							Settings.ButtonL2_AnalogR_Up = INPUT_NONE;
						}
						break;
						case SETTING_CONTROLS_BUTTON_L2_ANALOG_R_DOWN:
						if(CellInput->WasButtonHeld(0, CTRL_LEFT) | CellInput->IsAnalogPressedLeft(0, CTRL_LSTICK))
						{
							Settings.ButtonL2_AnalogR_Down = Menu_GetAdjacentButtonmap(Settings.ButtonL2_AnalogR_Down, false);
							if(CellInput->IsButtonPressed(0,CTRL_LEFT) | CellInput->IsAnalogPressedLeft(0,CTRL_LSTICK))
							{
								sys_timer_usleep(SETTINGS_DELAY);
							}
						}
						if(CellInput->WasButtonHeld(0, CTRL_RIGHT) | CellInput->IsAnalogPressedRight(0, CTRL_LSTICK) | CellInput->WasButtonPressed(0, CTRL_CROSS))
						{
							Settings.ButtonL2_AnalogR_Down = Menu_GetAdjacentButtonmap(Settings.ButtonL2_AnalogR_Down, true);
							if(CellInput->IsButtonPressed(0,CTRL_RIGHT) | CellInput->IsAnalogPressedRight(0,CTRL_LSTICK))
							{
								sys_timer_usleep(SETTINGS_DELAY);
							}
						}
						if(CellInput->IsButtonPressed(0, CTRL_START))
						{
							Settings.ButtonL2_AnalogR_Down = INPUT_NONE;
						}
						break;
						case SETTING_CONTROLS_BUTTON_R2_ANALOG_R_RIGHT:
						if(CellInput->WasButtonHeld(0, CTRL_LEFT) | CellInput->IsAnalogPressedLeft(0, CTRL_LSTICK))
						{
							Settings.ButtonR2_AnalogR_Right = Menu_GetAdjacentButtonmap(Settings.ButtonR2_AnalogR_Right, false);
							if(CellInput->IsButtonPressed(0,CTRL_LEFT) | CellInput->IsAnalogPressedLeft(0,CTRL_LSTICK))
							{
								sys_timer_usleep(SETTINGS_DELAY);
							}
						}
						if(CellInput->WasButtonHeld(0, CTRL_RIGHT) | CellInput->IsAnalogPressedRight(0, CTRL_LSTICK) | CellInput->WasButtonPressed(0, CTRL_CROSS))
						{
							Settings.ButtonR2_AnalogR_Right = Menu_GetAdjacentButtonmap(Settings.ButtonR2_AnalogR_Right, true);
							if(CellInput->IsButtonPressed(0,CTRL_RIGHT) | CellInput->IsAnalogPressedRight(0,CTRL_LSTICK))
							{
								sys_timer_usleep(SETTINGS_DELAY);
							}
						}
						if(CellInput->IsButtonPressed(0, CTRL_START))
						{
							Settings.ButtonR2_AnalogR_Right = INPUT_NONE;
						}
						break;
						case SETTING_CONTROLS_BUTTON_R2_ANALOG_R_LEFT:
						if(CellInput->WasButtonHeld(0, CTRL_LEFT) | CellInput->IsAnalogPressedLeft(0, CTRL_LSTICK))
						{
							Settings.ButtonR2_AnalogR_Left = Menu_GetAdjacentButtonmap(Settings.ButtonR2_AnalogR_Left, false);
							if(CellInput->IsButtonPressed(0,CTRL_LEFT) | CellInput->IsAnalogPressedLeft(0,CTRL_LSTICK))
							{
								sys_timer_usleep(SETTINGS_DELAY);
							}
						}
						if(CellInput->WasButtonHeld(0, CTRL_RIGHT) | CellInput->IsAnalogPressedRight(0, CTRL_LSTICK) | CellInput->WasButtonPressed(0, CTRL_CROSS))
						{
							Settings.ButtonR2_AnalogR_Left = Menu_GetAdjacentButtonmap(Settings.ButtonR2_AnalogR_Left, true);
							if(CellInput->IsButtonPressed(0,CTRL_RIGHT) | CellInput->IsAnalogPressedRight(0,CTRL_LSTICK))
							{
								sys_timer_usleep(SETTINGS_DELAY);
							}
						}
						if(CellInput->IsButtonPressed(0, CTRL_START))
						{
							Settings.ButtonR2_AnalogR_Left = INPUT_NONE;
						}
						break;
						case SETTING_CONTROLS_BUTTON_R2_ANALOG_R_UP:
						if(CellInput->WasButtonHeld(0, CTRL_LEFT) | CellInput->IsAnalogPressedLeft(0, CTRL_LSTICK))
						{
							Settings.ButtonR2_AnalogR_Up = Menu_GetAdjacentButtonmap(Settings.ButtonR2_AnalogR_Up, false);
							if(CellInput->IsButtonPressed(0,CTRL_LEFT) | CellInput->IsAnalogPressedLeft(0,CTRL_LSTICK))
							{
								sys_timer_usleep(SETTINGS_DELAY);
							}
						}
						if(CellInput->WasButtonHeld(0, CTRL_RIGHT) | CellInput->IsAnalogPressedRight(0, CTRL_LSTICK) | CellInput->WasButtonPressed(0, CTRL_CROSS))
						{
							Settings.ButtonR2_AnalogR_Up = Menu_GetAdjacentButtonmap(Settings.ButtonR2_AnalogR_Up, true);
							if(CellInput->IsButtonPressed(0,CTRL_RIGHT) | CellInput->IsAnalogPressedRight(0,CTRL_LSTICK))
							{
								sys_timer_usleep(SETTINGS_DELAY);
							}
						}
						if(CellInput->IsButtonPressed(0, CTRL_START))
						{
							Settings.ButtonR2_AnalogR_Up = INPUT_NONE;
						}
						break;
						case SETTING_CONTROLS_BUTTON_R2_ANALOG_R_DOWN:
						if(CellInput->WasButtonHeld(0, CTRL_LEFT) | CellInput->IsAnalogPressedLeft(0, CTRL_LSTICK))
						{
							Settings.ButtonR2_AnalogR_Down = Menu_GetAdjacentButtonmap(Settings.ButtonR2_AnalogR_Down, false);
							if(CellInput->IsButtonPressed(0,CTRL_LEFT) | CellInput->IsAnalogPressedLeft(0,CTRL_LSTICK))
							{
								sys_timer_usleep(SETTINGS_DELAY);
							}
						}
						if(CellInput->WasButtonHeld(0, CTRL_RIGHT) | CellInput->IsAnalogPressedRight(0, CTRL_LSTICK) | CellInput->WasButtonPressed(0, CTRL_CROSS))
						{
							Settings.ButtonR2_AnalogR_Down = Menu_GetAdjacentButtonmap(Settings.ButtonR2_AnalogR_Down, true);
							if(CellInput->IsButtonPressed(0,CTRL_RIGHT) | CellInput->IsAnalogPressedRight(0,CTRL_LSTICK))
							{
								sys_timer_usleep(SETTINGS_DELAY);
							}
						}
						if(CellInput->IsButtonPressed(0, CTRL_START))
						{
							Settings.ButtonR2_AnalogR_Down = INPUT_NONE;
						}
						break;
						case SETTING_CONTROLS_BUTTON_R3_BUTTON_L3:
						if(CellInput->WasButtonHeld(0, CTRL_LEFT) | CellInput->IsAnalogPressedLeft(0, CTRL_LSTICK))
						{
							Settings.ButtonR3_ButtonL3 = Menu_GetAdjacentButtonmap(Settings.ButtonR3_ButtonL3, false);
							if(CellInput->IsButtonPressed(0,CTRL_LEFT) | CellInput->IsAnalogPressedLeft(0,CTRL_LSTICK))
							{
								sys_timer_usleep(SETTINGS_DELAY);
							}
						}
						if(CellInput->WasButtonHeld(0, CTRL_RIGHT) | CellInput->IsAnalogPressedRight(0, CTRL_LSTICK) | CellInput->WasButtonPressed(0, CTRL_CROSS))
						{
							Settings.ButtonR3_ButtonL3 = Menu_GetAdjacentButtonmap(Settings.ButtonR3_ButtonL3, true);
							if(CellInput->IsButtonPressed(0,CTRL_RIGHT) | CellInput->IsAnalogPressedRight(0,CTRL_LSTICK))
							{
								sys_timer_usleep(SETTINGS_DELAY);
							}
						}
						if(CellInput->IsButtonPressed(0, CTRL_START))
						{
							Settings.ButtonR3_ButtonL3 = INPUT_QUIT;
						}
						break;
						case SETTING_CONTROLS_DEFAULT_ALL:
						if(CellInput->WasButtonPressed(0, CTRL_LEFT) | CellInput->WasAnalogPressedLeft(0, CTRL_LSTICK) | CellInput->WasButtonPressed(0, CTRL_RIGHT) | CellInput->WasAnalogPressedRight(0, CTRL_LSTICK) | CellInput->WasButtonPressed(0, CTRL_CROSS) | CellInput->IsButtonPressed(0, CTRL_START))
						{
							Emulator_Implementation_ButtonMappingSettings(MAP_BUTTONS_OPTION_DEFAULT);
						}
							break;
				default:
					break;
			} // end of switch
	}

	float yPos = 0.09;
	float ySpacing = 0.04;

	cellDbgFontPuts		(0.09f,	0.05f,	Emulator_GetFontSize(),	GREEN,	"GENERAL");
	cellDbgFontPuts		(0.27f,	0.05f,	Emulator_GetFontSize(),	GREEN,	"GENESIS PLUS");
	cellDbgFontPuts		(0.45f,	0.05f,	Emulator_GetFontSize(),	GREEN,	"PATH");
	cellDbgFontPuts		(0.63f, 0.05f,	Emulator_GetFontSize(), RED,	"CONTROLS"); 
	Graphics->FlushDbgFont();

//PAGE 1
//FIXME: Terrible hardcoding
if((currently_selected_controller_setting-FIRST_CONTROLS_SETTING) < NUM_ENTRY_PER_SETTINGS_PAGE)
{
	cellDbgFontPuts		(0.09f,	yPos,	Emulator_GetFontSize(),	currently_selected_controller_setting == SETTING_CONTROLS_DPAD_UP ? YELLOW : WHITE,	"D-Pad Up");
	cellDbgFontPuts		(0.5f,	yPos,	Emulator_GetFontSize(),	Settings.DPad_Up == INPUT_UP ? GREEN : ORANGE, Menu_PrintMappedButton(Settings.DPad_Up));

	yPos += ySpacing;
	cellDbgFontPuts		(0.09f,	yPos,	Emulator_GetFontSize(),	currently_selected_controller_setting == SETTING_CONTROLS_DPAD_DOWN ? YELLOW : WHITE,	"D-Pad Down");
	cellDbgFontPuts		(0.5f,	yPos,	Emulator_GetFontSize(),	Settings.DPad_Down == INPUT_DOWN ? GREEN : ORANGE, Menu_PrintMappedButton(Settings.DPad_Down));
	Graphics->FlushDbgFont();

	yPos += ySpacing;
	cellDbgFontPuts		(0.09f,	yPos,	Emulator_GetFontSize(),	currently_selected_controller_setting == SETTING_CONTROLS_DPAD_LEFT ? YELLOW : WHITE,	"D-Pad Left");
	cellDbgFontPuts		(0.5f,	yPos,	Emulator_GetFontSize(),	Settings.DPad_Left == INPUT_LEFT ? GREEN : ORANGE, Menu_PrintMappedButton(Settings.DPad_Left));
	Graphics->FlushDbgFont();

	yPos += ySpacing;
	cellDbgFontPuts		(0.09f,	yPos,	Emulator_GetFontSize(),	currently_selected_controller_setting == SETTING_CONTROLS_DPAD_RIGHT ? YELLOW : WHITE,	"D-Pad Right");
	cellDbgFontPuts		(0.5f,	yPos,	Emulator_GetFontSize(),	Settings.DPad_Right == INPUT_RIGHT ? GREEN : ORANGE, Menu_PrintMappedButton(Settings.DPad_Right));
	Graphics->FlushDbgFont();

	yPos += ySpacing;
	cellDbgFontPuts		(0.09f,	yPos,	Emulator_GetFontSize(),	currently_selected_controller_setting == SETTING_CONTROLS_BUTTON_CIRCLE ? YELLOW : WHITE,	"Circle button");
	cellDbgFontPuts		(0.5f,	yPos,	Emulator_GetFontSize(),	Settings.ButtonCircle == INPUT_C ? GREEN : ORANGE, Menu_PrintMappedButton(Settings.ButtonCircle));
	Graphics->FlushDbgFont();

	yPos += ySpacing;
	cellDbgFontPuts		(0.09f,	yPos,	Emulator_GetFontSize(),	currently_selected_controller_setting == SETTING_CONTROLS_BUTTON_CROSS ? YELLOW : WHITE,	"Cross button");
	cellDbgFontPuts		(0.5f,	yPos,	Emulator_GetFontSize(),	Settings.ButtonCross == INPUT_B ? GREEN : ORANGE, Menu_PrintMappedButton(Settings.ButtonCross));
	Graphics->FlushDbgFont();

	yPos += ySpacing;
	cellDbgFontPuts		(0.09f,	yPos,	Emulator_GetFontSize(),	currently_selected_controller_setting == SETTING_CONTROLS_BUTTON_TRIANGLE ? YELLOW : WHITE,	"Triangle button");
	cellDbgFontPuts		(0.5f,	yPos,	Emulator_GetFontSize(),	Settings.ButtonTriangle == INPUT_X ? GREEN : ORANGE, Menu_PrintMappedButton(Settings.ButtonTriangle));
	Graphics->FlushDbgFont();

	yPos += ySpacing;
	cellDbgFontPuts		(0.09f,	yPos,	Emulator_GetFontSize(),	currently_selected_controller_setting == SETTING_CONTROLS_BUTTON_SQUARE ? YELLOW : WHITE,	"Square button");
	cellDbgFontPuts		(0.5f,	yPos,	Emulator_GetFontSize(),	Settings.ButtonSquare == INPUT_A ? GREEN : ORANGE, Menu_PrintMappedButton(Settings.ButtonSquare));
	Graphics->FlushDbgFont();

	yPos += ySpacing;
	cellDbgFontPuts		(0.09f,	yPos,	Emulator_GetFontSize(),	currently_selected_controller_setting == SETTING_CONTROLS_BUTTON_SELECT ? YELLOW : WHITE,	"Select button");
	cellDbgFontPuts		(0.5f,	yPos,	Emulator_GetFontSize(),	Settings.ButtonSelect == INPUT_MODE ? GREEN : ORANGE, Menu_PrintMappedButton(Settings.ButtonSelect));
	Graphics->FlushDbgFont();

	yPos += ySpacing;
	cellDbgFontPuts		(0.09f,	yPos,	Emulator_GetFontSize(),	currently_selected_controller_setting == SETTING_CONTROLS_BUTTON_START ? YELLOW : WHITE,	"Start button");
	cellDbgFontPuts		(0.5f,	yPos,	Emulator_GetFontSize(),	Settings.ButtonStart == INPUT_START ? GREEN : ORANGE, Menu_PrintMappedButton(Settings.ButtonStart));
	Graphics->FlushDbgFont();

	yPos += ySpacing;
	cellDbgFontPuts		(0.09f,	yPos,	Emulator_GetFontSize(),	currently_selected_controller_setting == SETTING_CONTROLS_BUTTON_L1 ? YELLOW : WHITE,	"L1 button");
	cellDbgFontPuts		(0.5f,	yPos,	Emulator_GetFontSize(),	Settings.ButtonL1 == INPUT_Y ? GREEN : ORANGE, Menu_PrintMappedButton(Settings.ButtonL1));
	Graphics->FlushDbgFont();

	yPos += ySpacing;
	cellDbgFontPuts		(0.09f,	yPos,	Emulator_GetFontSize(),	currently_selected_controller_setting == SETTING_CONTROLS_BUTTON_R1 ? YELLOW : WHITE,	"R1 button");
	cellDbgFontPuts		(0.5f,	yPos,	Emulator_GetFontSize(),	Settings.ButtonR1 == INPUT_Z ? GREEN : ORANGE, Menu_PrintMappedButton(Settings.ButtonR1));
	Graphics->FlushDbgFont();

	yPos += ySpacing;
	cellDbgFontPuts		(0.09f,	yPos,	Emulator_GetFontSize(),	currently_selected_controller_setting == SETTING_CONTROLS_BUTTON_L2 ? YELLOW : WHITE,	"L2 button");
	cellDbgFontPuts		(0.5f,	yPos,	Emulator_GetFontSize(),	Settings.ButtonL2 == INPUT_NONE ? GREEN : ORANGE, Menu_PrintMappedButton(Settings.ButtonL2));
	Graphics->FlushDbgFont();

	yPos += ySpacing;
	cellDbgFontPuts		(0.09f,	yPos,	Emulator_GetFontSize(),	currently_selected_controller_setting == SETTING_CONTROLS_BUTTON_R2 ? YELLOW : WHITE,	"R2 button");
	cellDbgFontPuts		(0.5f,	yPos,	Emulator_GetFontSize(),	Settings.ButtonR2 == INPUT_NONE ? GREEN : ORANGE, Menu_PrintMappedButton(Settings.ButtonR2));
	Graphics->FlushDbgFont();

	yPos += ySpacing;
	cellDbgFontPuts		(0.09f,	yPos,	Emulator_GetFontSize(),	currently_selected_controller_setting == SETTING_CONTROLS_BUTTON_L3 ? YELLOW : WHITE,	"L3 button");
	cellDbgFontPuts		(0.5f,	yPos,	Emulator_GetFontSize(),	Settings.ButtonL3 == INPUT_NONE ? GREEN : ORANGE, Menu_PrintMappedButton(Settings.ButtonL3));
	Graphics->FlushDbgFont();

	yPos += ySpacing;
	cellDbgFontPuts		(0.09f,	yPos,	Emulator_GetFontSize(),	currently_selected_controller_setting == SETTING_CONTROLS_BUTTON_R3 ? YELLOW : WHITE,	"R3 button");
	cellDbgFontPuts		(0.5f,	yPos,	Emulator_GetFontSize(),	Settings.ButtonR3 == INPUT_NONE ? GREEN : ORANGE, Menu_PrintMappedButton(Settings.ButtonR3));
	Graphics->FlushDbgFont();

	yPos += ySpacing;
	cellDbgFontPuts		(0.09f,	yPos,	Emulator_GetFontSize(),	currently_selected_controller_setting == SETTING_CONTROLS_BUTTON_L2_BUTTON_L3 ? YELLOW : WHITE,	"Button combo: L2 & L3");
	cellDbgFontPuts		(0.5f,	yPos,	Emulator_GetFontSize(),	Settings.ButtonL2_ButtonL3 == INPUT_NONE ? GREEN : ORANGE, Menu_PrintMappedButton(Settings.ButtonL2_ButtonL3));
	Graphics->FlushDbgFont();

	yPos += ySpacing;
	cellDbgFontPuts		(0.09f,	yPos,	Emulator_GetFontSize(),	currently_selected_controller_setting == SETTING_CONTROLS_BUTTON_L2_BUTTON_R2 ? YELLOW : WHITE,	"Button combo: L2 & R2");
	cellDbgFontPuts		(0.5f,	yPos,	Emulator_GetFontSize(),	Settings.ButtonL2_ButtonR2 == INPUT_NONE ? GREEN : ORANGE, Menu_PrintMappedButton(Settings.ButtonL2_ButtonR2));
	Graphics->FlushDbgFont();

}

//PAGE 2
//FIXME: Terrible hardcoding 
if((currently_selected_controller_setting-FIRST_CONTROLS_SETTING) >= NUM_ENTRY_PER_SETTINGS_PAGE)
{
	yPos = 0.09;

	cellDbgFontPuts		(0.09f,	yPos,	Emulator_GetFontSize(),	currently_selected_controller_setting == SETTING_CONTROLS_BUTTON_L2_BUTTON_R3 ? YELLOW : WHITE,	"Button combo: L2 & R3");
	cellDbgFontPuts		(0.5f,	yPos,	Emulator_GetFontSize(),	Settings.ButtonL2_ButtonR3 == INPUT_LOADSTATE ? GREEN : ORANGE, Menu_PrintMappedButton(Settings.ButtonL2_ButtonR3));
	Graphics->FlushDbgFont();

	yPos += ySpacing;
	cellDbgFontPuts		(0.09f,	yPos,	Emulator_GetFontSize(),	currently_selected_controller_setting == SETTING_CONTROLS_BUTTON_L2_ANALOG_R_RIGHT ? YELLOW : WHITE,	"Button combo: L2 & RStick Right");
	cellDbgFontPuts		(0.5f,	yPos,	Emulator_GetFontSize(),	Settings.ButtonL2_AnalogR_Right == INPUT_NONE ? GREEN : ORANGE, Menu_PrintMappedButton(Settings.ButtonL2_AnalogR_Right));
	Graphics->FlushDbgFont();

	yPos += ySpacing;
	cellDbgFontPuts		(0.09f,	yPos,	Emulator_GetFontSize(),	currently_selected_controller_setting == SETTING_CONTROLS_BUTTON_L2_ANALOG_R_LEFT ? YELLOW : WHITE,	"Button combo: L2 & RStick Left");
	cellDbgFontPuts		(0.5f,	yPos,	Emulator_GetFontSize(),	Settings.ButtonL2_AnalogR_Left == INPUT_NONE ? GREEN : ORANGE, Menu_PrintMappedButton(Settings.ButtonL2_AnalogR_Left));
	Graphics->FlushDbgFont();

	yPos += ySpacing;
	cellDbgFontPuts		(0.09f,	yPos,	Emulator_GetFontSize(),	currently_selected_controller_setting == SETTING_CONTROLS_BUTTON_L2_ANALOG_R_UP ? YELLOW : WHITE,	"Button combo: L2 & RStick Up");
	cellDbgFontPuts		(0.5f,	yPos,	Emulator_GetFontSize(),	Settings.ButtonL2_AnalogR_Up == INPUT_NONE ? GREEN : ORANGE, Menu_PrintMappedButton(Settings.ButtonL2_AnalogR_Up));
	Graphics->FlushDbgFont();

	yPos += ySpacing;
	cellDbgFontPuts		(0.09f,	yPos,	Emulator_GetFontSize(),	currently_selected_controller_setting == SETTING_CONTROLS_BUTTON_L2_ANALOG_R_DOWN ? YELLOW : WHITE,	"Button combo: L2 & RStick Down");
	cellDbgFontPuts		(0.5f,	yPos,	Emulator_GetFontSize(),	Settings.ButtonL2_AnalogR_Down == INPUT_NONE ? GREEN : ORANGE, Menu_PrintMappedButton(Settings.ButtonL2_AnalogR_Down));
	Graphics->FlushDbgFont();

	yPos += ySpacing;
	cellDbgFontPuts		(0.09f,	yPos,	Emulator_GetFontSize(),	currently_selected_controller_setting == SETTING_CONTROLS_BUTTON_R2_ANALOG_R_RIGHT ? YELLOW : WHITE,	"Button combo: R2 & RStick Right");
	cellDbgFontPuts		(0.5f,	yPos,	Emulator_GetFontSize(),	Settings.ButtonR2_AnalogR_Right == INPUT_NONE ? GREEN : ORANGE, Menu_PrintMappedButton(Settings.ButtonR2_AnalogR_Right));
	Graphics->FlushDbgFont();

	yPos += ySpacing;
	cellDbgFontPuts		(0.09f,	yPos,	Emulator_GetFontSize(),	currently_selected_controller_setting == SETTING_CONTROLS_BUTTON_R2_ANALOG_R_LEFT ? YELLOW : WHITE,	"Button combo: R2 & RStick Left");
	cellDbgFontPuts		(0.5f,	yPos,	Emulator_GetFontSize(),	Settings.ButtonR2_AnalogR_Left == INPUT_NONE ? GREEN : ORANGE, Menu_PrintMappedButton(Settings.ButtonR2_AnalogR_Left));
	Graphics->FlushDbgFont();

	yPos += ySpacing;
	cellDbgFontPuts		(0.09f,	yPos,	Emulator_GetFontSize(),	currently_selected_controller_setting == SETTING_CONTROLS_BUTTON_R2_ANALOG_R_UP ? YELLOW : WHITE,	"Button combo: R2 & RStick Up");
	cellDbgFontPuts		(0.5f,	yPos,	Emulator_GetFontSize(),	Settings.ButtonR2_AnalogR_Up == INPUT_NONE ? GREEN : ORANGE, Menu_PrintMappedButton(Settings.ButtonR2_AnalogR_Up));
	Graphics->FlushDbgFont();

	yPos += ySpacing;
	cellDbgFontPuts		(0.09f,	yPos,	Emulator_GetFontSize(),	currently_selected_controller_setting == SETTING_CONTROLS_BUTTON_R2_ANALOG_R_DOWN ? YELLOW : WHITE,	"Button combo: R2 & RStick Down");
	cellDbgFontPuts		(0.5f,	yPos,	Emulator_GetFontSize(),	Settings.ButtonR2_AnalogR_Down == INPUT_NONE ? GREEN : ORANGE, Menu_PrintMappedButton(Settings.ButtonR2_AnalogR_Down));
	Graphics->FlushDbgFont();

	yPos += ySpacing;
	cellDbgFontPuts		(0.09f,	yPos,	Emulator_GetFontSize(),	currently_selected_controller_setting == SETTING_CONTROLS_BUTTON_R2_BUTTON_R3 ? YELLOW : WHITE,	"Button combo: R2 & R3");
	cellDbgFontPuts		(0.5f,	yPos,	Emulator_GetFontSize(),	Settings.ButtonR2_ButtonR3 == INPUT_SAVESTATE ? GREEN : ORANGE, Menu_PrintMappedButton(Settings.ButtonR2_ButtonR3));
	Graphics->FlushDbgFont();

	yPos += ySpacing;
	cellDbgFontPuts		(0.09f,	yPos,	Emulator_GetFontSize(),	currently_selected_controller_setting == SETTING_CONTROLS_BUTTON_R3_BUTTON_L3 ? YELLOW : WHITE,	"Button combo: R3 & L3");
	cellDbgFontPuts		(0.5f,	yPos,	Emulator_GetFontSize(),	Settings.ButtonR3_ButtonL3 == INPUT_QUIT ? GREEN : ORANGE, Menu_PrintMappedButton(Settings.ButtonR3_ButtonL3));
	Graphics->FlushDbgFont();

	yPos += ySpacing;
	cellDbgFontPrintf		(0.09f,	yPos,	Emulator_GetFontSize(),	currently_selected_controller_setting == SETTING_CONTROLS_ANALOG_R_UP ? YELLOW : WHITE,	"Right Stick - Up %s", Settings.AnalogR_Up_Type ? "(IsPressed)" : "(WasPressed)");
	cellDbgFontPuts		(0.5f,	yPos,	Emulator_GetFontSize(),	Settings.AnalogR_Up == INPUT_NONE ? GREEN : ORANGE, Menu_PrintMappedButton(Settings.AnalogR_Up));
	Graphics->FlushDbgFont();

	yPos += ySpacing;
	cellDbgFontPrintf		(0.09f,	yPos,	Emulator_GetFontSize(),	currently_selected_controller_setting == SETTING_CONTROLS_ANALOG_R_DOWN ? YELLOW : WHITE,	"Right Stick - Down %s", Settings.AnalogR_Down_Type ? "(IsPressed)" : "(WasPressed)");
	cellDbgFontPuts		(0.5f,	yPos,	Emulator_GetFontSize(),	Settings.AnalogR_Down == INPUT_NONE ? GREEN : ORANGE, Menu_PrintMappedButton(Settings.AnalogR_Down));
	Graphics->FlushDbgFont();

	yPos += ySpacing;
	cellDbgFontPrintf		(0.09f,	yPos,	Emulator_GetFontSize(),	currently_selected_controller_setting == SETTING_CONTROLS_ANALOG_R_LEFT ? YELLOW : WHITE,	"Right Stick - Left %s", Settings.AnalogR_Left_Type ? "(IsPressed)" : "(WasPressed)");
	cellDbgFontPuts		(0.5f,	yPos,	Emulator_GetFontSize(),	Settings.AnalogR_Left == INPUT_NONE ? GREEN : ORANGE, Menu_PrintMappedButton(Settings.AnalogR_Left));
	Graphics->FlushDbgFont();

	yPos += ySpacing;
	cellDbgFontPrintf		(0.09f,	yPos,	Emulator_GetFontSize(),	currently_selected_controller_setting == SETTING_CONTROLS_ANALOG_R_RIGHT ? YELLOW : WHITE,	"Right Stick - Right %s", Settings.AnalogR_Right_Type ? "(IsPressed)" : "(WasPressed)");
	cellDbgFontPuts		(0.5f,	yPos,	Emulator_GetFontSize(),	Settings.AnalogR_Right == INPUT_NONE ? GREEN : ORANGE, Menu_PrintMappedButton(Settings.AnalogR_Right));
	Graphics->FlushDbgFont();

	yPos += ySpacing;
	cellDbgFontPrintf(0.09f, yPos, Emulator_GetFontSize(), currently_selected_controller_setting == SETTING_CONTROLS_DEFAULT_ALL ? YELLOW : GREEN, "DEFAULT");
}

	DisplayHelpMessage(currently_selected_controller_setting);

	cellDbgFontPuts(0.09f, 0.89f, Emulator_GetFontSize(), YELLOW,
	"UP/DOWN - select  L2+R2 - resume game   X/LEFT/RIGHT - change");
	cellDbgFontPuts(0.09f, 0.93f, Emulator_GetFontSize(), YELLOW,
	"START - default   L1/CIRCLE - go back");
	Graphics->FlushDbgFont();
}

void do_path_settings()
{
	if(CellInput->UpdateDevice(0) == CELL_OK)
	{
			// back to ROM menu if CIRCLE is pressed
			if (CellInput->WasButtonPressed(0, CTRL_L1) | CellInput->WasButtonPressed(0, CTRL_CIRCLE))
			{
				menuStack.pop();
				return;
			}

			if (CellInput->WasButtonPressed(0, CTRL_R1))
			{
				menuStack.push(do_controls_settings);
				return;
			}

			if (CellInput->WasButtonHeld(0, CTRL_DOWN) | CellInput->IsAnalogPressedDown(0, CTRL_LSTICK))	// down to next setting
			{
				currently_selected_path_setting++;
				if (currently_selected_path_setting >= MAX_NO_OF_PATH_SETTINGS)
				{
					currently_selected_path_setting = FIRST_PATH_SETTING;
				}

				if(CellInput->IsButtonPressed(0,CTRL_DOWN) | CellInput->IsAnalogPressedDown(0, CTRL_LSTICK))
				{
					sys_timer_usleep(SETTINGS_DELAY);
				}
			}

			if (CellInput->IsButtonPressed(0,CTRL_L2) && CellInput->IsButtonPressed(0,CTRL_R2))
			{
				// if a rom is loaded then resume it
				if (Emulator_IsROMLoaded())
				{
					MenuStop();
					Emulator_StartROMRunning();
				}
				return;
			}

			if (CellInput->WasButtonHeld(0, CTRL_UP) | CellInput->IsAnalogPressedUp(0, CTRL_LSTICK))	// up to previous setting
			{
					currently_selected_path_setting--;
					if (currently_selected_path_setting < FIRST_PATH_SETTING)
					{
						currently_selected_path_setting = MAX_NO_OF_PATH_SETTINGS-1;
					}
					if(CellInput->IsButtonPressed(0,CTRL_UP) | CellInput->IsAnalogPressedUp(0, CTRL_LSTICK))
					{
						sys_timer_usleep(SETTINGS_DELAY);
					}
			}
					switch(currently_selected_path_setting)
					{
						case SETTING_PATH_DEFAULT_ROM_DIRECTORY:
						if(CellInput->WasButtonPressed(0, CTRL_LEFT) | CellInput->WasAnalogPressedLeft(0, CTRL_LSTICK) | CellInput->WasButtonPressed(0, CTRL_RIGHT) | CellInput->WasAnalogPressedRight(0, CTRL_LSTICK) | CellInput->WasButtonPressed(0, CTRL_CROSS))
						{
							tmpBrowser = NULL;
							menuStack.push(do_pathChoice);
						}
						if(CellInput->IsButtonPressed(0, CTRL_START))
						{
							Settings.PS3PathROMDirectory = "/";
						}
							break;
						case SETTING_PATH_SAVESTATES_DIRECTORY:
						if(CellInput->WasButtonPressed(0, CTRL_LEFT) | CellInput->WasAnalogPressedLeft(0, CTRL_LSTICK) | CellInput->WasButtonPressed(0, CTRL_RIGHT) | CellInput->WasAnalogPressedRight(0, CTRL_LSTICK) | CellInput->WasButtonPressed(0, CTRL_CROSS))
						{
							tmpBrowser = NULL;
							menuStack.push(do_pathChoice);
						}
						if(CellInput->IsButtonPressed(0, CTRL_START))
						{
							Settings.PS3PathSaveStates = USRDIR;
						}
							break;
						case SETTING_PATH_SRAM_DIRECTORY:
						if(CellInput->WasButtonPressed(0, CTRL_LEFT) | CellInput->WasAnalogPressedLeft(0, CTRL_LSTICK) | CellInput->WasButtonPressed(0, CTRL_RIGHT) | CellInput->WasAnalogPressedRight(0, CTRL_LSTICK) | CellInput->WasButtonPressed(0, CTRL_CROSS))
						{
							tmpBrowser = NULL;
							menuStack.push(do_pathChoice);
						}
						if(CellInput->IsButtonPressed(0, CTRL_START))
						{
							Settings.PS3PathSRAM = USRDIR;
						}
							break;
						case SETTING_PATH_DEFAULT_ALL:
						if(CellInput->WasButtonPressed(0, CTRL_LEFT) | CellInput->WasAnalogPressedLeft(0, CTRL_LSTICK) | CellInput->WasButtonPressed(0, CTRL_RIGHT) | CellInput->WasAnalogPressedRight(0, CTRL_LSTICK) | CellInput->WasButtonPressed(0, CTRL_CROSS))
						{
							Settings.PS3PathROMDirectory = "/";
							Settings.PS3PathSaveStates = USRDIR;
							Settings.PS3PathSRAM = USRDIR;
						}
							break;
					break;
				default:
					break;
			} // end of switch
	}

	float yPos = 0.09;
	float ySpacing = 0.04;

	cellDbgFontPuts		(0.09f,	0.05f,	Emulator_GetFontSize(),	GREEN,	"GENERAL");
	cellDbgFontPuts		(0.27f,	0.05f,	Emulator_GetFontSize(),	GREEN,	"GENESIS PLUS");
	cellDbgFontPuts		(0.45f,	0.05f,	Emulator_GetFontSize(),	RED,	"PATHS");
	cellDbgFontPuts		(0.63f,	0.05f,	Emulator_GetFontSize(),	GREEN,	"CONTROLS");
	Graphics->FlushDbgFont();

	cellDbgFontPuts		(0.09f,	yPos,	Emulator_GetFontSize(),	currently_selected_path_setting == SETTING_PATH_DEFAULT_ROM_DIRECTORY ? YELLOW : WHITE,	"Startup ROM Directory");
	cellDbgFontPuts		(0.5f,	yPos,	Emulator_GetFontSize(),	!(strcmp(Settings.PS3PathROMDirectory.c_str(),"/")) ? GREEN : ORANGE, Settings.PS3PathROMDirectory.c_str());

	yPos += ySpacing;
	cellDbgFontPuts		(0.09f,	yPos,	Emulator_GetFontSize(),	currently_selected_path_setting == SETTING_PATH_SAVESTATES_DIRECTORY ? YELLOW : WHITE,	"Savestate Directory");
	cellDbgFontPuts		(0.5f,	yPos,	Emulator_GetFontSize(),	!(strcmp(Settings.PS3PathSaveStates.c_str(),USRDIR)) ? GREEN : ORANGE, Settings.PS3PathSaveStates.c_str());
	Graphics->FlushDbgFont();

	yPos += ySpacing;
	cellDbgFontPuts		(0.09f,	yPos,	Emulator_GetFontSize(),	currently_selected_path_setting == SETTING_PATH_SRAM_DIRECTORY ? YELLOW : WHITE,	"SRAM directory");
	cellDbgFontPuts		(0.5f,	yPos,	Emulator_GetFontSize(),	!(strcmp(Settings.PS3PathSRAM.c_str(),USRDIR)) ? GREEN : ORANGE, Settings.PS3PathSRAM.c_str());

	yPos += ySpacing;
	cellDbgFontPrintf(0.09f, yPos, Emulator_GetFontSize(), currently_selected_path_setting == SETTING_PATH_DEFAULT_ALL ? YELLOW : GREEN, "DEFAULT");

	DisplayHelpMessage(currently_selected_path_setting);

	cellDbgFontPuts(0.09f, 0.89f, Emulator_GetFontSize(), YELLOW,
	"UP/DOWN - select  L2+R2 - resume game   X/LEFT/RIGHT - change");
	cellDbgFontPuts(0.09f, 0.93f, Emulator_GetFontSize(), YELLOW,
	"START - default   L1/CIRCLE - go back   R1 - go forward");
	Graphics->FlushDbgFont();
}

void do_genesis_settings()
{
	if(CellInput->UpdateDevice(0) == CELL_PAD_OK)
	{
			// back to ROM menu if CIRCLE is pressed
			if (CellInput->WasButtonPressed(0, CTRL_L1) | CellInput->WasButtonPressed(0, CTRL_CIRCLE))
			{
				menuStack.pop();
				return;
			}

			if (CellInput->WasButtonPressed(0, CTRL_R1))
			{
				menuStack.push(do_path_settings);
				return;
			}

			if (CellInput->WasButtonHeld(0, CTRL_DOWN) | CellInput->IsAnalogPressedDown(0, CTRL_LSTICK))	// down to next setting
			{
				currently_selected_genesis_setting++;
				if (currently_selected_genesis_setting >= MAX_NO_OF_GENESIS_SETTINGS)
				{
					currently_selected_genesis_setting = FIRST_GENESIS_SETTING;
				}
				if(CellInput->IsButtonPressed(0,CTRL_DOWN) | CellInput->IsAnalogPressedDown(0, CTRL_LSTICK))
				{
					sys_timer_usleep(SETTINGS_DELAY);
				}
			}

			if (CellInput->IsButtonPressed(0,CTRL_L2) && CellInput->IsButtonPressed(0,CTRL_R2))
			{
				// if a rom is loaded then resume it
				if (Emulator_IsROMLoaded())
				{
					MenuStop();
					Emulator_StartROMRunning();
				}
				return;
			}

			if (CellInput->WasButtonHeld(0, CTRL_UP) | CellInput->IsAnalogPressedUp(0, CTRL_LSTICK))	// up to previous setting
			{
					currently_selected_genesis_setting--;
					if (currently_selected_genesis_setting < FIRST_GENESIS_SETTING)
					{
						currently_selected_genesis_setting = MAX_NO_OF_GENESIS_SETTINGS-1;
					}
					if(CellInput->IsButtonPressed(0,CTRL_UP) | CellInput->IsAnalogPressedUp(0, CTRL_LSTICK))
					{
						sys_timer_usleep(SETTINGS_DELAY);
					}
			}
					switch(currently_selected_genesis_setting)
					{
				case SETTING_GENESIS_FPS:
					if(CellInput->WasButtonPressed(0, CTRL_LEFT) | CellInput->WasAnalogPressedLeft(0, CTRL_LSTICK) | CellInput->WasButtonPressed(0, CTRL_RIGHT) | CellInput->WasAnalogPressedRight(0, CTRL_LSTICK) | CellInput->WasButtonPressed(0, CTRL_CROSS))
					{
						Settings.DisplayFrameRate = !Settings.DisplayFrameRate;
					}
					if(CellInput->IsButtonPressed(0, CTRL_START))
					{
						Settings.DisplayFrameRate = false;
					}
					break;
				case SETTING_GENESIS_PAD:
					if(CellInput->WasButtonPressed(0, CTRL_LEFT) | CellInput->WasAnalogPressedLeft(0, CTRL_LSTICK) | CellInput->WasButtonPressed(0, CTRL_RIGHT) | CellInput->WasAnalogPressedRight(0, CTRL_LSTICK) | CellInput->WasButtonPressed(0, CTRL_CROSS))
					{
						Settings.SixButtonPad = !Settings.SixButtonPad;
						Emulator_SetControllerMode();
					}
					if(CellInput->IsButtonPressed(0, CTRL_START))
					{
						Settings.SixButtonPad = false;
						Emulator_SetControllerMode();
					}
					break;
				case SETTING_GENESIS_EXCART:
					if(CellInput->WasButtonPressed(0, CTRL_LEFT) | CellInput->WasAnalogPressedLeft(0, CTRL_LSTICK) | CellInput->WasButtonPressed(0, CTRL_RIGHT) | CellInput->WasAnalogPressedRight(0, CTRL_LSTICK) | CellInput->WasButtonPressed(0, CTRL_CROSS))
					{
						Settings.ExtraCart = !Settings.ExtraCart;
					}
					if(CellInput->IsButtonPressed(0, CTRL_START))
					{
						Settings.ExtraCart = false;
					}
					break;
				case SETTING_GENESIS_ACTIONREPLAY_ROMPATH:
					if(CellInput->WasButtonPressed(0, CTRL_LEFT) | CellInput->WasAnalogPressedLeft(0, CTRL_LSTICK) | CellInput->WasButtonPressed(0, CTRL_RIGHT) | CellInput->WasAnalogPressedRight(0, CTRL_LSTICK) | CellInput->WasButtonPressed(0, CTRL_CROSS))
					{
						tmpBrowser = NULL;
						menuStack.push(do_extracartChoice);
					}
					if(CellInput->IsButtonPressed(0, CTRL_START))
					{
						Settings.ActionReplayROMPath.assign("/dev_hdd0/game/GENP00001/USRDIR/areplay.bin");
						Emulator_SetExtraCartPaths();
					}
					break;
				case SETTING_GENESIS_GAMEGENIE_ROMPATH:
					if(CellInput->WasButtonPressed(0, CTRL_LEFT) | CellInput->WasAnalogPressedLeft(0, CTRL_LSTICK) | CellInput->WasButtonPressed(0, CTRL_RIGHT) | CellInput->WasAnalogPressedRight(0, CTRL_LSTICK) | CellInput->WasButtonPressed(0, CTRL_CROSS))
					{
						tmpBrowser = NULL;
						menuStack.push(do_extracartChoice);
					}
					if(CellInput->IsButtonPressed(0, CTRL_START))
					{
						Settings.GameGenieROMPath.assign("/dev_hdd0/game/GENP00001/USRDIR/ggenie.bin");
						Emulator_SetExtraCartPaths();
					}
					break;
				case SETTING_GENESIS_SK_ROMPATH:
					if(CellInput->WasButtonPressed(0, CTRL_LEFT) | CellInput->WasAnalogPressedLeft(0, CTRL_LSTICK) | CellInput->WasButtonPressed(0, CTRL_RIGHT) | CellInput->WasAnalogPressedRight(0, CTRL_LSTICK) | CellInput->WasButtonPressed(0, CTRL_CROSS))
					{
						tmpBrowser = NULL;
						menuStack.push(do_extracartChoice);
					}
					if(CellInput->IsButtonPressed(0, CTRL_START))
					{
						Settings.SKROMPath.assign("/dev_hdd0/game/GENP00001/USRDIR/sk.bin");
						Emulator_SetExtraCartPaths();
					}
					break;
				case SETTING_GENESIS_SK_UPMEM_ROMPATH:
					if(CellInput->WasButtonPressed(0, CTRL_LEFT) | CellInput->WasAnalogPressedLeft(0, CTRL_LSTICK) | CellInput->WasButtonPressed(0, CTRL_RIGHT) | CellInput->WasAnalogPressedRight(0, CTRL_LSTICK) | CellInput->WasButtonPressed(0, CTRL_CROSS))
					{
						tmpBrowser = NULL;
						menuStack.push(do_extracartChoice);
					}
					if(CellInput->IsButtonPressed(0, CTRL_START))
					{
						Settings.SKUpmemROMPath.assign("/dev_hdd0/game/GENP00001/USRDIR/sk2chip.bin");
						Emulator_SetExtraCartPaths();
					}
					break;
				case SETTING_GENESIS_BIOS_ROMPATH:
					if(CellInput->WasButtonPressed(0, CTRL_LEFT) | CellInput->WasAnalogPressedLeft(0, CTRL_LSTICK) | CellInput->WasButtonPressed(0, CTRL_RIGHT) | CellInput->WasAnalogPressedRight(0, CTRL_LSTICK) | CellInput->WasButtonPressed(0, CTRL_CROSS))
					{
						tmpBrowser = NULL;
						menuStack.push(do_extracartChoice);
					}
					if(CellInput->IsButtonPressed(0, CTRL_START))
					{
						Settings.BIOS.assign("/dev_hdd0/game/GENP00001/USRDIR/bios.bin");
						Emulator_SetExtraCartPaths();
					}
					break;
				case SETTING_GENESIS_DEFAULT_ALL:
					if(CellInput->WasButtonPressed(0, CTRL_LEFT) | CellInput->WasAnalogPressedLeft(0,CTRL_LSTICK) | CellInput->WasButtonPressed(0, CTRL_RIGHT) | CellInput->WasAnalogPressedRight(0,CTRL_LSTICK) | CellInput->IsButtonPressed(0, CTRL_START) | CellInput->WasButtonPressed(0, CTRL_CROSS))
					{
						Settings.ActionReplayROMPath.assign("/dev_hdd0/game/GENP00001/USRDIR/areplay.bin");
						Settings.GameGenieROMPath.assign("/dev_hdd0/game/GENP00001/USRDIR/ggenie.bin");
						Settings.SKROMPath.assign("/dev_hdd0/game/GENP00001/USRDIR/sk.bin");
						Settings.SKUpmemROMPath.assign("/dev_hdd0/game/GENP00001/USRDIR/sk2chip.bin");
						Settings.BIOS.assign("/dev_hdd0/game/GENP00001/USRDIR/bios.bin");
						Emulator_SetExtraCartPaths();
						Settings.ExtraCart = false;
						Settings.DisplayFrameRate = false;
						Settings.SixButtonPad = false;
						Emulator_SetControllerMode();
					}
					break;
				default:
					break;
			} // end of switch
	}

	float yPos = 0.09;
	float ySpacing = 0.04;

	cellDbgFontPuts		(0.09f,	0.05f,	Emulator_GetFontSize(),	GREEN,	"GENERAL");
	cellDbgFontPuts		(0.27f,	0.05f,	Emulator_GetFontSize(),	RED,	"GENESIS PLUS");
	cellDbgFontPuts		(0.45f,	0.05f,	Emulator_GetFontSize(),	GREEN,	"PATHS");
	cellDbgFontPuts		(0.63f,	0.05f,	Emulator_GetFontSize(),	GREEN,	"CONTROLS");
	Graphics->FlushDbgFont();

	cellDbgFontPuts(0.09f, yPos, Emulator_GetFontSize(), currently_selected_genesis_setting == SETTING_GENESIS_FPS ? YELLOW : WHITE, "Display Framerate");
	cellDbgFontPrintf(0.5f, yPos, Emulator_GetFontSize(), Settings.DisplayFrameRate == true ? ORANGE : GREEN,
			"%s", Settings.DisplayFrameRate == true ? "Enabled" : "Disabled");
	
	yPos += ySpacing;
	cellDbgFontPuts(0.09f, yPos, Emulator_GetFontSize(), currently_selected_genesis_setting == SETTING_GENESIS_PAD ? YELLOW : WHITE, "Gamepad type");
	cellDbgFontPrintf(0.5f, yPos, Emulator_GetFontSize(), Settings.SixButtonPad == true ? ORANGE : GREEN,
			"%s", Settings.SixButtonPad == true ? "6 buttons" : "3 buttons");

	yPos += ySpacing;
	cellDbgFontPuts(0.09f, yPos, Emulator_GetFontSize(), currently_selected_genesis_setting == SETTING_GENESIS_EXCART ? YELLOW : WHITE, "Extra Cartridge");
	cellDbgFontPrintf(0.5f, yPos, Emulator_GetFontSize(), Settings.ExtraCart == true ? ORANGE : GREEN,
			"%s", Settings.ExtraCart == true ? "ON" : "OFF");

	yPos += ySpacing;
	cellDbgFontPuts(0.09f, yPos, Emulator_GetFontSize(), currently_selected_genesis_setting == SETTING_GENESIS_ACTIONREPLAY_ROMPATH ? YELLOW : WHITE, "Action Replay ROM Path");
	cellDbgFontPrintf(0.5f, yPos, Emulator_GetFontSize(), !(strcmp(Settings.ActionReplayROMPath.c_str(),"/dev_hdd0/game/GENP00001/USRDIR/areplay.bin")) ? GREEN : ORANGE, Settings.ActionReplayROMPath.c_str());

	yPos += ySpacing;
	cellDbgFontPuts(0.09f, yPos, Emulator_GetFontSize(), currently_selected_genesis_setting == SETTING_GENESIS_GAMEGENIE_ROMPATH ? YELLOW : WHITE, "Game Genie ROM Path");
	cellDbgFontPrintf(0.5f, yPos, Emulator_GetFontSize(), !(strcmp(Settings.GameGenieROMPath.c_str(),"/dev_hdd0/game/GENP00001/USRDIR/ggenie.bin")) ? GREEN : ORANGE, Settings.GameGenieROMPath.c_str());

	yPos += ySpacing;
	cellDbgFontPuts(0.09f, yPos, Emulator_GetFontSize(), currently_selected_genesis_setting == SETTING_GENESIS_SK_ROMPATH ? YELLOW : WHITE, "Sonic & Knuckles ROM Path");
	cellDbgFontPrintf(0.5f, yPos, Emulator_GetFontSize(), !(strcmp(Settings.SKROMPath.c_str(),"/dev_hdd0/game/GENP00001/USRDIR/sk.bin")) ? GREEN : ORANGE, Settings.SKROMPath.c_str());

	yPos += ySpacing;
	cellDbgFontPuts(0.09f, yPos, Emulator_GetFontSize(), currently_selected_genesis_setting == SETTING_GENESIS_SK_UPMEM_ROMPATH ? YELLOW : WHITE, "S&K Upmem ROM Path");
	cellDbgFontPrintf(0.5f, yPos, Emulator_GetFontSize(), !(strcmp(Settings.SKUpmemROMPath.c_str(),"/dev_hdd0/game/GENP00001/USRDIR/sk2chip.bin")) ? GREEN : ORANGE, Settings.SKUpmemROMPath.c_str());

	yPos += ySpacing;
	cellDbgFontPuts(0.09f, yPos, Emulator_GetFontSize(), currently_selected_genesis_setting == SETTING_GENESIS_BIOS_ROMPATH ? YELLOW : WHITE, "BIOS ROM Path");
	cellDbgFontPrintf(0.5f, yPos, Emulator_GetFontSize(), !(strcmp(Settings.BIOS.c_str(),"/dev_hdd0/game/GENP00001/USRDIR/bios.bin")) ? GREEN : ORANGE, Settings.BIOS.c_str());

	yPos += ySpacing;
	cellDbgFontPrintf(0.09f, yPos, Emulator_GetFontSize(), currently_selected_genesis_setting == SETTING_GENESIS_DEFAULT_ALL ? YELLOW : GREEN, "DEFAULT");
	Graphics->FlushDbgFont();

	DisplayHelpMessage(currently_selected_genesis_setting);

	cellDbgFontPuts(0.09f, 0.89f, Emulator_GetFontSize(), YELLOW,
	"UP/DOWN - select  L2+R2 - resume game   X/LEFT/RIGHT - change");
	cellDbgFontPuts(0.09f, 0.93f, Emulator_GetFontSize(), YELLOW,
	"START - default   L1/CIRCLE - go back   R1 - go forward");
	Graphics->FlushDbgFont();
}

void do_general_settings()
{
	if(CellInput->UpdateDevice(0) == CELL_PAD_OK)
	{
			// back to ROM menu if CIRCLE is pressed
			if (CellInput->WasButtonPressed(0, CTRL_CIRCLE) || CellInput->WasButtonPressed(0, CTRL_L1))
			{
				menuStack.pop();
				return;
			}

			if (CellInput->WasButtonPressed(0, CTRL_R1))
			{
				menuStack.push(do_genesis_settings);
				return;
			}

			if (CellInput->WasButtonHeld(0, CTRL_DOWN) | CellInput->IsAnalogPressedDown(0, CTRL_LSTICK))	// down to next setting
			{
				currently_selected_setting++;
				if (currently_selected_setting >= MAX_NO_OF_SETTINGS)
				{
					currently_selected_setting = FIRST_GENERAL_SETTING;
				}
				if(CellInput->IsButtonPressed(0,CTRL_DOWN) | CellInput->IsAnalogPressedDown(0, CTRL_LSTICK))
				{
					sys_timer_usleep(SETTINGS_DELAY);
				}
			}

			if (CellInput->WasButtonHeld(0, CTRL_UP) | CellInput->IsAnalogPressedUp(0, CTRL_LSTICK))	// up to previous setting
			{
					currently_selected_setting--;
					if (currently_selected_setting < FIRST_GENERAL_SETTING)
					{
						currently_selected_setting = MAX_NO_OF_SETTINGS-1;
					}
					if(CellInput->IsButtonPressed(0,CTRL_UP) | CellInput->IsAnalogPressedUp(0, CTRL_LSTICK))
					{
						sys_timer_usleep(SETTINGS_DELAY);
					}
			}

			if (CellInput->IsButtonPressed(0,CTRL_L2) && CellInput->IsButtonPressed(0,CTRL_R2))
			{
				// if a rom is loaded then resume it
				if (Emulator_IsROMLoaded())
				{
					MenuStop();
					Emulator_StartROMRunning();
				}
				return;
			}

					switch(currently_selected_setting)
					{
					case SETTING_CHANGE_RESOLUTION:
						   if(CellInput->WasButtonHeld(0, CTRL_RIGHT) | CellInput->IsAnalogPressedRight(0,CTRL_LSTICK))
						   {
							   Graphics->NextResolution();
							if(CellInput->IsButtonPressed(0,CTRL_RIGHT) | CellInput->IsAnalogPressedRight(0,CTRL_LSTICK))
							{
								sys_timer_usleep(SETTINGS_DELAY);
							}
						   }
						   if(CellInput->WasButtonHeld(0, CTRL_LEFT) | CellInput->IsAnalogPressedLeft(0,CTRL_LSTICK))
						   {
							   Graphics->PreviousResolution();
							if(CellInput->IsButtonPressed(0,CTRL_LEFT) | CellInput->IsAnalogPressedLeft(0,CTRL_LSTICK))
							{
								sys_timer_usleep(SETTINGS_DELAY);
							}
						   }
						   if(CellInput->WasButtonPressed(0, CTRL_CROSS))
						   {
							   Graphics->SwitchResolution(Graphics->GetCurrentResolution(), Settings.PS3PALTemporalMode60Hz);
						   }
						   if(CellInput->IsButtonPressed(0, CTRL_START))
						   {
							   Graphics->SwitchResolution(Graphics->GetInitialResolution(), Settings.PS3PALTemporalMode60Hz);
						   }
						   break;
				/*
				case SETTING_PAL60_MODE:
						   if(CellInput->WasButtonPressed(0, CTRL_RIGHT) | CellInput->WasAnalogPressedLeft(0,CTRL_LSTICK) | CellInput->WasButtonPressed(0,CTRL_CROSS) | CellInput->WasButtonPressed(0, CTRL_LEFT) | CellInput->WasAnalogPressedLeft(0,CTRL_LSTICK))
						   {
							   if (Graphics->GetCurrentResolution() == CELL_VIDEO_OUT_RESOLUTION_576)
							   {
								   if(Graphics->CheckResolution(CELL_VIDEO_OUT_RESOLUTION_576))
								   {
									   Settings.PS3PALTemporalMode60Hz = !Settings.PS3PALTemporalMode60Hz;
									   Graphics->SetPAL60Hz(Settings.PS3PALTemporalMode60Hz);
									   Graphics->SwitchResolution(Graphics->GetCurrentResolution(), Settings.PS3PALTemporalMode60Hz);
								   }
							   }

						   }
						   break;
				*/
				case SETTING_SHADER:
						if(CellInput->WasButtonPressed(0, CTRL_LEFT) | CellInput->WasAnalogPressedLeft(0, CTRL_LSTICK) | CellInput->WasButtonPressed(0, CTRL_RIGHT) | CellInput->WasAnalogPressedRight(0, CTRL_LSTICK) | CellInput->WasButtonPressed(0, CTRL_CROSS))
						{
							menuStack.push(do_shaderChoice);
							tmpBrowser = NULL;
						}
						if(CellInput->IsButtonPressed(0, CTRL_START))
						{
							Graphics->LoadFragmentShader(DEFAULT_SHADER_FILE);
						}
						break;
				case SETTING_FONT_SIZE:
					if(CellInput->WasButtonHeld(0, CTRL_LEFT) | CellInput->IsAnalogPressedLeft(0,CTRL_LSTICK) | CellInput->WasButtonPressed(0,CTRL_CROSS))
					{
						if(Settings.PS3FontSize > -100)
						{
							Settings.PS3FontSize--;
							if(CellInput->IsButtonPressed(0,CTRL_LEFT) | CellInput->IsAnalogPressedLeft(0,CTRL_LSTICK))
							{
								sys_timer_usleep(SETTINGS_DELAY);
							}
						}
					}
					if(CellInput->WasButtonHeld(0, CTRL_RIGHT) | CellInput->IsAnalogPressedRight(0,CTRL_LSTICK) | CellInput->WasButtonPressed(0,CTRL_CROSS))
					{
						if((Settings.PS3FontSize < 200))
						{
							Settings.PS3FontSize++;
							if(CellInput->IsButtonPressed(0,CTRL_RIGHT) | CellInput->IsAnalogPressedRight(0,CTRL_LSTICK))
							{
								sys_timer_usleep(SETTINGS_DELAY);
							}
						}
					}
					if(CellInput->IsButtonPressed(0, CTRL_START))
					{
						Settings.PS3FontSize = 100;
					}
					break;
				case SETTING_KEEP_ASPECT_RATIO:
					if(CellInput->WasButtonPressed(0, CTRL_LEFT) | CellInput->WasAnalogPressedLeft(0,CTRL_LSTICK) | CellInput->WasButtonPressed(0, CTRL_RIGHT) | CellInput->WasAnalogPressedRight(0,CTRL_LSTICK) | CellInput->WasButtonPressed(0,CTRL_CROSS))
					{
						Settings.PS3KeepAspect = !Settings.PS3KeepAspect;
						Graphics->SetAspectRatio(Settings.PS3KeepAspect);
					}
					if(CellInput->IsButtonPressed(0, CTRL_START))
					{
						Settings.PS3KeepAspect = true;
						Graphics->SetAspectRatio(Settings.PS3KeepAspect);
					}
					break;
				case SETTING_HW_TEXTURE_FILTER:
					if(CellInput->WasButtonPressed(0, CTRL_LEFT) | CellInput->WasAnalogPressedLeft(0,CTRL_LSTICK) | CellInput->WasButtonPressed(0, CTRL_RIGHT) | CellInput->WasAnalogPressedRight(0,CTRL_LSTICK) | CellInput->WasButtonPressed(0,CTRL_CROSS))
					{
						Settings.PS3Smooth = !Settings.PS3Smooth;
						Graphics->SetSmooth(Settings.PS3Smooth);
					}
					if(CellInput->IsButtonPressed(0, CTRL_START))
					{
						Settings.PS3Smooth = true;
						Graphics->SetSmooth(Settings.PS3Smooth);
					}
					break;
				case SETTING_HW_OVERSCAN_AMOUNT:
					if(CellInput->WasButtonHeld(0, CTRL_LEFT) | CellInput->IsAnalogPressedLeft(0,CTRL_LSTICK) | CellInput->WasButtonPressed(0,CTRL_CROSS))
					{
						if(Settings.PS3OverscanAmount > -40)
						{
							Settings.PS3OverscanAmount--;
							Settings.PS3OverscanEnabled = true;
							Graphics->SetOverscan(Settings.PS3OverscanEnabled, (float)Settings.PS3OverscanAmount/100);
							if(CellInput->IsButtonPressed(0,CTRL_LEFT) | CellInput->IsAnalogPressedLeft(0,CTRL_LSTICK))
							{
								sys_timer_usleep(SETTINGS_DELAY);
							}
						}
						if(Settings.PS3OverscanAmount == 0)
						{
							Settings.PS3OverscanEnabled = false;
							Graphics->SetOverscan(Settings.PS3OverscanEnabled, (float)Settings.PS3OverscanAmount/100);
							if(CellInput->IsButtonPressed(0,CTRL_LEFT) | CellInput->IsAnalogPressedLeft(0,CTRL_LSTICK))
							{
								sys_timer_usleep(SETTINGS_DELAY);
							}
						}
					}
					if(CellInput->WasButtonHeld(0, CTRL_RIGHT) | CellInput->IsAnalogPressedRight(0,CTRL_LSTICK) | CellInput->WasButtonPressed(0,CTRL_CROSS))
					{
						if((Settings.PS3OverscanAmount < 40))
						{
							Settings.PS3OverscanAmount++;
							Settings.PS3OverscanEnabled = true;
							Graphics->SetOverscan(Settings.PS3OverscanEnabled, (float)Settings.PS3OverscanAmount/100);
							if(CellInput->IsButtonPressed(0,CTRL_RIGHT) | CellInput->IsAnalogPressedRight(0,CTRL_LSTICK))
							{
								sys_timer_usleep(SETTINGS_DELAY);
							}
						}
						if(Settings.PS3OverscanAmount == 0)
						{
							Settings.PS3OverscanEnabled = false;
							Graphics->SetOverscan(Settings.PS3OverscanEnabled, (float)Settings.PS3OverscanAmount/100);
						}
							if(CellInput->IsButtonPressed(0,CTRL_RIGHT) | CellInput->IsAnalogPressedRight(0,CTRL_LSTICK))
							{
								sys_timer_usleep(SETTINGS_DELAY);
							}
					}
					if(CellInput->IsButtonPressed(0, CTRL_START))
					{
						Settings.PS3OverscanAmount = 0;
						Settings.PS3OverscanEnabled = false;
						Graphics->SetOverscan(Settings.PS3OverscanEnabled, (float)Settings.PS3OverscanAmount/100);
					}
					break;
				case SETTING_RSOUND_ENABLED:
					if(CellInput->WasButtonPressed(0, CTRL_LEFT) | CellInput->WasAnalogPressedLeft(0,CTRL_LSTICK) | CellInput->WasButtonPressed(0, CTRL_RIGHT) | CellInput->WasAnalogPressedRight(0,CTRL_LSTICK) | CellInput->WasButtonPressed(0,CTRL_CROSS))
					{
						Settings.RSoundEnabled = !Settings.RSoundEnabled;
						Emulator_ToggleSound();
					}
					if(CellInput->IsButtonPressed(0, CTRL_START))
					{
						Settings.RSoundEnabled = false;
						Emulator_ToggleSound();
					}
					break;
				case SETTING_RSOUND_SERVER_IP_ADDRESS:
					if(CellInput->WasButtonPressed(0, CTRL_LEFT) | CellInput->WasAnalogPressedLeft(0,CTRL_LSTICK) | CellInput->WasButtonPressed(0, CTRL_RIGHT) | CellInput->WasButtonPressed(0, CTRL_CROSS) | CellInput->WasAnalogPressedRight(0,CTRL_LSTICK))
					{
						Emulator_OSKStart(L"Enter the IP address for the RSound Server. Example (below):", L"192.168.1.1");
						Settings.RSoundServerIPAddress = Emulator_OSKOutputString();
					}
					if(CellInput->IsButtonPressed(0, CTRL_START))
					{
						Settings.RSoundServerIPAddress = "0.0.0.0";
					}
					break;
				case SETTING_DEFAULT_ALL:
					if(CellInput->WasButtonPressed(0, CTRL_LEFT) | CellInput->WasAnalogPressedLeft(0,CTRL_LSTICK) | CellInput->WasButtonPressed(0, CTRL_RIGHT) | CellInput->WasAnalogPressedRight(0,CTRL_LSTICK) | CellInput->IsButtonPressed(0, CTRL_START) | CellInput->WasButtonPressed(0, CTRL_CROSS))
					{
						Settings.PS3KeepAspect = true;
						Settings.PS3Smooth = true;
						Graphics->SetAspectRatio(Settings.PS3KeepAspect);
						Graphics->SetSmooth(Settings.PS3Smooth);
						Settings.PS3PALTemporalMode60Hz = false;
						Graphics->SetPAL60Hz(Settings.PS3PALTemporalMode60Hz);
						Graphics->LoadFragmentShader(DEFAULT_SHADER_FILE);
						Settings.PS3FontSize= 100;
						Settings.PS3OverscanAmount = 0;
						Settings.PS3OverscanEnabled = false;
						Graphics->SetOverscan(Settings.PS3OverscanEnabled, (float)Settings.PS3OverscanAmount/100);
						Settings.RSoundServerIPAddress = "0.0.0.0";
						if(Settings.RSoundEnabled)
						{
							Settings.RSoundEnabled = false;
							Emulator_ToggleSound();
						}
					}
					break;
				default:
					break;
			} // end of switch
	}

	float yPos = 0.09;
	float ySpacing = 0.04;

	cellDbgFontPuts		(0.09f,	0.05f,	Emulator_GetFontSize(),	RED,	"GENERAL");
	cellDbgFontPuts		(0.27f,	0.05f,	Emulator_GetFontSize(),	GREEN,	"GENESIS PLUS");
	cellDbgFontPuts		(0.45f,	0.05f,	Emulator_GetFontSize(),	GREEN,	"PATHS");
	cellDbgFontPuts		(0.63f, 0.05f,	Emulator_GetFontSize(), GREEN,	"CONTROLS"); 
	Graphics->FlushDbgFont();

	cellDbgFontPuts(0.09f, yPos, Emulator_GetFontSize(), currently_selected_setting == SETTING_CHANGE_RESOLUTION ? YELLOW : WHITE, "Resolution");

	switch(Graphics->GetCurrentResolution())
	{
		case CELL_VIDEO_OUT_RESOLUTION_480:
			cellDbgFontPrintf(0.5f, yPos, Emulator_GetFontSize(), Graphics->GetInitialResolution() == CELL_VIDEO_OUT_RESOLUTION_480 ? GREEN : ORANGE, "720x480 (480p)");
			break;
		case CELL_VIDEO_OUT_RESOLUTION_720:
			cellDbgFontPrintf(0.5f, yPos, Emulator_GetFontSize(), Graphics->GetInitialResolution() == CELL_VIDEO_OUT_RESOLUTION_720 ? GREEN : ORANGE, "1280x720 (720p)");
			break;
		case CELL_VIDEO_OUT_RESOLUTION_1080:
			cellDbgFontPrintf(0.5f, yPos, Emulator_GetFontSize(), Graphics->GetInitialResolution() == CELL_VIDEO_OUT_RESOLUTION_1080 ? GREEN : ORANGE, "1920x1080 (1080p)");
			break;
		case CELL_VIDEO_OUT_RESOLUTION_576:
			cellDbgFontPrintf(0.5f, yPos, Emulator_GetFontSize(), Graphics->GetInitialResolution() == CELL_VIDEO_OUT_RESOLUTION_576 ? GREEN : ORANGE, "720x576 (576p)");
			break;
		case CELL_VIDEO_OUT_RESOLUTION_1600x1080:
			cellDbgFontPrintf(0.5f, yPos, Emulator_GetFontSize(), Graphics->GetInitialResolution() == CELL_VIDEO_OUT_RESOLUTION_1600x1080 ? GREEN : ORANGE, "1600x1080");
			break;
		case CELL_VIDEO_OUT_RESOLUTION_1440x1080:
			cellDbgFontPrintf(0.5f, yPos, Emulator_GetFontSize(), Graphics->GetInitialResolution() == CELL_VIDEO_OUT_RESOLUTION_1440x1080 ? GREEN : ORANGE, "1440x1080");
			break;
		case CELL_VIDEO_OUT_RESOLUTION_1280x1080:
			cellDbgFontPrintf(0.5f, yPos, Emulator_GetFontSize(), Graphics->GetInitialResolution() == CELL_VIDEO_OUT_RESOLUTION_1280x1080 ? GREEN : ORANGE, "1280x1080");
			break;
		case CELL_VIDEO_OUT_RESOLUTION_960x1080:
			cellDbgFontPrintf(0.5f, yPos, Emulator_GetFontSize(), Graphics->GetInitialResolution() == CELL_VIDEO_OUT_RESOLUTION_960x1080 ? GREEN : ORANGE, "960x1080");
			break;
	}
	Graphics->FlushDbgFont();

/*
	yPos += ySpacing;
	cellDbgFontPuts		(0.09f,	yPos,	Emulator_GetFontSize(),	currently_selected_setting == SETTING_PAL60_MODE ? YELLOW : WHITE,	"PAL60 Mode (576p only)");
	cellDbgFontPrintf	(0.5f,	yPos,	Emulator_GetFontSize(),	Settings.PS3PALTemporalMode60Hz == true ? ORANGE : GREEN, Settings.PS3PALTemporalMode60Hz == true ? "ON" : "OFF");
*/

	yPos += ySpacing;
	cellDbgFontPuts(0.09f, yPos, Emulator_GetFontSize(), currently_selected_setting == SETTING_SHADER ? YELLOW : WHITE, "Selected shader");
	cellDbgFontPrintf(0.5f, yPos, Emulator_GetFontSize(), 
			GREEN, 
			"%s", Graphics->GetFragmentShaderPath().substr(Graphics->GetFragmentShaderPath().find_last_of('/')).c_str());

	yPos += ySpacing;
	cellDbgFontPuts(0.09f, yPos, Emulator_GetFontSize(), currently_selected_setting == SETTING_FONT_SIZE ? YELLOW : WHITE, "Font size");
	cellDbgFontPrintf(0.5f,	yPos,	Emulator_GetFontSize(),	Settings.PS3FontSize == 100 ? GREEN : ORANGE, "%f", Emulator_GetFontSize());

	yPos += ySpacing;
	cellDbgFontPuts(0.09f, yPos, Emulator_GetFontSize(), currently_selected_setting == SETTING_KEEP_ASPECT_RATIO ? YELLOW : WHITE, "Aspect Ratio");
	cellDbgFontPrintf(0.5f, yPos, Emulator_GetFontSize(), Settings.PS3KeepAspect == true ? GREEN : ORANGE, "%s", Settings.PS3KeepAspect == true ? "Scaled" : "Stretched");
	Graphics->FlushDbgFont();

	yPos += ySpacing;
	cellDbgFontPuts(0.09f, yPos, Emulator_GetFontSize(), currently_selected_setting == SETTING_HW_TEXTURE_FILTER ? YELLOW : WHITE, "Hardware Filtering");
	cellDbgFontPrintf(0.5f, yPos, Emulator_GetFontSize(), Settings.PS3Smooth == true ? GREEN : ORANGE,
			"%s", Settings.PS3Smooth == true ? "Linear interpolation" : "Point filtering");

	yPos += ySpacing;
	cellDbgFontPuts		(0.09f,	yPos,	Emulator_GetFontSize(),	currently_selected_setting == SETTING_HW_OVERSCAN_AMOUNT ? YELLOW : WHITE,	"Overscan");
	cellDbgFontPrintf	(0.5f,	yPos,	Emulator_GetFontSize(),	Settings.PS3OverscanAmount == 0 ? GREEN : ORANGE, "%f", (float)Settings.PS3OverscanAmount/100);

	yPos += ySpacing;
	cellDbgFontPuts(0.09f, yPos, Emulator_GetFontSize(), currently_selected_setting == SETTING_RSOUND_ENABLED ? YELLOW : WHITE, "Sound");
	cellDbgFontPuts(0.5f, yPos, Emulator_GetFontSize(), Settings.RSoundEnabled == false ? GREEN : ORANGE, Settings.RSoundEnabled == true ? "RSound" : "Normal");

	yPos += ySpacing;
	cellDbgFontPuts(0.09f, yPos, Emulator_GetFontSize(), currently_selected_setting == SETTING_RSOUND_SERVER_IP_ADDRESS ? YELLOW : WHITE, "RSound Server IP Address");
	cellDbgFontPuts(0.5f, yPos, Emulator_GetFontSize(), strcmp(Settings.RSoundServerIPAddress,"0.0.0.0") ? ORANGE : GREEN, Settings.RSoundServerIPAddress);

	yPos += ySpacing;
	cellDbgFontPrintf(0.09f, yPos, Emulator_GetFontSize(), currently_selected_setting == SETTING_DEFAULT_ALL ? YELLOW : GREEN, "DEFAULT");
	Graphics->FlushDbgFont();

	DisplayHelpMessage(currently_selected_setting);

	cellDbgFontPuts(0.09f, 0.89f, Emulator_GetFontSize(), YELLOW,
	"UP/DOWN - select  L2+R2 - resume game   X/LEFT/RIGHT - change");
	cellDbgFontPuts(0.09f, 0.93f, Emulator_GetFontSize(), YELLOW,
	"START - default   L1/CIRCLE - go back   R1 - go forward");
	Graphics->FlushDbgFont();
}



void do_ROMMenu()
{
	string rom_path;

	if (CellInput->UpdateDevice(0) == CELL_PAD_OK)
	{
		UpdateBrowser(browser);

		if (CellInput->WasButtonPressed(0,CTRL_CROSS))
		{
			if(browser->IsCurrentADirectory())
			{
				browser->PushDirectory( browser->GetCurrentDirectoryInfo().dir + "/" + browser->GetCurrentEntry()->d_name,
						CELL_FS_TYPE_REGULAR | CELL_FS_TYPE_DIRECTORY,
						"md|smd|bin|gen|zip|MD|SMD|bin|GEN|ZIP");
			}
			else if (browser->IsCurrentAFile())
			{
				// load game (standard controls), go back to main loop
				rom_path = browser->GetCurrentDirectoryInfo().dir + "/" + browser->GetCurrentEntry()->d_name;

				MenuStop();
				Emulator_StartROMRunning();

				//FIXME: 1x dirty const char* to char* casts... menu sucks.
				Emulator_RequestLoadROM((char*)rom_path.c_str(), true);

				return;
			}
		}
		if (CellInput->WasButtonPressed(0,CTRL_SELECT))
		{
			menuStack.push(do_general_settings);
			tmpBrowser = NULL;
		}
		if (CellInput->IsButtonPressed(0,CTRL_L2) && CellInput->IsButtonPressed(0,CTRL_R2))
		{
			// if a rom is loaded then resume it
			if (Emulator_IsROMLoaded())
			{
				MenuStop();

				Emulator_StartROMRunning();
			}
		}
	}

	if(browser->IsCurrentADirectory())
	{
		if(!strcmp(browser->GetCurrentEntry()->d_name,"app_home") || !strcmp(browser->GetCurrentEntry()->d_name,"host_root"))
		{
			cellDbgFontPrintf(0.09f, 0.90f, 0.91f, RED, "%s","WARNING - Do not open this directory, or you might have to restart!");
		}
		else if(!strcmp(browser->GetCurrentEntry()->d_name,".."))
		{
			cellDbgFontPrintf(0.09f, 0.90f, 0.91f, LIGHTBLUE, "%s","INFO - Press X to go back to the previous directory.");
		}
		else
		{
			cellDbgFontPrintf(0.09f, 0.90f, 0.91f, LIGHTBLUE, "%s","INFO - Press X to enter the directory.");
		}
	}
	if(browser->IsCurrentAFile())
	{
		cellDbgFontPrintf(0.09f, 0.90f, 0.91f, LIGHTBLUE, "%s", "INFO - Press X to load the game. ");
	}
	cellDbgFontPuts	(0.09f,	0.05f,	Emulator_GetFontSize(),	RED,	"FILE BROWSER");
	cellDbgFontPrintf(0.7f, 0.05f, 0.82f, WHITE, "Genesis Plus GX PS3 v%s", EMULATOR_VERSION);
	cellDbgFontPrintf(0.09f, 0.09f, Emulator_GetFontSize(), YELLOW, "PATH: %s", browser->GetCurrentDirectoryInfo().dir.c_str());
	cellDbgFontPuts(0.09f, 0.93f, Emulator_GetFontSize(), YELLOW,
	"L2 + R2 - resume game           SELECT - Settings screen");
	Graphics->FlushDbgFont();

	RenderBrowser(browser);
}

void MenuMainLoop()
{
	// create file browser->if null
	if (browser == NULL)
	{
		browser = new FileBrowser(Settings.PS3PathROMDirectory);
		browser->SetEntryWrap(false);
	}


	// FIXME: could always just return to last menu item... don't pop on resume kinda thing
	if (menuStack.empty())
	{
		menuStack.push(do_ROMMenu);
	}

	// menu loop
	menuRunning = true;
	while (!menuStack.empty() && menuRunning)
	{
		Graphics->Clear();

		menuStack.top()();

		Graphics->Swap();

		cellSysutilCheckCallback();
	}
}

