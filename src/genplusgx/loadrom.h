
#ifndef _LOADROM_H_
#define _LOADROM_H_

#define MAXROMSIZE 10485760

typedef struct
{
  char consoletype[18];         /* Genesis or Mega Drive */
  char copyright[18];           /* Copyright message */
  char domestic[50];            /* Domestic name of ROM */
  char international[50];       /* International name of ROM */
  char ROMType[4];              /* Educational or Game */
  char product[14];             /* Product serial number */
  unsigned short checksum;      /* ROM Checksum (header) */
  unsigned short realchecksum;  /* ROM Checksum (calculated) */
  unsigned int romstart;        /* ROM start address */
  unsigned int romend;          /* ROM end address */
  char country[18];             /* Country flag */
  uint16 peripherals;           /* Supported peripherals */
} ROMINFO;


/* Global variables */
extern ROMINFO rominfo;

/* Function prototypes */
void deinterleave_block(uint8 *src);
int load_rom(char *filename);

#endif /* _LOADROM_H_ */

