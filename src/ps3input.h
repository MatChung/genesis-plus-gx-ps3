#include <string.h>
/* Input bitmasks */
//BEGINNING OF GENESIS PLUS PS3
#define BTN_SOFTRESET	(0x0E00)
#define BTN_HARDRESET	(0x0D00)
#define BTN_QUICKLOAD	(0x0C00)
#define BTN_QUICKSAVE	(0x0B00)
#define BTN_EXITTOMENU  (0x0A00)
#define BTN_NONE	(0x0900)
//END OF GENESIS PLUS PS3
#define INPUT_MODE      (0x0800)
#define INPUT_X         (0x0400)
#define INPUT_Y         (0x0200)
#define INPUT_Z         (0x0100)
#define INPUT_START     (0x0080)
#define INPUT_A         (0x0040)
#define INPUT_B         (0x0010)
#define INPUT_C         (0x0020)
#define INPUT_RIGHT     (0x0008)
#define INPUT_LEFT      (0x0004)
#define INPUT_DOWN      (0x0002)
#define INPUT_UP        (0x0001)
#define BTN_MODE	INPUT_MODE
#define BTN_X		INPUT_X
#define BTN_Y		INPUT_Y
#define BTN_Z		INPUT_Z
#define BTN_START	INPUT_START
#define BTN_A		INPUT_A
#define BTN_B		INPUT_B
#define BTN_C		INPUT_C
#define BTN_RIGHT	INPUT_RIGHT
#define BTN_LEFT	INPUT_LEFT
#define BTN_DOWN	INPUT_DOWN
#define BTN_UP		INPUT_UP

char * Input_PrintMappedButton(int mappedbutton);
int Input_GetAdjacentButtonmap(int buttonmap, bool next);
void Input_MapButton(int* buttonmap, bool next, int defaultbutton);
