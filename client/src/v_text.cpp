// Emacs style mode select   -*- C++ -*- 
//-----------------------------------------------------------------------------
//
// $Id$
//
// Copyright (C) 1993-1996 by id Software, Inc.
// Copyright (C) 2006-2014 by The Odamex Team.
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
//	V_TEXT
//
//-----------------------------------------------------------------------------


#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>

#include "v_text.h"

#include "i_system.h"
#include "i_video.h"
#include "v_video.h"
#include "hu_stuff.h"
#include "w_wad.h"
#include "z_zone.h"
#include "m_swap.h"

#include "doomstat.h"

EXTERN_CVAR(msg0color)
EXTERN_CVAR(msg1color)
EXTERN_CVAR(msg2color)
EXTERN_CVAR(msg3color)
EXTERN_CVAR(msg4color)

EXTERN_CVAR(hud_scaletext)

extern patch_t *hu_font[HU_FONTSIZE];


byte *ConChars;

extern byte *Ranges;

int V_TextScaleXAmount()
{
	return int(hud_scaletext);
}

int V_TextScaleYAmount()
{
	return int(hud_scaletext);
}


//
// V_GetTextColor
//
// Decodes a \c escape sequence and returns the index of the appropriate
// color translation to use. This assumes that str is at least three characters
// in length.
//
int V_GetTextColor(const char* str)
{
	static int table[128];
	static bool initialized = false;

	if (!initialized)
	{
		for (int i = 0; i < 128; i++)
			table[i] = -1;

		table['A'] = table['a'] = CR_BRICK;
		table['B'] = table['b'] = CR_TAN;
		table['C'] = table['c'] = CR_GRAY;
		table['D'] = table['d'] = CR_GREEN;
		table['E'] = table['e'] = CR_BROWN;
		table['F'] = table['f'] = CR_GOLD;
		table['G'] = table['g'] = CR_RED;
		table['H'] = table['h'] = CR_BLUE;
		table['I'] = table['i'] = CR_ORANGE;
		table['J'] = table['j'] = CR_WHITE;
		table['K'] = table['k'] = CR_YELLOW;


		initialized = true;
	}

	if (str[0] == '\\' && str[1] == 'c' && str[2] < 128)
	{
		int c = str[2];
		if (c == '-')
			return CR_GRAY;			// use print color
		if (c == '+')
			return CR_GREEN;		// use print bold color
		if (c == '*')
			return msg3color;		// use chat color
		if (c == '!')
			return msg4color;		// use team chat color

		return table[c];
	}
	return -1;
}

//
// V_PrintStr
// Print a line of text using the console font
//
void DCanvas::PrintStr(int x, int y, const char* str, int count) const
{
	const int default_color = CR_GRAY;
	translationref_t trans = translationref_t(Ranges + default_color * 256);

	if (!buffer)
		return;

	if (y > (height - 8) || y < 0)
		return;

	if (x < 0)
	{
		int skip;

		skip = -(x - 7) / 8;
		x += skip * 8;
		if (count <= skip)
			return;

		count -= skip;
		str += skip;
	}

	x &= ~3;
	byte* destline = buffer + y * pitch;

	while (count && x <= (width - 8))
	{
	    // john - tab 4 spaces
	    if (*str == '\t')
	    {
	        str++;
	        count--;
	        x += 8 * 4;
	        continue;
	    }

		// [SL] parse color escape codes (\cX)
		if (count >= 3 && str[0] == '\\' && str[1] == 'c')
		{
			int new_color = V_GetTextColor(str);
			if (new_color == -1)
				new_color = default_color; 

			trans = translationref_t(Ranges + new_color * 256);

			str += 3;
			count -= 3;
			continue;
		}

		int c = *(byte*)str;

		if (is8bit())
		{
			const byte* source = (byte*)&ConChars[c * 128];
			palindex_t* dest = (palindex_t*)(destline + x);
			for (int z = 0; z < 8; z++)
			{
				for (int a = 0; a < 8; a++)
				{
					const palindex_t mask = source[a+8];
					palindex_t color = trans.tlate(source[a]);
					dest[a] = (dest[a] & mask) ^ color;
				}

				dest += pitch;
				source += 16;
			}
		}
		else
		{
			byte* source = (byte*)&ConChars[c * 128];
			argb_t* dest = (argb_t*)(destline + (x << 2));
			for (int z = 0; z < 8; z++)
			{
				for (int a = 0; a < 8; a++)
				{
					const argb_t mask = (source[a+8] << 24) | (source[a+8] << 16)
										| (source[a+8] << 8) | source[a+8];

					argb_t color = V_Palette.shade(trans.tlate(source[a])) & ~mask;
					dest[a] = (dest[a] & mask) ^ color; 
				}

				dest += pitch >> 2;
				source += 16;
			}
		}

		str++;
		count--;
		x += 8;
	}
}

//
// V_DrawText
//
// Write a string using the hu_font
//

void DCanvas::TextWrapper (EWrapperCode drawer, int normalcolor, int x, int y, const byte *string) const
{
	int 		w;
	const byte *ch;
	int 		c;
	int 		cx;
	int 		cy;
	int			boldcolor;

	if (normalcolor > NUM_TEXT_COLORS)
		normalcolor = CR_RED;
	boldcolor = normalcolor ? normalcolor - 1 : NUM_TEXT_COLORS - 1;

	V_ColorMap = translationref_t(Ranges + normalcolor * 256);

	ch = string;
	cx = x;
	cy = y;

	while (1)
	{
		c = *ch++;
		if (!c)
			break;

		if (c == 0x8a)
		{
			int newcolor = toupper(*ch++);

			if (newcolor == 0)
			{
				return;
			}
			else if (newcolor == '-')
			{
				newcolor = normalcolor;
			}
			else if (newcolor >= 'A' && newcolor < 'A' + NUM_TEXT_COLORS)
			{
				newcolor -= 'A';
			}
			else if (newcolor == '+')
			{
				newcolor = boldcolor;
			}
			else
			{
				continue;
			}
			V_ColorMap = translationref_t(Ranges + newcolor * 256);
			continue;
		}

		if (c == '\n')
		{
			cx = x;
			cy += 9;
			continue;
		}

		c = toupper(c) - HU_FONTSTART;
		if (c < 0 || c>= HU_FONTSIZE)
		{
			cx += 4;
			continue;
		}

		w = hu_font[c]->width();
		if (cx+w > width)
			break;

		DrawWrapper (drawer, hu_font[c], cx, cy);
		cx+=w;
	}
}

void DCanvas::TextSWrapper (EWrapperCode drawer, int normalcolor, int x, int y, const byte *string) const
{
	TextSWrapper(drawer, normalcolor, x, y, string, CleanXfac, CleanYfac);
}

void DCanvas::TextSWrapper (EWrapperCode drawer, int normalcolor, int x, int y, 
							const byte *string, int scalex, int scaley) const
{
	int 		w;
	const byte *ch;
	int 		c;
	int 		cx;
	int 		cy;
	int			boldcolor;

	if (normalcolor > NUM_TEXT_COLORS)
		normalcolor = CR_RED;
	boldcolor = normalcolor ? normalcolor - 1 : NUM_TEXT_COLORS - 1;

	V_ColorMap = translationref_t(Ranges + normalcolor * 256);

	ch = string;
	cx = x;
	cy = y;

	while (1)
	{
		c = *ch++;
		if (!c)
			break;

		if (c == 0x8a)
		{
			int newcolor = toupper(*ch++);

			if (newcolor == 0)
			{
				return;
			}
			else if (newcolor == '-')
			{
				newcolor = normalcolor;
			}
			else if (newcolor >= 'A' && newcolor < 'A' + NUM_TEXT_COLORS)
			{
				newcolor -= 'A';
			}
			else if (newcolor == '+')
			{
				newcolor = boldcolor;
			}
			else
			{
				continue;
			}
			V_ColorMap = translationref_t(Ranges + newcolor * 256);
			continue;
		}

		if (c == '\n')
		{
			cx = x;
			cy += 9 * scalex;
			continue;
		}

		c = toupper(c) - HU_FONTSTART;
		if (c < 0 || c>= HU_FONTSIZE)
		{
			cx += 4 * scaley;
			continue;
		}

		w = hu_font[c]->width() * scalex;
		if (cx+w > width)
			break;

        DrawSWrapper (drawer, hu_font[c], cx, cy,
                        hu_font[c]->width() * scalex,
                        hu_font[c]->height() * scaley);

		cx+=w;
	}
}

//
// Find string width from hu_font chars
//
int V_StringWidth (const byte *string)
{
	int w = 0, c;
	
	if(!string)
		return 0;

	while (*string)
	{
		if (*string == 0x8a)
		{
			if (*(++string))
				string++;
			continue;
		}
		else
		{
			c = toupper((*string++) & 0x7f) - HU_FONTSTART;
			if (c < 0 || c >= HU_FONTSIZE)
			{
				w += 4;
			}
			else
			{
				w += hu_font[c]->width();
			}
		}
	}

	return w;
}

//
// Break long lines of text into multiple lines no longer than maxwidth pixels
//
static void breakit (brokenlines_t *line, const byte *start, const byte *string)
{
	// Leave out trailing white space
	while (string > start && isspace (*(string - 1)))
		string--;

	line->string = new char[string - start + 1];
	strncpy (line->string, (char *)start, string - start);
	line->string[string - start] = 0;
	line->width = V_StringWidth (line->string);
}

brokenlines_t *V_BreakLines (int maxwidth, const byte *string)
{
	brokenlines_t lines[128];	// Support up to 128 lines (should be plenty)

	const byte *space = NULL, *start = string;
	int i, c, w, nw;
	BOOL lastWasSpace = false;

	i = w = 0;

	while ( (c = *string++) ) {
		if (c == 0x8a) {
			if (*string)
				string++;
			continue;
		}

		if (isspace(c)) {
			if (!lastWasSpace) {
				space = string - 1;
				lastWasSpace = true;
			}
		} else
			lastWasSpace = false;

		c = toupper (c & 0x7f) - HU_FONTSTART;

		if (c < 0 || c >= HU_FONTSIZE)
			nw = 4;
		else
			nw = hu_font[c]->width();

		if (w + nw > maxwidth || c == '\n' - HU_FONTSTART) {	// Time to break the line
			if (!space)
				space = string - 1;

			breakit (&lines[i], start, space);

			i++;
			w = 0;
			lastWasSpace = false;
			start = space;
			space = NULL;

			while (*start && isspace (*start) && *start != '\n')
				start++;
			if (*start == '\n')
				start++;
			else
				while (*start && isspace (*start))
					start++;
			string = start;
		} else
			w += nw;
	}

	if (string - start > 1) {
		const byte *s = start;

		while (s < string) {
			if (!isspace (*s++)) {
				breakit (&lines[i++], start, string);
				break;
			}
		}
	}

	{
		// Make a copy of the broken lines and return them
		brokenlines_t *broken = new brokenlines_t[i+1];

		memcpy (broken, lines, sizeof(brokenlines_t) * i);
		broken[i].string = NULL;
		broken[i].width = -1;

		return broken;
	}
}

void V_FreeBrokenLines (brokenlines_t *lines)
{
	if (lines)
	{
		int i = 0;

		while (lines[i].width != -1)
		{
			delete[] lines[i].string;
			lines[i].string = NULL;
			i++;
		}
		delete[] lines;
	}
}


VERSION_CONTROL (v_text_cpp, "$Id$")

