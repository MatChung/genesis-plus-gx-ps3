#ifndef GENESIS_PLUS_H_
#define GENESIS_PLUS_H_

#include "cellframework/audio/audioport.hpp"
#include "cellframework/audio/rsound.hpp"

#define USRDIR "/dev_hdd0/game/GENP00001/USRDIR/"

enum Emulator_Modes
{
	MODE_MENU,
	MODE_EMULATION,
	MODE_EXIT
};

extern PS3Graphics* Graphics;
extern CellInputFacade* CellInput;

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
