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

// if you add more settings to the screen, remember to change this value to the correct number
// PATH - Total amount of Path settings
// Offset is 100
#define MAX_NO_OF_PATH_SETTINGS 104

//GENESIS PLUS - Total amount of Genesis Plus GX settings
// Offset is 50

#define MAX_NO_OF_GENESIS_SETTINGS      59

//GENERAL - Total amount of settings
// Offset is 0
#define MAX_NO_OF_SETTINGS      10

//GENERAL - setting constants
#define SETTING_CHANGE_RESOLUTION 0
#define SETTING_PAL60_MODE 1
#define SETTING_SHADER 2
#define SETTING_FONT_SIZE 3
#define SETTING_KEEP_ASPECT_RATIO 4
#define SETTING_HW_TEXTURE_FILTER 5
#define SETTING_HW_OVERSCAN_AMOUNT 6
#define SETTING_RSOUND_ENABLED 7
#define SETTING_RSOUND_SERVER_IP_ADDRESS 8
#define SETTING_DEFAULT_ALL 9

//GENESIS PLUS - setting constants
#define SETTING_GENESIS_FPS 50
#define SETTING_GENESIS_PAD 51
#define SETTING_GENESIS_EXCART 52
#define SETTING_GENESIS_ACTIONREPLAY_ROMPATH 53
#define SETTING_GENESIS_GAMEGENIE_ROMPATH 54
#define SETTING_GENESIS_SK_ROMPATH 55
#define SETTING_GENESIS_SK_UPMEM_ROMPATH 56
#define SETTING_GENESIS_BIOS_ROMPATH 57
#define SETTING_GENESIS_DEFAULT_ALL 58

//PATH - setting constants
#define SETTING_PATH_DEFAULT_ROM_DIRECTORY 100
#define SETTING_PATH_SAVESTATES_DIRECTORY 101
#define SETTING_PATH_SRAM_DIRECTORY 102
#define SETTING_PATH_DEFAULT_ALL 103

void MenuMainLoop();
float FontSize();
void MenuStop();
bool MenuIsRunning();
char* MenuGetSelectedROM();
char* MenuGetCurrentPath();
char* do_pathmenu(uint16_t is_for_shader_or_dir_selection, const char * pathname = "/");

#endif /* MENU_H_ */

