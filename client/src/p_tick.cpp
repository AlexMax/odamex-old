// Emacs style mode select   -*- C++ -*- 
//-----------------------------------------------------------------------------
//
// $Id$
//
// Copyright (C) 1993-1996 by id Software, Inc.
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
//	Ticker.
//
//-----------------------------------------------------------------------------


#include "z_zone.h"
#include "p_local.h"
#include "c_console.h"
#include "doomstat.h"
#include "cl_main.h"

extern constate_e ConsoleState;
extern gamestate_t wipegamestate;


void P_XYMovement (AActor *mo);
void P_ZMovement (AActor *mo);

//
// P_Ticker
//
void P_Ticker (void)
{
	if(serverside)
	{
		for(size_t i = 0; i < players.size(); i++)
			if(players[i].ingame())
				P_PlayerThink (&players[i]);
	}
	else if (noservermsgs && !demoplayback)
		return;

    DThinker::RunThinkers ();
	
	P_UpdateSpecials ();
	P_RespawnSpecials ();

	// for par times
	level.time++;
}

VERSION_CONTROL (p_tick_cpp, "$Id$")


