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
//  all external data is defined here
//  most of the data is loaded into different structures at run time
//  some internal structures shared by many modules are here
//
//-----------------------------------------------------------------------------


#ifndef __DOOMDATA__
#define __DOOMDATA__

// The most basic types we use, portability.
#include "doomtype.h"

// Some global defines, that configure the game.
#include "doomdef.h"



//
// Map level types.
// The following data structures define the persistent format
// used in the lumps of the WAD files.
//

// Lump order in a map WAD: each map needs a couple of lumps
// to provide a complete scene geometry description.
enum
{
	ML_LABEL, 			// A separator, name, ExMx or MAPxx
	ML_THINGS,			// Monsters, items
	ML_LINEDEFS,		// LineDefs, from editing
	ML_SIDEDEFS,		// SideDefs, from editing
	ML_VERTEXES,		// Vertices, edited and BSP splits generated
	ML_SEGS,			// LineSegs, from LineDefs split by BSP
	ML_SSECTORS,		// SubSectors, list of LineSegs
	ML_NODES, 			// BSP nodes
	ML_SECTORS,			// Sectors, from editing
	ML_REJECT,			// LUT, sector-sector visibility
	ML_BLOCKMAP,			// LUT, motion clipping, walls/grid element
	ML_BEHAVIOR			// [RH] Hexen-style scripts. If present, THINGS
						//		and LINEDEFS are also Hexen-style.	
};


// A single Vertex.
typedef struct
{
	short		x;
	short		y;
} mapvertex_t;

// A SideDef, defining the visual appearance of a wall,
// by setting textures and offsets.
typedef struct
{
	short	textureoffset;
	short	rowoffset;
	char	toptexture[8];
	char	bottomtexture[8];
	char	midtexture[8];
	short	sector;	// Front sector, towards viewer.
} mapsidedef_t;

// A LineDef, as used for editing, and as input to the BSP builder.
typedef struct
{
	unsigned short	v1;
	unsigned short	v2;
	short	flags;
	short	special;
	short	tag;
	// sidenum[1] will be -1 if one sided
	short		sidenum[2];		
} maplinedef_t;

// A ZDoom style LineDef, from the Doom Wiki.
typedef struct
{
	unsigned short	v1;
	unsigned short	v2;
	short	flags;
	byte	special;
	byte	args[5];
	// sidenum[1] will be -1 if one sided
	short		sidenum[2];		
} maplinedef2_t;

//
// LineDef attributes.
//

#define ML_BLOCKING			0x0001	// solid, is an obstacle
#define ML_BLOCKMONSTERS	0x0002	// blocks monsters only
#define ML_TWOSIDED			0x0004	// backside will not be present at all if not two sided

// If a texture is pegged, the texture will have
// the end exposed to air held constant at the
// top or bottom of the texture (stairs or pulled
// down things) and will move with a height change
// of one of the neighbor sectors.
// Unpegged textures always have the first row of
// the texture at the top pixel of the line for both
// top and bottom textures (use next to windows).


#define ML_DONTPEGTOP		0x0008	// upper texture unpegged
#define ML_DONTPEGBOTTOM	0x0010	// lower texture unpegged
#define ML_SECRET			0x0020	// don't map as two sided: IT'S A SECRET!
#define ML_SOUNDBLOCK		0x0040	// don't let sound cross two of these
#define ML_DONTDRAW 		0x0080	// don't draw on the automap
#define ML_MAPPED			0x0100	// set if already drawn in automap
#define ML_REPEAT_SPECIAL	0x0200	// special is repeatable
#define ML_SPAC_SHIFT		10
#define ML_SPAC_MASK		0x1c00
#define GET_SPAC(flags)		((flags&ML_SPAC_MASK)>>ML_SPAC_SHIFT)

// Special activation types
#define SPAC_CROSS		0	// when player crosses line
#define SPAC_USE		1	// when player uses line
#define SPAC_MCROSS		2	// when monster crosses line
#define SPAC_IMPACT		3	// when projectile hits line
#define SPAC_PUSH		4	// when player/monster pushes line
#define SPAC_PCROSS		5	// when projectile crosses line
#define SPAC_USETHROUGH	6	// SPAC_USE, but passes it through
#define SPAC_CROSSTHROUGH 7 // SPAC_CROSS, but passes it through

// [RH] Monsters (as well as players) can active the line
#define ML_MONSTERSCANACTIVATE		0x2000

// [RH] BOOM's ML_PASSUSE flag (conflicts with ML_REPEATSPECIAL)
#define ML_PASSUSE_BOOM				0x0200

// [RH] Line blocks everything
#define ML_BLOCKEVERYTHING			0x8000

// Sector definition, from editing
typedef struct
{
	short	floorheight;
	short	ceilingheight;
	char	floorpic[8];
	char	ceilingpic[8];
	short	lightlevel;
	short	special;
	short	tag;
} mapsector_t;

// SubSector, as generated by BSP
typedef struct
{
	short	numsegs;
	short	firstseg;	// index of first one, segs are stored sequentially
} mapsubsector_t;


// LineSeg, generated by splitting LineDefs
// using partition lines selected by BSP builder.
typedef struct
{
	short	v1;
	short	v2;
	short	angle;
	short	linedef;
	short	side;
	short	offset;
} mapseg_t;



// BSP node structure.

typedef struct
{
  // Partition line from (x,y) to x+dx,y+dy)
	short		x;
	short		y;
	short		dx;
	short		dy;
	
  // Bounding box for each child,
  // clip against view frustum.
	short		bbox[2][4];
	
  // If top bit is set, it's a subsector,
  // else it's a node of another subtree.
	unsigned short	children[2];
	
} mapnode_t;

// Thing definition, position, orientation and type,
// plus skill/visibility flags and attributes.
typedef struct
{
	short		x;
	short		y;
	short		angle;
	short		type;
	short		options;
} mapthing_t;

// [RH] Hexen-compatible MapThing.
typedef struct MapThing
{
	unsigned short thingid;
	short		x;
	short		y;
	short		z;
	short		angle;
	short		type;
	short		flags;
	byte		special;
	byte		args[5];

	void Serialize (FArchive &);
} mapthing2_t;


// [RH] MapThing flags.

/*
#define MTF_EASY			0x0001	// Thing will appear on easy skill setting
#define MTF_MEDIUM			0x0002	// Thing will appear on medium skill setting
#define MTF_HARD			0x0004	// Thing will appear on hard skill setting
#define MTF_AMBUSH			0x0008	// Thing is deaf
*/
#define MTF_DORMANT			0x0010	// Thing is dormant (use Thing_Activate)
#define MTF_SINGLE			0x0100	// Thing appears in single-player games
#define MTF_COOPERATIVE		0x0200	// Thing appears in cooperative games
#define MTF_DEATHMATCH		0x0400	// Thing appears in deathmatch games


// BOOM and DOOM compatible versions of some of the above

#define BTF_NOTSINGLE		0x0010	// (TF_COOPERATIVE|TF_DEATHMATCH)
#define BTF_NOTDEATHMATCH	0x0020	// (TF_SINGLE|TF_COOPERATIVE)
#define BTF_NOTCOOPERATIVE	0x0040	// (TF_SINGLE|TF_DEATHMATCH)

//
// Texture definition.
// Each texture is composed of one or more patches,
// with patches being lumps stored in the WAD.
// The lumps are referenced by number, and patched
// into the rectangular texture space using origin
// and possibly other attributes.
//
typedef struct
{
	short	originx;
	short	originy;
	short	patch;
	short	stepdir;
	short	colormap;
} mappatch_t;

//
// Texture definition.
// A DOOM wall texture is a list of patches
// which are to be combined in a predefined order.
//
typedef struct
{
	char		name[8];
	WORD		masked;				// [RH] Unused
	BYTE		scalex;				// [RH] Scaling (8 is normal)
	BYTE		scaley;				// [RH] Same as above
	short		width;
	short		height;
	byte		columndirectory[4];	// OBSOLETE
	short		patchcount;
	mappatch_t	patches[1];
} maptexture_t;




#endif					// __DOOMDATA__


