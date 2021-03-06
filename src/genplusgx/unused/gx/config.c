/****************************************************************************
 *  config.c
 *
 *  Genesis Plus GX configuration file support
 *
 *  Eke-Eke (2008)
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

void config_save(void)
{
  /* open configuration file */
  char fname[MAXPATHLEN];
  sprintf (fname, "%s/config.ini", DEFAULT_PATH);
  FILE *fp = fopen(fname, "wb");
  if (fp)
  {
    /* write file */
    fwrite(&config, sizeof(config), 1, fp);
    fclose(fp);
  }
}

void config_load(void)
{
  /* open configuration file */
  char fname[MAXPATHLEN];
  sprintf (fname, "%s/config.ini", DEFAULT_PATH);
  FILE *fp = fopen(fname, "rb");
  if (fp)
  {
    /* read version */
    char version[16];
    fread(version, 16, 1, fp); 
    fclose(fp);
    if (strcmp(version,VERSION))
      return;

    /* read file */
    fp = fopen(fname, "rb");
    if (fp)
    {
      fread(&config, sizeof(config), 1, fp);
      fclose(fp);
    }
  }
}

void config_default(void)
{
  /* version TAG */
  strncpy(config.version,VERSION,16);

  /* sound options */
  config.psg_preamp     = 150;
  config.fm_preamp      = 100;
  config.hq_fm          = 1;
  config.psgBoostNoise  = 0;
  config.filter         = 1;
  config.lp_range       = 50;
  config.low_freq       = 880;
  config.high_freq      = 5000;
  config.lg             = 1.0;
  config.mg             = 1.0;
  config.hg             = 1.0;
  config.rolloff        = 0.995;
  config.dac_bits 		= 14;

  /* system options */
  config.region_detect  = 0;
  config.force_dtack    = 0;
  config.addr_error     = 1;
  config.bios_enabled   = 0;
  config.lock_on        = 0;
  config.romtype        = 0;
  config.hot_swap       = 0;

  /* video options */
  config.xshift   = 0;
  config.yshift   = 0;
  config.xscale   = 0;
  config.yscale   = 0;
  config.aspect   = 1;
  config.overscan = 1;
  if (VIDEO_HaveComponentCable())
    config.render = 2;
  else
    config.render = 0;
  config.ntsc     = 0;
  config.bilinear = 1;
#ifdef HW_RVL
  config.trap     = 1;
  config.gamma    = VI_GM_1_0 / 10.0;
#endif

  /* controllers options */
  config.gun_cursor[0]  = 1;
  config.gun_cursor[1]  = 1;
  config.invert_mouse   = 0;
  gx_input_SetDefault();

  /* menu options */
#ifdef HW_RVL
  config.sram_auto    = 0;
#else
  config.sram_auto    = -1;
#endif
  config.state_auto   = -1;
  config.bg_color     = 0;
  config.screen_w     = 658;
  config.ask_confirm  = 0;
  config.bgm_volume   = 100.0;
  config.sfx_volume   = 100.0;

  /* restore saved configuration */
  config_load();
  io_init();
}

