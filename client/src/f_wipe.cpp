// Emacs style mode select   -*- C++ -*-
//-----------------------------------------------------------------------------
//
// $Id$
//
// Copyright (C) 1998-2006 by Randy Heit (ZDoom).
// Copyright (C) 2006-2013 by The Odamex Team.
//
// This program is free software; you can redistribute it and/or
// modify it under the terms of the GNU General Public License
// as published by the Free Software Foundation; either version 2
// of the License, or (at your option) any later version.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//
// DESCRIPTION:
//		Mission begin melt/wipe screen special effect.
//
//-----------------------------------------------------------------------------

#include "z_zone.h"
#include "v_video.h"
#include "m_random.h"
#include "m_alloc.h"
#include "doomdef.h"
#include "f_wipe.h"
#include "c_cvars.h"
#include "i_music.h"
#include "r_draw.h"

//
//		SCREEN WIPE PACKAGE
//

typedef enum
{
	wipe_None,			// don't bother
	wipe_Melt,			// weird screen melt
	wipe_Burn,			// fade in shape of fire
	wipe_Fade,			// crossfade from old to new
	wipe_NUMWIPES
} wipe_type_t;

static bool in_progress = false;

static wipe_type_t current_wipe_type;
EXTERN_CVAR (r_wipetype)

static byte* wipe_screen = NULL;

// Melt -------------------------------------------------------------

// [SL] The standard Doom screen wipe. This implementation borrows
// heavily from Eternity Engine, written by James Haley and SoM.
//

static int worms[320];

static void Wipe_StartMelt()
{
	worms[0] = - (M_Random() & 15); 

	for (int x = 1; x < 320; x++)
	{
		int random_value = (M_Random() % 3) - 1;
		worms[x] = worms[x - 1] + random_value;
		worms[x] = clamp(worms[x], -15, 0);
	}

	// copy each column of the current screen image to wipe_screen
	// each column is transposed and stored in row-major form for ease of use
	screen->GetTransposedBlock(0, 0, screen->width, screen->height, wipe_screen);
}

static void Wipe_StopMelt()
{
}

static bool Wipe_TickMelt()
{
	bool done = true;

	for (int x = 0; x < 320; x++)
	{
		if (worms[x] < 0)
		{
			++worms[x];
			done = false;
		}
		else if (worms[x] < 200)
		{
			int dy = (worms[x] < 16) ? worms[x] + 1 : 8;
			if (worms[x] + dy >= 200)
				dy = 200 - worms[x];
		
			worms[x] += dy;
			done = false;
		}
	}

	return done;
}

template<typename PIXEL_T>
static inline void Wipe_DrawMeltLoop(int x, int starty)
{
	const int pitch = screen->pitch / sizeof(PIXEL_T);
	PIXEL_T* to = (PIXEL_T*)screen->buffer + pitch * starty + x;
	const PIXEL_T* from = (PIXEL_T*)wipe_screen + screen->height * x;

	int y = screen->height - starty;
	while (y--)
	{
		*to = *from;
		to += pitch;
		from++; 
	}
}

static void Wipe_DrawMelt()
{
	for (int x = 0; x < screen->width; x++)
	{
		int wormx = x * 320 / screen->width;
		int wormy = worms[wormx] > 0 ? worms[wormx] : 0;

		wormy = wormy * screen->height / 200;

		if (screen->is8bit())
			Wipe_DrawMeltLoop<palindex_t>(x, wormy);
		else
			Wipe_DrawMeltLoop<argb_t>(x, wormy);
	}
}


// Burn -------------------------------------------------------------

// Well let's burn, until the sun burns your eyes and keep burnin' 'til it sets
// on the Westside...

// [RH] Fire Wipe
static const int FIREWIDTH = 64, FIREHEIGHT = 64;

static byte *burnarray = NULL;
static int density;
static int burntime;
static int voop;

static void Wipe_StartBurn()
{
	const size_t array_size = FIREWIDTH * (FIREHEIGHT + 5);
	burnarray = new byte[array_size];
	memset(burnarray, 0, array_size);
	density = 4;
	burntime = 0;
	voop = 0;
	screen->GetBlock(0, 0, screen->width, screen->height, (byte*)wipe_screen);
}

static void Wipe_StopBurn()
{
	if (burnarray)
	{
		delete [] burnarray;
		burnarray = NULL;
	}
}

static bool Wipe_TickBurn()
{
	// This is a modified version of the fire from the player
	// setup menu.
	burntime++;

	// Make the fire burn (twice per tic)
	for (int count = 0; count < 2; count++)
	{
		int a, b;
		byte *from;

		// generator
		from = burnarray + FIREHEIGHT * FIREWIDTH;
		b = voop;
		voop += density / 3;
		for (a = 0; a < density/8; a++)
		{
			unsigned int offs = (a+b) % FIREWIDTH;
			unsigned int v = M_Random();
			v = from[offs] + 4 + (v & 15) + (v >> 3) + (M_Random() & 31);
			if (v > 255)
				v = 255;
			from[offs] = from[FIREWIDTH*2 + (offs + FIREWIDTH*3/2)%FIREWIDTH] = v;
		}

		density += 10;
		if (density > FIREWIDTH*7)
			density = FIREWIDTH*7;

		from = burnarray;
		for (b = 0; b <= FIREHEIGHT; b += 2)
		{
			byte *pixel = from;

			// special case: first pixel on line
			byte *p = pixel + (FIREWIDTH << 1);
			unsigned int top = *p + *(p + FIREWIDTH - 1) + *(p + 1);
			unsigned int bottom = *(pixel + (FIREWIDTH << 2));
			unsigned int c1 = (top + bottom) >> 2;
			if (c1 > 1) c1--;
			*pixel = c1;
			*(pixel + FIREWIDTH) = (c1 + bottom) >> 1;
			pixel++;

			// main line loop
			for (a = 1; a < FIREWIDTH-1; a++)
			{
				// sum top pixels
				p = pixel + (FIREWIDTH << 1);
				top = *p + *(p - 1) + *(p + 1);

				// bottom pixel
				bottom = *(pixel + (FIREWIDTH << 2));

				// combine pixels
				c1 = (top + bottom) >> 2;
				if (c1 > 1) c1--;

				// store pixels
				*pixel = c1;
				*(pixel + FIREWIDTH) = (c1 + bottom) >> 1;		// interpolate

				// next pixel
				pixel++;
			}

			// special case: last pixel on line
			p = pixel + (FIREWIDTH << 1);
			top = *p + *(p - 1) + *(p - FIREWIDTH + 1);
			bottom = *(pixel + (FIREWIDTH << 2));
			c1 = (top + bottom) >> 2;
			if (c1 > 1) c1--;
			*pixel = c1;
			*(pixel + FIREWIDTH) = (c1 + bottom) >> 1;

			// next line
			from += FIREWIDTH << 1;
		}
	}

	// check if done
	if (burntime > 40)
		return true;

	int x, y;
	fixed_t firex, firey;
	const fixed_t xstep = (FIREWIDTH * FRACUNIT) / screen->width;
	const fixed_t ystep = (FIREHEIGHT * FRACUNIT) / screen->height;

	for (y = 0, firey = 0; y < screen->height; y++, firey += ystep)
	{
		for (x = 0, firex = 0; x < screen->width; x++, firex += xstep)
		{
			int fglevel = burnarray[(firex>>FRACBITS)+(firey>>FRACBITS)*FIREWIDTH] / 2;

			if (fglevel < 63)
				return false;
		}
	}

	return true;
}

template <typename PIXEL_T>
static inline void Wipe_DrawBurnGeneric()
{
	const int pitch = screen->pitch / sizeof(PIXEL_T);
	PIXEL_T* to = (PIXEL_T*)screen->buffer;
	const PIXEL_T* from = (PIXEL_T*)wipe_screen;

	fixed_t firex, firey;
	int x, y;

	const fixed_t xstep = (FIREWIDTH * FRACUNIT) / screen->width;
	const fixed_t ystep = (FIREHEIGHT * FRACUNIT) / screen->height;

	for (y = 0, firey = 0; y < screen->height; y++, firey += ystep)
	{
		for (x = 0, firex = 0; x < screen->width; x++, firex += xstep)
		{
			int fglevel;

			fglevel = burnarray[(firex>>FRACBITS)+(firey>>FRACBITS)*FIREWIDTH] / 2;

			if (fglevel > 0 && fglevel < 63)
			{
				int bglevel = 64 - fglevel;
				to[x] = rt_blend2<PIXEL_T>(from[x], bglevel << 2, to[x], fglevel << 2);
			}
			else if (fglevel == 0)
			{
				to[x] = from[x];
			}
		}

		from += screen->width;
		to += pitch;
	}
} 

static void Wipe_DrawBurn()
{
	if (screen->is8bit())
		Wipe_DrawBurnGeneric<palindex_t>();
	else
		Wipe_DrawBurnGeneric<argb_t>();
}


// Crossfade --------------------------------------------------------

static int fade = 0;

static void Wipe_StartFade()
{
	fade = 0;
	screen->GetBlock(0, 0, screen->width, screen->height, wipe_screen);
}

static void Wipe_StopFade()
{
}

static bool Wipe_TickFade()
{
	fade += 2;
	return (fade > 64);
}

template <typename PIXEL_T>
static inline void Wipe_DrawFadeGeneric()
{
	const int pitch = screen->pitch / sizeof(PIXEL_T);
	PIXEL_T* to = (PIXEL_T*)screen->buffer;
	const PIXEL_T* from = (PIXEL_T*)wipe_screen;

	const fixed_t bglevel = MAX(64 - fade, 0);

	for (int y = 0; y < screen->height; y++)
	{
		for (int x = 0; x < screen->width; x++)
			to[x] = rt_blend2<PIXEL_T>(from[x], bglevel << 2, to[x], fade << 2);

		from += screen->width;
		to += pitch;
	}
}

static void Wipe_DrawFade()
{
	if (screen->is8bit())
		Wipe_DrawFadeGeneric<palindex_t>();
	else
		Wipe_DrawFadeGeneric<argb_t>();
}


// General Wipe Functions -------------------------------------------

static void (*wipe_start_func)() = NULL;
static void (*wipe_stop_func)() = NULL;
static bool (*wipe_tick_func)() = NULL;
static void (*wipe_draw_func)() = NULL;

//
// Wipe_Stop
//
// Performs cleanup following the completion of the wipe animation.
//
void Wipe_Stop()
{
	in_progress = false;

	if (wipe_stop_func)
		wipe_stop_func();

	if (wipe_screen)
	{
		delete [] wipe_screen;
		wipe_screen = NULL;
	}
}

//
// Wipe_Start
//
// Initializes the function pointers for the wiping animation system.
//
void Wipe_Start()
{
	if (in_progress)
		Wipe_Stop();

	if (r_wipetype.asInt() < 0 || r_wipetype.asInt() >= int(wipe_NUMWIPES))
		current_wipe_type = wipe_Melt;
	else
		current_wipe_type = static_cast<wipe_type_t>(r_wipetype.asInt());

	if (current_wipe_type == wipe_None)
		return;

	if (current_wipe_type == wipe_Melt)
	{
		wipe_start_func = Wipe_StartMelt;
		wipe_stop_func = Wipe_StopMelt;
		wipe_tick_func = Wipe_TickMelt;
		wipe_draw_func = Wipe_DrawMelt;
	}
	else if (current_wipe_type == wipe_Burn)
	{
		wipe_start_func = Wipe_StartBurn;
		wipe_stop_func = Wipe_StopBurn;
		wipe_tick_func = Wipe_TickBurn;
		wipe_draw_func = Wipe_DrawBurn;
	}
	else if (current_wipe_type == wipe_Fade)
	{
		wipe_start_func = Wipe_StartFade;
		wipe_stop_func = Wipe_StopFade;
		wipe_tick_func = Wipe_TickFade;
		wipe_draw_func = Wipe_DrawFade;
	}	

	//  allocate data for the temporary screens
	int pixel_size = screen->is8bit() ? sizeof(palindex_t) : sizeof(argb_t);
	wipe_screen = new byte[screen->width * screen->height * pixel_size];
	
	in_progress = true;
	if (wipe_start_func)
		wipe_start_func();
}

//
// Wipe_Ticker
//
// Advances the wipe animation by one tick. This returns true
// when the animation has completed. When completed, the cleanup is performed
// and in_progress is set to false to indicate to the other wiping functions
// that the resources are no longer valid.
//
bool Wipe_Ticker()
{
	if (!in_progress)
		return true;

	bool done = true;
	if (wipe_tick_func)
		done = wipe_tick_func();

	if (done)
		Wipe_Stop();

	return done;
}

//
// Wipe_Drawer
//
// Renders the wipe animation independent of the ticker. This allows the video
// framerate to be uncapped while the animation speed moves at the consistent
// 35Hz ticrate.
//
void Wipe_Drawer()
{
	if (in_progress)
	{
		if (wipe_draw_func)
			wipe_draw_func();	
		V_MarkRect(0, 0, screen->width, screen->height);
	}
}

VERSION_CONTROL (f_wipe_cpp, "$Id$")

