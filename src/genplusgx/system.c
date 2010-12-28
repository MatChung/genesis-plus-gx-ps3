/***************************************************************************************
 *  Genesis Plus
 *  Main Emulation
 *
 *  Copyright (C) 1998, 1999, 2000, 2001, 2002, 2003  Charles Mac Donald (original code)
 *  Eke-Eke (2007,2008,2009), additional code & fixes for the GCN/Wii port
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
 ****************************************************************************************/

#include "shared.h"
#include "sound/Fir_Resampler.h"
#include "sound/eq.h"

/* Global variables */
t_bitmap bitmap;
t_snd snd;
uint32 mcycles_vdp;
uint32 mcycles_z80;
uint32 mcycles_68k;
uint32 hint_68k;
uint8 system_hw;

/****************************************************************
 * AUDIO equalizer
 ****************************************************************/
static EQSTATE eq;

void audio_set_equalizer(void)
{
  init_3band_state(&eq,config.low_freq,config.high_freq,snd.sample_rate);
  eq.lg = (double)(config.lg);
  eq.mg = (double)(config.mg);
  eq.hg = (double)(config.hg);
}

/****************************************************************
 * AUDIO stream update & mixing
 ****************************************************************/
static int32 llp,rrp;

extern unsigned char soundbuffer[3840];

int audio_update (void)
{
  int32 i, l, r;
  int32 ll = llp;
  int32 rr = rrp;

  int psg_preamp  = config.psg_preamp;
  int fm_preamp   = config.fm_preamp;
  int filter      = config.filter;
  uint32 factora  = (config.lp_range << 16) / 100;
  uint32 factorb  = 0x10000 - factora;

  int32 *fm       = snd.fm.buffer;
  int16 *psg      = snd.psg.buffer;

#ifdef NGC
  int16 *sb = (int16 *) soundbuffer[mixbuffer];
#else
  int16 *sb = (int16 *) soundbuffer;
#endif

  /* get number of available samples */
  int size = sound_update(mcycles_vdp);

  /* return an aligned number of samples */
  size &= ~7;

  if (config.hq_fm)
  {
    /* resample into FM output buffer */
    Fir_Resampler_read(fm,size);

#ifdef LOGSOUND
    error("%d FM samples remaining\n",Fir_Resampler_written() >> 1);
#endif
  }
  else
  {  
    /* adjust remaining samples in FM output buffer*/
    snd.fm.pos -= (size << 1);

#ifdef LOGSOUND
    error("%d FM samples remaining\n",(snd.fm.pos - snd.fm.buffer)>>1);
#endif
  }

  /* adjust remaining samples in PSG output buffer*/
  snd.psg.pos -= size;

#ifdef LOGSOUND
  error("%d PSG samples remaining\n",snd.psg.pos - snd.psg.buffer);
#endif

  /* mix samples */
  for (i = 0; i < size; i ++)
  {
    /* PSG samples (mono) */
    l = r = (((*psg++) * psg_preamp) / 100);

    /* FM samples (stereo) */
    l += ((*fm++ * fm_preamp) / 100);
    r += ((*fm++ * fm_preamp) / 100);

    /* filtering */
    if (filter & 1)
    {
      /* single-pole low-pass filter (6 dB/octave) */
      ll = (ll>>16)*factora + l*factorb;
      rr = (rr>>16)*factora + r*factorb;
      l = ll >> 16;
      r = rr >> 16;
    }
    else if (filter & 2)
    {
      /* 3 Band EQ */
      l = do_3band(&eq,l);
      r = do_3band(&eq,r);
    }

    /* clipping (16-bit samples) */
    if (l > 32767)
      l = 32767;
    else if (l < -32768)
      l = -32768;
    if (r > 32767)
      r = 32767;
    else if (r < -32768)
      r = -32768;

    /* update sound buffer */
#ifndef NGC
//    snd.buffer[0][i] = r;
//    snd.buffer[1][i] = l;
    *sb++ = r;
    *sb++ = l;
#else
    *sb++ = r;
    *sb++ = l;
#endif
  }

  /* save filtered samples for next frame */
  llp = ll;
  rrp = rr;

  /* keep remaining samples for next frame */
  memcpy(snd.fm.buffer, fm, (snd.fm.pos - snd.fm.buffer) << 1);
  memcpy(snd.psg.buffer, psg, (snd.psg.pos - snd.psg.buffer) << 1);

#ifdef LOGSOUND
  error("%d samples returned\n\n",size);
#endif

  return size;
}

/****************************************************************
 * AUDIO System initialization
 ****************************************************************/
int audio_init (int samplerate, float framerate)
{
  /* Shutdown first */
  audio_shutdown();

  /* Clear the sound data context */
  memset(&snd, 0, sizeof (snd));

  /* Default settings */
  snd.sample_rate = samplerate;
  snd.frame_rate  = framerate;

  /* Calculate the sound buffer size (for one frame) */
  snd.buffer_size = (int)(samplerate / framerate) + 32;

#ifndef NGC
  /* Output buffers */
  snd.buffer[0] = (int16 *) malloc(snd.buffer_size * sizeof(int16));
  snd.buffer[1] = (int16 *) malloc(snd.buffer_size * sizeof(int16));
  if (!snd.buffer[0] || !snd.buffer[1])
    return (-1);
#endif

  /* SN76489 stream buffers */
  snd.psg.buffer = (int16 *) malloc(snd.buffer_size * sizeof(int16));
  if (!snd.psg.buffer)
    return (-1);

  /* YM2612 stream buffers */
  snd.fm.buffer = (int32 *) malloc(snd.buffer_size * sizeof(int32) * 2);
  if (!snd.fm.buffer)
    return (-1);

  /* Resampling buffer */
  if (config.hq_fm)
  {
    if (!Fir_Resampler_initialize(4096))
      return (-1);
  }

  /* Set audio enable flag */
  snd.enabled = 1;

  /* Reset audio */
  audio_reset();

  return (0);
}

/****************************************************************
 * AUDIO System reset
 ****************************************************************/
void audio_reset(void)
{
  /* Low-Pass filter */
  llp = 0;
  rrp = 0;

  /* 3 band EQ */
  audio_set_equalizer();

  /* audio buffers */
  Fir_Resampler_clear();
  snd.psg.pos = snd.psg.buffer;
  snd.fm.pos  = snd.fm.buffer;
#ifndef NGC
  if (snd.buffer[0])
    memset (snd.buffer[0], 0, snd.buffer_size * sizeof(int16));
  if (snd.buffer[1])
    memset (snd.buffer[1], 0, snd.buffer_size * sizeof(int16));
#endif
  if (snd.psg.buffer)
    memset (snd.psg.buffer, 0, snd.buffer_size * sizeof(int16));
  if (snd.fm.buffer)
    memset (snd.fm.buffer, 0, snd.buffer_size * sizeof(int32) * 2);
}

/****************************************************************
 * AUDIO System shutdown
 ****************************************************************/
void audio_shutdown(void)
{
  /* Sound buffers */
#ifndef NGC
  if (snd.buffer[0])
    free(snd.buffer[0]);
  if (snd.buffer[1])
    free(snd.buffer[1]);
#endif
  if (snd.fm.buffer)
    free(snd.fm.buffer);
  if (snd.psg.buffer)
    free(snd.psg.buffer);

  /* Resampling buffer */
  Fir_Resampler_shutdown();
}

/****************************************************************
 * Virtual Genesis initialization
 ****************************************************************/
void system_init (void)
{
  /* Genesis hardware */
  gen_init();
  io_init();
  vdp_init();
  render_init();

  /* Cartridge hardware */
  cart_hw_init();

  /* Sound Chips hardware */
  sound_init();
}

/****************************************************************
 * Virtual Genesis Hard Reset
 ****************************************************************/
void system_reset (void)
{
  /* Genesis hardware */
  gen_reset(1); 
  /* Cartridge Hardware */
  cart_hw_reset();
  io_reset();
  vdp_reset();
  render_reset();

  /* Sound chips */
  sound_reset();

  /* Audio System */
  audio_reset();
}

/****************************************************************
 * Virtual Genesis shutdown
 ****************************************************************/
void system_shutdown (void)
{
  gen_shutdown ();
  vdp_shutdown ();
  render_shutdown ();
}

/****************************************************************
 * Virtual Genesis Frame emulation
 ****************************************************************/
int system_frame (int do_skip)
{
  if (!gen_running)
  {
    osd_input_Update();
    return 0;
  }

  /* update display settings */
  int line;
  int reset = resetline;
  int vdp_height  = bitmap.viewport.h;
  int end_line    = vdp_height + bitmap.viewport.y;
  int start_line  = lines_per_frame - bitmap.viewport.y;
  int old_interlaced = interlaced;
  interlaced = (reg[12] & 2) >> 1;
  if (old_interlaced != interlaced)
  {
    bitmap.viewport.changed = 1;
    im2_flag = ((reg[12] & 6) == 6);
    odd_frame = 1;
  }
  odd_frame ^= 1;

  /* clear VBLANK, DMA, FIFO FULL & field flags */
  status &= 0xFEE5;

  /* set FIFO EMPTY flag */
  status |= 0x0200;

  /* even/odd field flag (interlaced modes only) */
  if (odd_frame && interlaced)
    status |= 0x0010;

  /* reload HCounter */
  int h_counter = reg[10];

  /* reset VDP FIFO */
  fifo_write_cnt  = 0;
  fifo_lastwrite  = 0;

  /* reset line cycle count */
  mcycles_vdp = 0;

  /* process scanlines */
  for (line = 0; line < lines_per_frame; line ++)
  {
    /* update VCounter */
    v_counter = line;

    /* update 6-Buttons or Menacer */
    input_update();

    /* 68k line cycle count */
    hint_68k = mcycles_68k;

    /* Soft Reset line */
    if (line == reset)
    {
      /* Pro Action Replay (switch at "Trainer" position) */
      if (config.lock_on == TYPE_AR)
        datel_reset(0);

      gen_reset(0);
    }

    /* update VDP DMA */
    if (dma_length)
      vdp_update_dma();

    /* vertical blanking */
    if (status & 8)
    {
      /* render overscan */
      if (!do_skip && ((line < end_line) || (line >= start_line)))
        render_line(line, 1);

      /* clear any pending Z80 interrupt */
      if (zirq)
      {
        zirq = 0;
        z80_set_irq_line(0, CLEAR_LINE);
      }
    }

    /* active display */
    else
    {
      /* H Interrupt */
      if(--h_counter < 0)
      {
        h_counter = reg[10];
        hint_pending = 0x10;
        if (reg[0] & 0x10)
          irq_status = (irq_status & ~0x40) | 0x14;
      }

      /* end of active display */
      if (line == vdp_height)
      {
        /* render overscan */
        if (!do_skip && (line < end_line))
          render_line(line, 1);

        /* update inputs (doing this here fix Warriors of Eternal Sun) */
        osd_input_Update();

        /* set VBLANK flag */
        status |= 0x08;

        /* Z80 interrupt is 16ms period (one frame) and 64us length (one scanline) */
        zirq = 1;
        z80_set_irq_line(0, ASSERT_LINE);

        /* delay between VINT flag & V Interrupt (Ex-Mutants, Tyrant) */
        m68k_run(mcycles_vdp + 588);
        status |= 0x80;

        /* delay between VBLANK flag & V Interrupt (Dracula, OutRunners, VR Troopers) */
        m68k_run(mcycles_vdp + 788);
        if (zstate == 1)
          z80_run(mcycles_vdp + 788);
        else
          mcycles_z80 = mcycles_vdp + 788;

        /* V Interrupt */
        vint_pending = 0x20;
        if (reg[1] & 0x20)
          irq_status = (irq_status & ~0x40) | 0x36;
      }
      else if (!do_skip) 
      {
        /* sprites are processed during horizontal blanking */
        if (reg[1] & 0x40)
          parse_satb(0x80 + line);

        /* render scanline */
        render_line(line, 0);
      }
    }

    /* process line */
    m68k_run(mcycles_vdp + MCYCLES_PER_LINE);
    if (zstate == 1)
      z80_run(mcycles_vdp + MCYCLES_PER_LINE);
    else 
      mcycles_z80 = mcycles_vdp + MCYCLES_PER_LINE;
    
    /* SVP chip */
    if (svp)
      ssp1601_run(SVP_cycles);

    /* update line cycle count */
    mcycles_vdp += MCYCLES_PER_LINE;
  }

  /* adjust cpu cycle count for next frame */
  mcycles_68k -= mcycles_vdp;
  mcycles_z80 -= mcycles_vdp;

  return gen_running;
}