/****************************************************************************
 *  main.c
 *
 *  Genesis Plus GX
 *
 *  Softdev (2006)
 *  Eke-Eke (2007,2008,2009)
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 *
 ***************************************************************************/

#include "shared.h"
#include "font.h"
#include "gui.h"
#include "history.h"
#include "aram.h"
#include "dvd.h"
#include "file_slot.h"

#ifdef HW_RVL
#include "usb2storage.h"
#include "mload.h"
#endif

#include <fat.h>

#ifdef HW_RVL
#include <wiiuse/wpad.h>
#endif

u32 Shutdown = 0;

#ifdef HW_RVL
/****************************************************************************
 * Power Button callback 
 ***************************************************************************/
static void Power_Off(void)
{
  Shutdown = 1;
  ConfigRequested = 1;
}

/****************************************************************************
 * IOS support
 ***************************************************************************/
static bool FindIOS(u32 ios)
{
	s32 ret;
	u32 n;
	
  u64 *titles = NULL;
	u32 num_titles=0;
	
	ret = ES_GetNumTitles(&num_titles);
	if (ret < 0)
		return false;
	
	if(num_titles < 1) 
		return false;
	
	titles = (u64 *)memalign(32, num_titles * sizeof(u64) + 32);
	if (!titles)
		return false;
	
	ret = ES_GetTitles(titles, num_titles);
	if (ret < 0)
	{
		free(titles);
		return false;
	}
	
	for(n=0; n < num_titles; n++)
	{
		if((titles[n] & 0xFFFFFFFF)==ios) 
		{
			free(titles); 
			return true;
		}
	}
  free(titles); 
	return false;
}
#endif

/***************************************************************************
 * Genesis Plus Virtual Machine
 *
 ***************************************************************************/
static void load_bios(void)
{
  /* clear BIOS detection flag */
  config.tmss &= ~2;

  /* open BIOS file */
  FILE *fp = fopen(OS_ROM, "rb");
  if (fp == NULL) return;

  /* read file */
  fread(bios_rom, 1, 0x800, fp);
  fclose(fp);

  /* check ROM file */
  if (!strncmp((char *)(bios_rom + 0x120),"GENESIS OS", 10))
  {
    /* valid BIOS detected */
    config.tmss |= 2;
  }
}

static void init_machine(void)
{
  /* Allocate cart_rom here ( 10 MBytes ) */
  cart.rom = memalign(32, MAXROMSIZE);
  if (!cart.rom)
  {
    FONT_writeCenter("Failed to allocate ROM buffer... Rebooting",18,0,640,200,(GXColor)WHITE);
    gxSetScreen();
    sleep(2);
    gx_audio_Shutdown();
    gx_video_Shutdown();
#ifdef HW_RVL
    DI_Close();
    SYS_ResetSystem(SYS_RESTART,0,0);
#else
    SYS_ResetSystem(SYS_HOTRESET,0,0);
#endif
  }

  /* BIOS support */
  load_bios();

  /* allocate global work bitmap */
  memset (&bitmap, 0, sizeof (bitmap));
  bitmap.width  = 720;
  bitmap.height = 576;
  bitmap.depth  = 16;
  bitmap.granularity = 2;
  bitmap.pitch = bitmap.width * bitmap.granularity;
  bitmap.viewport.w = 256;
  bitmap.viewport.h = 224;
  bitmap.viewport.x = 0;
  bitmap.viewport.y = 0;
  bitmap.data = texturemem;
}

static void run_emulation(void)
{
  /* main emulation loop */
  while (1)
  {
    /* Main Menu request */
    if (ConfigRequested)
    {
      /* stop video & audio */
      gx_audio_Stop();
      gx_video_Stop();

      /* show menu */
      MainMenu ();
      ConfigRequested = 0;

      /* start video & audio */
      gx_video_Start();
      gx_audio_Start();
      frameticker = 1;
    }

    /* automatic frame skipping */
    if (frameticker > 1)
    {
      /* skip frame */
      system_frame(1);
      frameticker = 1;
    }
    else
    {
      /* render frame */
      frameticker = 0;
      system_frame(0);

      /* update video */
      gx_video_Update();
    }

    /* update audio */
    gx_audio_Update();

    /* check interlaced mode change */
    if (bitmap.viewport.changed & 4)
    {
      /* in original 60hz modes, audio is synced with framerate */
      if (!config.render && !vdp_pal && (config.tv_mode != 1))
      {
        u8 *temp = memalign(32,YM2612GetContextSize());
        if (temp)
        {
          /* save YM2612 context */
          memcpy(temp, YM2612GetContextPtr(), YM2612GetContextSize());

          /* framerate has changed, reinitialize audio timings */
          audio_init(48000, interlaced ? 59.94 : (1000000.0/16715.0));
          sound_init();

          /* restore YM2612 context */
          YM2612Restore(temp);
          free(temp);
        }
      }

      /* clear flag */
      bitmap.viewport.changed &= ~4;
    }

    /* wait for next frame */
    while (frameticker < 1)
      usleep(1);
  }
}

/**************************************************
  Load a new rom and performs some initialization
***************************************************/
void reloadrom (int size, char *name)
{
  /* cartridge hot-swap support */
  uint8 hotswap = 0;
  if (cart.romsize)
    hotswap = config.hot_swap;

  /* Load ROM */
  cart.romsize = size;
  load_rom(name);

  if (hotswap)
  {
    cart_hw_init();
    cart_hw_reset();
  }
  else
  {
    /* initialize audio back-end */
    /* 60hz video mode requires synchronization with Video Interrupt.    */
    /* Framerate is 59.94 fps in interlaced/progressive modes, ~59.825 fps in non-interlaced mode */
    float framerate = vdp_pal ? 50.0 : ((config.tv_mode == 1) ? 60.0 : (config.render ? 59.94 : (1000000.0/16715.0)));
    audio_init(48000, framerate);

    /* System Power ON */
    system_init ();
    ClearGGCodes ();
    system_reset ();
  }
}

/**************************************************
  Shutdown everything properly
***************************************************/
void shutdown(void)
{
  /* system shutdown */
  if (config.s_auto & 2)
    slot_autosave(config.s_default,config.s_device);
  system_shutdown();
  audio_shutdown();
  free(cart.rom);
  gx_audio_Shutdown();
  gx_video_Shutdown();
#ifdef HW_RVL
  DI_Close();
  mload_close();
#endif
}

/***************************************************************************
 *  M A I N
 *
 ***************************************************************************/
u32 fat_enabled = 0;
u32 frameticker = 0;

int main (int argc, char *argv[])
{
#ifdef HW_RVL
	/* try to load IOS 202 */
	if(IOS_GetVersion() != 202 && FindIOS(202))
		IOS_ReloadIOS(202);
	
	if(IOS_GetVersion() == 202)
	{
		/* disable DVDX stub */
        DI_LoadDVDX(false);
		
		/* load EHCI module & enable USB2 driver */
		if(mload_init() >= 0 && load_ehci_module())
			USB2Enable(true);
	}

  /* initialize DVD driver */
  DI_Init();
#endif

  /* initialize video engine */
  gx_video_Init();

#ifdef HW_DOL
  /* initialize DVD driver */
  DVD_Init ();
  dvd_drive_detect();
#endif

  /* initialize FAT devices */
  if (fatInitDefault())
  {
    DIR_ITER *dir = NULL;

    /* base directory */
    char pathname[MAXPATHLEN];
    sprintf (pathname, DEFAULT_PATH);
    dir = diropen(pathname);
    if (dir == NULL) mkdir(pathname,S_IRWXU);
    else dirclose(dir);

    /* default SRAM & Savestate files directory */ 
    sprintf (pathname, "%s/saves",DEFAULT_PATH);
    dir = diropen(pathname);
    if (dir == NULL) mkdir(pathname,S_IRWXU);
    else dirclose(dir);

    /* default Snapshot files directory */ 
    sprintf (pathname, "%s/snaps",DEFAULT_PATH);
    dir = diropen(pathname);
    if (dir == NULL) mkdir(pathname,S_IRWXU);
    else dirclose(dir);
  }

  /* initialize input engine */
  gx_input_Init();

  /* initialize sound engine (need libfat) */
  gx_audio_Init();

  /* initialize genesis plus core */
  legal();
  config_default();
  history_default();
  init_machine();

  /* run any injected rom */
  if (cart.romsize)
  {
    ARAMFetch((char *)cart.rom, (void *)0x8000, cart.romsize);
    reloadrom (cart.romsize,"INJECT.bin");
    gx_video_Start();
    gx_audio_Start();
    frameticker = 1;
  }
  else
  {
    /* Main Menu */
    ConfigRequested = 1;
  }

  /* initialize GUI engine */
  GUI_Initialize();

#ifdef HW_RVL
  /* Power button callback */
  SYS_SetPowerCallback(Power_Off);
#endif

  /* main emulation loop */
  while (1)
  {
    /* Main Menu request */
    if (ConfigRequested)
    {
      /* stop video & audio */
      gx_audio_Stop();
      gx_video_Stop();

      /* show menu */
      MainMenu ();
      ConfigRequested = 0;

      /* start video & audio */
      gx_audio_Start();
      gx_video_Start();
      frameticker = 1;
    }

    if (frameticker > 1)
    {
      /* skip frame */
      system_frame(1);
      --frameticker;
    }
    else
    {
      while (frameticker < 1)
        usleep(10);

      /* render frame */
      system_frame(0);
      --frameticker;

      /* update video */
      gx_video_Update();
    }

    /* update audio */
    gx_audio_Update();
  }

  return 0;
}
