#include "ps3input.h"

char * Input_PrintMappedButton(int mappedbutton)
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
int Input_GetAdjacentButtonmap(int buttonmap, bool next)
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

void Input_MapButton(int* buttonmap, bool next, int defaultbutton)
{
	if(defaultbutton == NULL)
	{
		*buttonmap = Input_GetAdjacentButtonmap(*buttonmap, next);
	}
	else
	{
		*buttonmap = defaultbutton;
	}
	if(*buttonmap == (INPUT_LEFT | INPUT_RIGHT | INPUT_DOWN | INPUT_UP | INPUT_A | INPUT_B | INPUT_C | \
	INPUT_X | INPUT_Y | INPUT_Z | INPUT_START | INPUT_MODE))
	{
	}
}
