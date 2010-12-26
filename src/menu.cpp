/*
 * menu.cpp
 *
 *  Created on: Oct 10, 2010
 *      Author: halsafar
 */
#include <string.h>
#include <stack>
#include <vector>

#include "ps3video.h"
#include "menu.h"
#include "cellframework/input/cellInput.h"
#include "cellframework/fileio/FileBrowser.h"
#include "cellframework/logger/Logger.h"
#include <cell/pad.h>

#include <cell/audio.h>
#include <cell/sysmodule.h>
#include <cell/cell_fs.h>
#include <cell/dbgfont.h>
#include <sysutil/sysutil_sysparam.h>

#include "GenesisPlus.h"

#include "conf/conffile.h"

#define MIN(x,y) ((x) < (y) ? (x) : (y))
#define MAX(x,y) ((x) > (y) ? (x) : (y))

#define NUM_ENTRY_PER_PAGE 24

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

int currently_selected_setting = 0;
int currently_selected_genesis_setting = 50;
int currently_selected_path_setting = 100;

void MenuStop()
{
	menuRunning = false;

}

float FontSize()
{
	return Settings.PS3FontSize/100.0;
}

bool MenuIsRunning()
{
	return menuRunning;
}

void UpdateBrowser(FileBrowser* b)
{
	if (CellInput->WasButtonPressed(0,CTRL_DOWN) | CellInput->IsAnalogPressedDown(0,CTRL_LSTICK))
	{
		b->IncrementEntry();
	}
	if (CellInput->WasButtonPressed(0,CTRL_UP) | CellInput->IsAnalogPressedUp(0,CTRL_LSTICK))
	{
		b->DecrementEntry();
	}
	if (CellInput->WasButtonPressed(0,CTRL_RIGHT) | CellInput->IsAnalogPressedRight(0,CTRL_LSTICK))
	{
		b->GotoEntry(MIN(b->GetCurrentEntryIndex()+5, b->GetCurrentDirectoryInfo().numEntries-1));
	}
	if (CellInput->WasButtonPressed(0, CTRL_LEFT) | CellInput->IsAnalogPressedLeft(0,CTRL_LSTICK))
	{
		if (b->GetCurrentEntryIndex() <= 5)
		{
			b->GotoEntry(0);
		}
		else
		{
			b->GotoEntry(b->GetCurrentEntryIndex()-5);
		}
	}
	if (CellInput->WasButtonPressed(0,CTRL_R1))
	{
		b->GotoEntry(MIN(b->GetCurrentEntryIndex()+NUM_ENTRY_PER_PAGE, b->GetCurrentDirectoryInfo().numEntries-1));
	}
	if (CellInput->WasButtonPressed(0,CTRL_L1))
	{
		if (b->GetCurrentEntryIndex() <= NUM_ENTRY_PER_PAGE)
		{
			b->GotoEntry(0);
		}
		else
		{
			b->GotoEntry(b->GetCurrentEntryIndex()-NUM_ENTRY_PER_PAGE);
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
		cellDbgFontPuts		(0.09f,	0.05f,	FontSize(),	RED,	"No Roms founds!!!\n");
	}
	else if (current_index > file_count)
	{
		printf("2: current_index >= file_count");
	}
	else
	{
		int page_number = current_index / NUM_ENTRY_PER_PAGE;
		int page_base = page_number * NUM_ENTRY_PER_PAGE;
		float currentX = 0.05f;
		float currentY = 0.00f;
		float ySpacing = 0.035f;
		for (int i = page_base; i < file_count && i < page_base + NUM_ENTRY_PER_PAGE; ++i)
		{
			currentY = currentY + ySpacing;
			cellDbgFontPuts(currentX, currentY, FontSize(),
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

	cellDbgFontPuts(0.05f, 0.88f, FontSize(), YELLOW, "X - enter directory/select ExtraCart ROM");
	cellDbgFontPuts(0.05f, 0.92f, FontSize(), PURPLE, "Triangle - return to settings");
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

	cellDbgFontPuts(0.09f, 0.88f, FontSize(), YELLOW, "X - Select shader");
	cellDbgFontPuts(0.09f, 0.92f, FontSize(), PURPLE, "Triangle - return to settings");
	Graphics->FlushDbgFont();

	RenderBrowser(tmpBrowser);
}

void DisplayHelpMessage(int currentsetting)
{
	switch(currentsetting)
	{
		case SETTING_PATH_SAVESTATES_DIRECTORY:
			cellDbgFontPrintf(0.09f, 0.80f, FontSize(), WHITE, "%s", "Set the default path where all the savestate files will be saved.\n");
			break;
		case SETTING_PATH_DEFAULT_ROM_DIRECTORY:
			cellDbgFontPrintf(0.09f, 0.80f, FontSize(), WHITE, "%s", "Set the default ROM directory. Upon restarting this application,the\nemulator will use this as the default startup directory.\n");
			break;
		case SETTING_PATH_SRAM_DIRECTORY:
			cellDbgFontPrintf(0.09f, 0.80f, FontSize(), WHITE, "%s", "Set the default SRAM (SaveRAM) directory path.\n");
			break;
		case SETTING_SHADER:
			cellDbgFontPrintf(0.09f, 0.80f, FontSize(), WHITE, "%s", "Select a pixel shader. NOTE: Some shaders might be too slow at 1080p.\nIf you experience any slowdown, try another shader.");
			break;
		case SETTING_GENESIS_FPS:
			cellDbgFontPrintf(0.09f, 0.80f, FontSize(), WHITE, "%s", Settings.DrawFps ? "Show Framerate is set to 'Enabled' - show an FPS counter onscreen.\nNOTE: Not working yet." : "Show Framerate is set to 'Disabled'.");
			break;
		case SETTING_GENESIS_PAD:
			cellDbgFontPrintf(0.09f, 0.80f, FontSize(), WHITE, "%s", Settings.SixButtonPad ? "Controller type is set to '6 buttons' - certain games will now have\n6-button pad support." : "Controller type is set to '3 buttons'.");
			break;
		case SETTING_CHANGE_RESOLUTION:
			cellDbgFontPrintf(0.09f, 0.80f, FontSize(), WHITE, "%s", "Change the display resolution - press X to confirm.\nNOTE: This setting might not work correctly on 1.92 FW.");
			break;
		case SETTING_PAL60_MODE:
			cellDbgFontPrintf(0.09f, 0.80f, FontSize(), WHITE, "%s", Settings.PS3PALTemporalMode60Hz ? "PAL 60Hz mode is enabled - 60Hz NTSC games will run correctly at 576p PAL\nresolution. NOTE: This is configured on-the-fly." : "PAL 60Hz mode disabled - 50Hz PAL games will run correctly at 576p PAL\nresolution. NOTE: This is configured on-the-fly.");
			break;
		case SETTING_HW_TEXTURE_FILTER:
			cellDbgFontPrintf(0.09f, 0.80f, FontSize(), WHITE, "%s", Settings.PS3Smooth ? "Hardware filtering is set to 'Bilinear filtering'." : "Hardware filtering is set to 'Point filtering' - most shaders\nlook much better on this setting.");
			break;
		case SETTING_KEEP_ASPECT_RATIO:
			cellDbgFontPrintf(0.09f, 0.80f, FontSize(), WHITE, "%s", Settings.PS3KeepAspect ? "Aspect ratio is set to 'Scaled' - screen will have black borders\nleft and right on widescreen TVs/monitors." : "Aspect ratio is set to 'Stretched' - widescreen TVs/monitors will have\na full stretched picture.");
			break;
		case SETTING_RSOUND_ENABLED:
			cellDbgFontPrintf(0.09f, 0.80f, FontSize(), WHITE, "%s", Settings.RSoundEnabled ? "Sound is set to 'RSound' - the sound will be streamed over the network\nto the RSound audio server." : "Sound is set to 'Normal' - normal audio output will be used.");
			break;
		case SETTING_RSOUND_SERVER_IP_ADDRESS:
			cellDbgFontPuts(0.09f, 0.80f, FontSize(), WHITE, "Enter the IP address of the RSound audio server. IP address must be\nan IPv4 32-bits address, eg: '192.168.0.7'.");
			break;
		case SETTING_FONT_SIZE:
			cellDbgFontPuts(0.09f, 0.80f, FontSize(), WHITE, "Increase or decrease the font size in the menu.");
			break;
		case SETTING_HW_OVERSCAN_AMOUNT:
			cellDbgFontPuts(0.09f, 0.80f, FontSize(), WHITE, "Set this to higher than 0.000 if the screen doesn't fit on your\n(HD)TV/monitor.");
			break;
		case SETTING_DEFAULT_ALL:
			cellDbgFontPuts(0.09f, 0.80f, FontSize(), WHITE, "Set all 'General' settings back to their default values.");
			break;
		case SETTING_GENESIS_DEFAULT_ALL:
			cellDbgFontPuts(0.09f, 0.80f, FontSize(), WHITE, "Set all 'Genesis' settings back to their default values.");
			break;
		case SETTING_PATH_DEFAULT_ALL:
			cellDbgFontPuts(0.09f, 0.80f, FontSize(), WHITE, "Set all 'Path' settings back to their default values.");
			break;
		case SETTING_GENESIS_EXCART:
			cellDbgFontPrintf(0.09f, 0.80f, FontSize(), WHITE, "%s", Settings.ExtraCart ? "Extra Cartridge is set to 'Enabled'. NOTE: this will not do anything\nif you have not specified the path to the lock-on ROM carts." : "Extra Cartridge is set to 'Disabled'.");
			break;
		case SETTING_GENESIS_ACTIONREPLAY_ROMPATH:
			cellDbgFontPuts(0.09f, 0.80f, FontSize(), WHITE, "To enable Action Replay functionality, you need to have the file\nareplay.bin somewhere on your HDD/USB device. Select the file here.");
			break;
		case SETTING_GENESIS_GAMEGENIE_ROMPATH:
			cellDbgFontPuts(0.09f, 0.80f, FontSize(), WHITE, "To enable Game Genie functionality, you need to have the file ggenie.bin\nsomewhere on your HDD/USB device. Select the file here.");
			break;
		case SETTING_GENESIS_SK_ROMPATH:
			cellDbgFontPuts(0.09f, 0.80f, FontSize(), WHITE, "To enable S&K lock-on functionality, you need to have the file sk.bin\nsomewhere on your HDD/USB device. Select the file here.");
			break;
		case SETTING_GENESIS_SK_UPMEM_ROMPATH:
			cellDbgFontPuts(0.09f, 0.80f, FontSize(), WHITE, "To enable S&KUPMEM lock-on functionality, you need to have the file\nsk2chip.bin somewhere on your HDD/USB device. Select the file here.");
			break;
		case SETTING_GENESIS_BIOS_ROMPATH:
			cellDbgFontPuts(0.09f, 0.80f, FontSize(), WHITE, "For increased compatibility, you need to load with the Genesis BIOS file.\nSelect the file (bios.bin) here.");
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
			
	cellDbgFontPuts(0.09f, 0.88f, FontSize(), YELLOW, "X - enter directory");
	cellDbgFontPuts(0.09f, 0.92f, FontSize(), BLUE, "SQUARE - select directory as path");
	cellDbgFontPuts(0.09f, 0.96f, FontSize(), PURPLE, "Triangle - return to settings");
	Graphics->FlushDbgFont();
			
	RenderBrowser(tmpBrowser);
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

			if (CellInput->WasButtonPressed(0, CTRL_DOWN) | CellInput->WasAnalogPressedDown(0, CTRL_LSTICK))	// down to next setting
			{
				currently_selected_path_setting++;
				if (currently_selected_path_setting >= MAX_NO_OF_PATH_SETTINGS)
				{
					currently_selected_path_setting = 100;
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

			if (CellInput->WasButtonPressed(0, CTRL_UP) | CellInput->WasAnalogPressedUp(0, CTRL_LSTICK))	// up to previous setting
			{
					currently_selected_path_setting--;
					if (currently_selected_path_setting < 100)
					{
						currently_selected_path_setting = MAX_NO_OF_PATH_SETTINGS-1;
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

	cellDbgFontPuts		(0.09f,	0.05f,	FontSize(),	GREEN,	"GENERAL");
	cellDbgFontPuts		(0.25f,	0.05f,	FontSize(),	GREEN,	"GENESIS PLUS");
	cellDbgFontPuts		(0.45f,	0.05f,	FontSize(),	RED,	"PATHS");
	Graphics->FlushDbgFont();

	cellDbgFontPuts		(0.09f,	yPos,	FontSize(),	currently_selected_path_setting == SETTING_PATH_DEFAULT_ROM_DIRECTORY ? YELLOW : WHITE,	"Startup ROM Directory");
	cellDbgFontPuts		(0.5f,	yPos,	FontSize(),	Settings.PS3PathROMDirectory.c_str() == "/" ? GREEN : ORANGE, Settings.PS3PathROMDirectory.c_str());

	yPos += ySpacing;
	cellDbgFontPuts		(0.09f,	yPos,	FontSize(),	currently_selected_path_setting == SETTING_PATH_SAVESTATES_DIRECTORY ? YELLOW : WHITE,	"Savestate Directory");
	cellDbgFontPuts		(0.5f,	yPos,	FontSize(),	Settings.PS3PathSaveStates.c_str() == USRDIR ? GREEN : ORANGE, Settings.PS3PathSaveStates.c_str());
	Graphics->FlushDbgFont();

	yPos += ySpacing;
	cellDbgFontPuts		(0.09f,	yPos,	FontSize(),	currently_selected_path_setting == SETTING_PATH_SRAM_DIRECTORY ? YELLOW : WHITE,	"SRAM directory");
	cellDbgFontPuts		(0.5f,	yPos,	FontSize(),	Settings.PS3PathSRAM.c_str() == USRDIR ? GREEN : ORANGE, Settings.PS3PathSRAM.c_str());

	yPos += ySpacing;
	cellDbgFontPrintf(0.09f, yPos, FontSize(), currently_selected_path_setting == SETTING_PATH_DEFAULT_ALL ? YELLOW : GREEN, "DEFAULT");

	DisplayHelpMessage(currently_selected_path_setting);

	cellDbgFontPuts(0.09f, 0.88f, FontSize(), YELLOW, "UP/DOWN - select,  X/LEFT/RIGHT - change,  START - default");
	cellDbgFontPuts(0.09f, 0.92f, FontSize(), YELLOW, "L1/CIRCLE - go back, L2+R2 - resume game");
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

			if (CellInput->WasButtonPressed(0, CTRL_DOWN) | CellInput->WasAnalogPressedDown(0, CTRL_LSTICK))	// down to next setting
			{
				currently_selected_genesis_setting++;
				if (currently_selected_genesis_setting >= MAX_NO_OF_GENESIS_SETTINGS)
				{
					currently_selected_genesis_setting = 50;
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

			if (CellInput->WasButtonPressed(0, CTRL_UP) | CellInput->WasAnalogPressedUp(0, CTRL_LSTICK))	// up to previous setting
			{
					currently_selected_genesis_setting--;
					if (currently_selected_genesis_setting < 50)
					{
						currently_selected_genesis_setting = MAX_NO_OF_GENESIS_SETTINGS-1;
					}
			}
					switch(currently_selected_genesis_setting)
					{
				case SETTING_GENESIS_FPS:
					if(CellInput->WasButtonPressed(0, CTRL_LEFT) | CellInput->WasAnalogPressedLeft(0, CTRL_LSTICK) | CellInput->WasButtonPressed(0, CTRL_RIGHT) | CellInput->WasAnalogPressedRight(0, CTRL_LSTICK) | CellInput->WasButtonPressed(0, CTRL_CROSS))
					{
						Settings.DrawFps = !Settings.DrawFps;
					}
					if(CellInput->IsButtonPressed(0, CTRL_START))
					{
						Settings.DrawFps = false;
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
						Settings.DrawFps = false;
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

	cellDbgFontPuts		(0.09f,	0.05f,	FontSize(),	GREEN,	"GENERAL");
	cellDbgFontPuts		(0.25f,	0.05f,	FontSize(),	RED,	"GENESIS PLUS");
	cellDbgFontPuts		(0.45f,	0.05f,	FontSize(),	GREEN,	"PATHS");
	Graphics->FlushDbgFont();

	cellDbgFontPuts(0.09f, yPos, FontSize(), currently_selected_genesis_setting == SETTING_GENESIS_FPS ? YELLOW : WHITE, "Show Framerate");
	cellDbgFontPrintf(0.5f, yPos, FontSize(), Settings.DrawFps == true ? ORANGE : GREEN,
			"%s", Settings.DrawFps == true ? "Enabled" : "Disabled");
	
	yPos += ySpacing;
	cellDbgFontPuts(0.09f, yPos, FontSize(), currently_selected_genesis_setting == SETTING_GENESIS_PAD ? YELLOW : WHITE, "Gamepad type");
	cellDbgFontPrintf(0.5f, yPos, FontSize(), Settings.SixButtonPad == true ? ORANGE : GREEN,
			"%s", Settings.SixButtonPad == true ? "6 buttons" : "3 buttons");

	yPos += ySpacing;
	cellDbgFontPuts(0.09f, yPos, FontSize(), currently_selected_genesis_setting == SETTING_GENESIS_EXCART ? YELLOW : WHITE, "Extra Cartridge");
	cellDbgFontPrintf(0.5f, yPos, FontSize(), Settings.ExtraCart == true ? ORANGE : GREEN,
			"%s", Settings.ExtraCart == true ? "ON" : "OFF");

	yPos += ySpacing;
	cellDbgFontPuts(0.09f, yPos, FontSize(), currently_selected_genesis_setting == SETTING_GENESIS_ACTIONREPLAY_ROMPATH ? YELLOW : WHITE, "Action Replay ROM Path");
	cellDbgFontPrintf(0.5f, yPos, FontSize(),Settings.ActionReplayROMPath.c_str() == "/dev_hdd0/game/GENP00001/USRDIR/areplay.bin" ? GREEN : ORANGE, Settings.ActionReplayROMPath.c_str());

	yPos += ySpacing;
	cellDbgFontPuts(0.09f, yPos, FontSize(), currently_selected_genesis_setting == SETTING_GENESIS_GAMEGENIE_ROMPATH ? YELLOW : WHITE, "Game Genie ROM Path");
	cellDbgFontPrintf(0.5f, yPos, FontSize(),Settings.GameGenieROMPath.c_str() == "/dev_hdd0/game/GENP00001/USRDIR/ggenie.bin" ? GREEN : ORANGE, Settings.GameGenieROMPath.c_str());

	yPos += ySpacing;
	cellDbgFontPuts(0.09f, yPos, FontSize(), currently_selected_genesis_setting == SETTING_GENESIS_SK_ROMPATH ? YELLOW : WHITE, "Sonic & Knuckles ROM Path");
	cellDbgFontPrintf(0.5f, yPos, FontSize(),Settings.SKROMPath.c_str() == "/dev_hdd0/game/GENP00001/USRDIR/sk.bin" ? GREEN : ORANGE, Settings.SKROMPath.c_str());

	yPos += ySpacing;
	cellDbgFontPuts(0.09f, yPos, FontSize(), currently_selected_genesis_setting == SETTING_GENESIS_SK_UPMEM_ROMPATH ? YELLOW : WHITE, "S&K Upmem ROM Path");
	cellDbgFontPrintf(0.5f, yPos, FontSize(),Settings.SKUpmemROMPath.c_str() == "/dev_hdd0/game/GENP00001/USRDIR/sk2chip.bin" ? GREEN : ORANGE, Settings.SKUpmemROMPath.c_str());

	yPos += ySpacing;
	cellDbgFontPuts(0.09f, yPos, FontSize(), currently_selected_genesis_setting == SETTING_GENESIS_BIOS_ROMPATH ? YELLOW : WHITE, "BIOS ROM Path");
	cellDbgFontPrintf(0.5f, yPos, FontSize(),Settings.BIOS.c_str() == "/dev_hdd0/game/GENP00001/USRDIR/bios.bin" ? GREEN : ORANGE, Settings.BIOS.c_str());

	yPos += ySpacing;
	cellDbgFontPrintf(0.09f, yPos, FontSize(), currently_selected_genesis_setting == SETTING_GENESIS_DEFAULT_ALL ? YELLOW : GREEN, "DEFAULT");
	Graphics->FlushDbgFont();

	DisplayHelpMessage(currently_selected_genesis_setting);

	cellDbgFontPuts(0.09f, 0.88f, FontSize(), YELLOW, "UP/DOWN - select,  X/LEFT/RIGHT - change,  START - default");
	cellDbgFontPuts(0.09f, 0.92f, FontSize(), YELLOW, "L1/CIRCLE - go back, L2+R2 - resume game");
	Graphics->FlushDbgFont();
}

void do_general_settings()
{
	if(CellInput->UpdateDevice(0) == CELL_PAD_OK)
	{
			// back to ROM menu if CIRCLE is pressed
			if (CellInput->WasButtonPressed(0, CTRL_CIRCLE))
			{
				menuStack.pop();
				return;
			}

			if (CellInput->WasButtonPressed(0, CTRL_R1))
			{
				menuStack.push(do_genesis_settings);
				return;
			}

			if (CellInput->WasButtonPressed(0, CTRL_DOWN) | CellInput->WasAnalogPressedDown(0, CTRL_LSTICK))	// down to next setting
			{
				currently_selected_setting++;
				if (currently_selected_setting >= MAX_NO_OF_SETTINGS)
				{
					currently_selected_setting = 0;
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

			if (CellInput->WasButtonPressed(0, CTRL_UP) | CellInput->WasAnalogPressedUp(0, CTRL_LSTICK))	// up to previous setting
			{
					currently_selected_setting--;
					if (currently_selected_setting < 0)
					{
						currently_selected_setting = MAX_NO_OF_SETTINGS-1;
					}
			}
					switch(currently_selected_setting)
					{
					case SETTING_CHANGE_RESOLUTION:
						   if(CellInput->WasButtonPressed(0, CTRL_RIGHT) | CellInput->WasAnalogPressedLeft(0,CTRL_LSTICK))
						   {
							   Graphics->NextResolution();
						   }
						   if(CellInput->WasButtonPressed(0, CTRL_LEFT) | CellInput->WasAnalogPressedLeft(0,CTRL_LSTICK))
						   {
							   Graphics->PreviousResolution();
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
					if(CellInput->WasButtonPressed(0, CTRL_LEFT) | CellInput->WasAnalogPressedLeft(0,CTRL_LSTICK) | CellInput->WasButtonPressed(0,CTRL_CROSS))
					{
						if(Settings.PS3FontSize > -100)
						{
							Settings.PS3FontSize--;
						}
					}
					if(CellInput->WasButtonPressed(0, CTRL_RIGHT) | CellInput->WasAnalogPressedRight(0,CTRL_LSTICK) | CellInput->WasButtonPressed(0,CTRL_CROSS))
					{
						if((Settings.PS3FontSize < 200))
						{
							Settings.PS3FontSize++;
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
					if(CellInput->WasButtonPressed(0, CTRL_LEFT) | CellInput->WasAnalogPressedLeft(0,CTRL_LSTICK) | CellInput->WasButtonPressed(0,CTRL_CROSS))
					{
						if(Settings.PS3OverscanAmount > -40)
						{
							Settings.PS3OverscanAmount--;
							Settings.PS3OverscanEnabled = true;
							Graphics->SetOverscan(Settings.PS3OverscanEnabled, (float)Settings.PS3OverscanAmount/100);
						}
						if(Settings.PS3OverscanAmount == 0)
						{
							Settings.PS3OverscanEnabled = false;
							Graphics->SetOverscan(Settings.PS3OverscanEnabled, (float)Settings.PS3OverscanAmount/100);
						}
					}
					if(CellInput->WasButtonPressed(0, CTRL_RIGHT) | CellInput->WasAnalogPressedRight(0,CTRL_LSTICK) | CellInput->WasButtonPressed(0,CTRL_CROSS))
					{
						if((Settings.PS3OverscanAmount < 40))
						{
							Settings.PS3OverscanAmount++;
							Settings.PS3OverscanEnabled = true;
							Graphics->SetOverscan(Settings.PS3OverscanEnabled, (float)Settings.PS3OverscanAmount/100);
						}
						if(Settings.PS3OverscanAmount == 0)
						{
							Settings.PS3OverscanEnabled = false;
							Graphics->SetOverscan(Settings.PS3OverscanEnabled, (float)Settings.PS3OverscanAmount/100);
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

	cellDbgFontPuts		(0.09f,	0.05f,	FontSize(),	RED,	"GENERAL");
	cellDbgFontPuts		(0.25f,	0.05f,	FontSize(),	GREEN,	"GENESIS PLUS");
	cellDbgFontPuts		(0.45f,	0.05f,	FontSize(),	GREEN,	"PATHS");
	Graphics->FlushDbgFont();

	cellDbgFontPuts(0.09f, yPos, FontSize(), currently_selected_setting == SETTING_CHANGE_RESOLUTION ? YELLOW : WHITE, "Resolution");

	switch(Graphics->GetCurrentResolution())
	{
		case CELL_VIDEO_OUT_RESOLUTION_480:
			cellDbgFontPrintf(0.5f, yPos, FontSize(), Graphics->GetInitialResolution() == CELL_VIDEO_OUT_RESOLUTION_480 ? GREEN : ORANGE, "720x480 (480p)");
			break;
		case CELL_VIDEO_OUT_RESOLUTION_720:
			cellDbgFontPrintf(0.5f, yPos, FontSize(), Graphics->GetInitialResolution() == CELL_VIDEO_OUT_RESOLUTION_720 ? GREEN : ORANGE, "1280x720 (720p)");
			break;
		case CELL_VIDEO_OUT_RESOLUTION_1080:
			cellDbgFontPrintf(0.5f, yPos, FontSize(), Graphics->GetInitialResolution() == CELL_VIDEO_OUT_RESOLUTION_1080 ? GREEN : ORANGE, "1920x1080 (1080p)");
			break;
		case CELL_VIDEO_OUT_RESOLUTION_576:
			cellDbgFontPrintf(0.5f, yPos, FontSize(), Graphics->GetInitialResolution() == CELL_VIDEO_OUT_RESOLUTION_576 ? GREEN : ORANGE, "720x576 (576p)");
			break;
		case CELL_VIDEO_OUT_RESOLUTION_1600x1080:
			cellDbgFontPrintf(0.5f, yPos, FontSize(), Graphics->GetInitialResolution() == CELL_VIDEO_OUT_RESOLUTION_1600x1080 ? GREEN : ORANGE, "1600x1080");
			break;
		case CELL_VIDEO_OUT_RESOLUTION_1440x1080:
			cellDbgFontPrintf(0.5f, yPos, FontSize(), Graphics->GetInitialResolution() == CELL_VIDEO_OUT_RESOLUTION_1440x1080 ? GREEN : ORANGE, "1440x1080");
			break;
		case CELL_VIDEO_OUT_RESOLUTION_1280x1080:
			cellDbgFontPrintf(0.5f, yPos, FontSize(), Graphics->GetInitialResolution() == CELL_VIDEO_OUT_RESOLUTION_1280x1080 ? GREEN : ORANGE, "1280x1080");
			break;
		case CELL_VIDEO_OUT_RESOLUTION_960x1080:
			cellDbgFontPrintf(0.5f, yPos, FontSize(), Graphics->GetInitialResolution() == CELL_VIDEO_OUT_RESOLUTION_960x1080 ? GREEN : ORANGE, "960x1080");
			break;
	}
	Graphics->FlushDbgFont();

	yPos += ySpacing;
	cellDbgFontPuts		(0.09f,	yPos,	FontSize(),	currently_selected_setting == SETTING_PAL60_MODE ? YELLOW : WHITE,	"PAL60 Mode (576p only)");
	cellDbgFontPrintf	(0.5f,	yPos,	FontSize(),	Settings.PS3PALTemporalMode60Hz == true ? ORANGE : GREEN, Settings.PS3PALTemporalMode60Hz == true ? "ON" : "OFF");

	yPos += ySpacing;
	cellDbgFontPuts(0.09f, yPos, FontSize(), currently_selected_setting == SETTING_SHADER ? YELLOW : WHITE, "Selected shader");
	cellDbgFontPrintf(0.5f, yPos, FontSize(), 
			GREEN, 
			"%s", Graphics->GetFragmentShaderPath().substr(Graphics->GetFragmentShaderPath().find_last_of('/')).c_str());

	yPos += ySpacing;
	cellDbgFontPuts(0.09f, yPos, FontSize(), currently_selected_setting == SETTING_FONT_SIZE ? YELLOW : WHITE, "Font size");
	cellDbgFontPrintf(0.5f,	yPos,	FontSize(),	Settings.PS3FontSize == 100 ? GREEN : ORANGE, "%f", FontSize());

	yPos += ySpacing;
	cellDbgFontPuts(0.09f, yPos, FontSize(), currently_selected_setting == SETTING_KEEP_ASPECT_RATIO ? YELLOW : WHITE, "Aspect Ratio");
	cellDbgFontPrintf(0.5f, yPos, FontSize(), Settings.PS3KeepAspect == true ? GREEN : ORANGE, "%s", Settings.PS3KeepAspect == true ? "Scaled" : "Stretched");
	Graphics->FlushDbgFont();

	yPos += ySpacing;
	cellDbgFontPuts(0.09f, yPos, FontSize(), currently_selected_setting == SETTING_HW_TEXTURE_FILTER ? YELLOW : WHITE, "Hardware Filtering");
	cellDbgFontPrintf(0.5f, yPos, FontSize(), Settings.PS3Smooth == true ? GREEN : ORANGE,
			"%s", Settings.PS3Smooth == true ? "Linear interpolation" : "Point filtering");

	yPos += ySpacing;
	cellDbgFontPuts		(0.09f,	yPos,	FontSize(),	currently_selected_setting == SETTING_HW_OVERSCAN_AMOUNT ? YELLOW : WHITE,	"Overscan");
	cellDbgFontPrintf	(0.5f,	yPos,	FontSize(),	Settings.PS3OverscanAmount == 0 ? GREEN : ORANGE, "%f", (float)Settings.PS3OverscanAmount/100);

	yPos += ySpacing;
	cellDbgFontPuts(0.09f, yPos, FontSize(), currently_selected_setting == SETTING_RSOUND_ENABLED ? YELLOW : WHITE, "Sound");
	cellDbgFontPuts(0.5f, yPos, FontSize(), Settings.RSoundEnabled == false ? GREEN : ORANGE, Settings.RSoundEnabled == true ? "RSound" : "Normal");

	yPos += ySpacing;
	cellDbgFontPuts(0.09f, yPos, FontSize(), currently_selected_setting == SETTING_RSOUND_SERVER_IP_ADDRESS ? YELLOW : WHITE, "RSound Server IP Address");
	cellDbgFontPuts(0.5f, yPos, FontSize(), strcmp(Settings.RSoundServerIPAddress,"0.0.0.0") ? ORANGE : GREEN, Settings.RSoundServerIPAddress);

	yPos += ySpacing;
	cellDbgFontPrintf(0.09f, yPos, FontSize(), currently_selected_setting == SETTING_DEFAULT_ALL ? YELLOW : GREEN, "DEFAULT");
	Graphics->FlushDbgFont();

	DisplayHelpMessage(currently_selected_setting);

	cellDbgFontPuts(0.09f, 0.88f, FontSize(), YELLOW, "UP/DOWN - select,  X/LEFT/RIGHT - change,  START - default");
	cellDbgFontPuts(0.09f, 0.92f, FontSize(), YELLOW, "CIRCLE - return to menu, L2+R2 - resume game, R1 - go forward");
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

	cellDbgFontPuts(0.09f, 0.88f, FontSize(), YELLOW, "X - Enter directory/Load game");
	cellDbgFontPuts(0.09f, 0.92f, FontSize(), PURPLE, "L2 + R2 - return to game");
	cellDbgFontPuts(0.5f, 0.92f, FontSize(), BLUE, "SELECT - Settings screen");
	Graphics->FlushDbgFont();

	RenderBrowser(browser);
}

void MenuMainLoop()
{
	// create file browser->if null
	if (browser == NULL)
	{
		browser = new FileBrowser(Settings.PS3PathROMDirectory);
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

