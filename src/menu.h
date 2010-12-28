/*
 * menu.h
 *
 *  Created on: Oct 10, 2010
 *      Author: halsafar
 */

#ifndef MENU_H_
#define MENU_H_

#include <string>
#include "colors.h"

#define MAX_PATH 1024

#define FIRST_GENERAL_SETTING	0
#define FIRST_GENESIS_SETTING	50
#define FIRST_PATH_SETTING	100
#define FIRST_CONTROLS_SETTING	150

//GENERAL - setting constants
enum
{
	SETTING_CHANGE_RESOLUTION = FIRST_GENERAL_SETTING,
	//SETTING_PAL60_MODE,
	SETTING_SHADER,
	SETTING_FONT_SIZE,
	SETTING_KEEP_ASPECT_RATIO,
	SETTING_HW_TEXTURE_FILTER,
	SETTING_HW_OVERSCAN_AMOUNT,
	SETTING_RSOUND_ENABLED,
	SETTING_RSOUND_SERVER_IP_ADDRESS,
	SETTING_DEFAULT_ALL
};

//GENESIS PLUS - setting constants
enum
{
	SETTING_GENESIS_FPS		= FIRST_GENESIS_SETTING,
	SETTING_GENESIS_PAD,
	SETTING_GENESIS_EXCART,
	SETTING_GENESIS_ACTIONREPLAY_ROMPATH,
	SETTING_GENESIS_GAMEGENIE_ROMPATH,
	SETTING_GENESIS_SK_ROMPATH,
	SETTING_GENESIS_SK_UPMEM_ROMPATH,
	SETTING_GENESIS_BIOS_ROMPATH,
	SETTING_GENESIS_DEFAULT_ALL
};

//PATH - setting constants
enum
{
	SETTING_PATH_DEFAULT_ROM_DIRECTORY	= FIRST_PATH_SETTING,
	SETTING_PATH_SAVESTATES_DIRECTORY,
	SETTING_PATH_SRAM_DIRECTORY,
	SETTING_PATH_DEFAULT_ALL
};

//CONTROLS - setting constants
enum
{
	SETTING_CONTROLS_DPAD_UP = FIRST_CONTROLS_SETTING,
	SETTING_CONTROLS_DPAD_DOWN,
	SETTING_CONTROLS_DPAD_LEFT,
	SETTING_CONTROLS_DPAD_RIGHT,
	SETTING_CONTROLS_BUTTON_CIRCLE,
	SETTING_CONTROLS_BUTTON_CROSS,
	SETTING_CONTROLS_BUTTON_TRIANGLE,
	SETTING_CONTROLS_BUTTON_SQUARE,
	SETTING_CONTROLS_BUTTON_SELECT,
	SETTING_CONTROLS_BUTTON_START,
	SETTING_CONTROLS_BUTTON_L1,
	SETTING_CONTROLS_BUTTON_R1,
	SETTING_CONTROLS_BUTTON_L2,
	SETTING_CONTROLS_BUTTON_R2,
	SETTING_CONTROLS_BUTTON_L3,
	SETTING_CONTROLS_BUTTON_R3,
	SETTING_CONTROLS_BUTTON_L2_BUTTON_L3,
	SETTING_CONTROLS_BUTTON_L2_BUTTON_R2,
	SETTING_CONTROLS_BUTTON_L2_BUTTON_R3,
	SETTING_CONTROLS_BUTTON_L2_ANALOG_R_RIGHT,
	SETTING_CONTROLS_BUTTON_L2_ANALOG_R_LEFT,
	SETTING_CONTROLS_BUTTON_L2_ANALOG_R_UP,
	SETTING_CONTROLS_BUTTON_L2_ANALOG_R_DOWN,
	SETTING_CONTROLS_BUTTON_R2_ANALOG_R_RIGHT,
	SETTING_CONTROLS_BUTTON_R2_ANALOG_R_LEFT,
	SETTING_CONTROLS_BUTTON_R2_ANALOG_R_UP,
	SETTING_CONTROLS_BUTTON_R2_ANALOG_R_DOWN,
	SETTING_CONTROLS_BUTTON_R2_BUTTON_R3,
	SETTING_CONTROLS_BUTTON_R3_BUTTON_L3,
	SETTING_CONTROLS_ANALOG_R_UP,
	SETTING_CONTROLS_ANALOG_R_DOWN,
	SETTING_CONTROLS_ANALOG_R_LEFT,
	SETTING_CONTROLS_ANALOG_R_RIGHT,
	SETTING_CONTROLS_DEFAULT_ALL
};

//GENERAL - Total amount of settings
#define MAX_NO_OF_SETTINGS				SETTING_DEFAULT_ALL+1
//SNES9x - Total amount of GENESIS PLUS settings
#define MAX_NO_OF_GENESIS_SETTINGS			SETTING_GENESIS_DEFAULT_ALL+1
// PATH - Total amount of Path settings
#define MAX_NO_OF_PATH_SETTINGS				SETTING_PATH_DEFAULT_ALL+1
//CONTROLS - Total amount of controls settings
#define MAX_NO_OF_CONTROLS_SETTINGS			SETTING_CONTROLS_DEFAULT_ALL+1

void MenuMainLoop();
float FontSize();
void MenuStop();
bool MenuIsRunning();
char* MenuGetSelectedROM();
char* MenuGetCurrentPath();
char* do_pathmenu(uint16_t is_for_shader_or_dir_selection, const char * pathname = "/");

/* Input bitmasks */
//BEGINNING OF GENESIS PLUS PS3
#define INPUT_NONE	(0x00000900)
//END OF GENESIS PLUS PS3
#define INPUT_MODE      (0x00000800)
#define INPUT_Z         (0x00000400)
#define INPUT_Y         (0x00000200)
#define INPUT_X         (0x00000100)
#define INPUT_START     (0x00000080)
#define INPUT_C         (0x00000040)
#define INPUT_B         (0x00000020)
#define INPUT_A         (0x00000010)
#define INPUT_LEFT      (0x00000008)
#define INPUT_RIGHT     (0x00000004)
#define INPUT_DOWN      (0x00000002)
#define INPUT_UP        (0x00000001)


#endif /* MENU_H_ */
