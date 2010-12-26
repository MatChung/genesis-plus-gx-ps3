#ifndef GENESIS_PLUS_H_
#define GENESIS_PLUS_H_

#include "cellframework/audio/audioport.hpp"
#include "cellframework/audio/rsound.hpp"

#define USRDIR "/dev_hdd0/game/GENP00001/USRDIR/"

#define EMULATOR_VERSION "1.2"

enum Emulator_Modes
{
	MODE_MENU,
	MODE_EMULATION,
	MODE_EXIT
};

enum
{
	MAP_BUTTONS_OPTION_SETTER,
	MAP_BUTTONS_OPTION_GETTER,
	MAP_BUTTONS_OPTION_DEFAULT
};

extern PS3Graphics* Graphics;
extern CellInputFacade* CellInput;

float Emulator_GetFontSize();
void Emulator_Implementation_ButtonMappingSettings(bool map_button_option_enum);
bool Emulator_IsROMLoaded();
void Emulator_SwitchMode(Emulator_Modes);
void Emulator_Shutdown();
void Emulator_StopROMRunning();
void Emulator_StartROMRunning();
void Emulator_SetExtraCartPaths();
bool Emulator_ROMRunning();
void Emulator_OSKStart(const wchar_t* msg, const wchar_t* init);
const char * Emulator_OSKOutputString();
void Emulator_RequestLoadROM(char * rom, bool forceReload);
void Emulator_ToggleSound();
void Emulator_SetControllerMode();

#endif /* GENESIS_PLUS_H_ */
