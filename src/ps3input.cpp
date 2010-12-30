#include "ps3input.h"

char * Input_PrintMappedButton(int mappedbutton)
{
	switch(mappedbutton)
	{
		case BTN_A:
			return "Button A";
			break;
		case BTN_B:
			return "Button B";
			break;
		case BTN_C:
			return "Button C";
			break;
		case BTN_X:
			return "Button X";
			break;
		case BTN_Y:
			return "Button Y";
			break;
		case BTN_Z:
			return "Button Z";
			break;
		case BTN_START:
			return "Button Start";
			break;
		case BTN_MODE:
			return "Button Mode";
			break;
		case BTN_LEFT:
			return "D-Pad Left";
			break;
		case BTN_RIGHT:
			return "D-Pad Right";
			break;
		case BTN_UP:
			return "D-Pad Up";
			break;
		case BTN_DOWN:
			return "D-Pad Down";
			break;
		case BTN_NONE:
			return "None";
			break;
		case BTN_EXITTOMENU:
			return "Exit to menu";
			break;
		case BTN_QUICKSAVE:
			return "Save State";
			break;
		case BTN_QUICKLOAD:
			return "Load State";
			break;
		case BTN_SOFTRESET:
			return "Software Reset";
			break;
		case BTN_HARDRESET:
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
		case BTN_UP:
			return next ? BTN_DOWN : BTN_NONE;
			break;
		case BTN_DOWN:
			return next ? BTN_LEFT : BTN_UP;
			break;
		case BTN_LEFT:
			return next ? BTN_RIGHT : BTN_DOWN;
			break;
		case BTN_RIGHT:
			return next ? BTN_A : BTN_LEFT;
			break;
		case BTN_A:
			return next ? BTN_B : BTN_RIGHT;
			break;
		case BTN_B:
			return next ? BTN_C : BTN_A;
			break;
		case BTN_C:
			return next ? BTN_X : BTN_B;
			break;
		case BTN_X:
			return next ? BTN_Y : BTN_C;
			break;
		case BTN_Y:
			return next ? BTN_Z : BTN_X;
			break;
		case BTN_Z:
			return next ? BTN_START : BTN_Y;
			break;
		case BTN_START:
			return next ? BTN_MODE : BTN_Z;
			break;
		case BTN_MODE:
			return next ? BTN_HARDRESET : BTN_START;
			break;
		case BTN_HARDRESET:
			return next ? BTN_SOFTRESET : BTN_MODE;
			break;
		case BTN_SOFTRESET:
			return next ? BTN_QUICKSAVE : BTN_HARDRESET;
			break;
		case BTN_QUICKSAVE:
			return next ? BTN_QUICKLOAD : BTN_SOFTRESET;
			break;
		case BTN_QUICKLOAD:
			return next ? BTN_EXITTOMENU : BTN_QUICKSAVE;
			break;
		case BTN_EXITTOMENU:
			return next ? BTN_NONE : BTN_QUICKLOAD;
			break;
		case BTN_NONE:
			return next ? BTN_UP : BTN_EXITTOMENU;
			break;
		default:
			return BTN_NONE;
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
	if(*buttonmap == (BTN_LEFT | BTN_RIGHT | BTN_DOWN | BTN_UP | BTN_A | BTN_B | BTN_C | \
	BTN_X | BTN_Y | BTN_Z | BTN_START | BTN_MODE))
	{
	}
}
