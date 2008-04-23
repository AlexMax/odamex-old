// Emacs style mode select   -*- C++ -*- 
//-----------------------------------------------------------------------------
//
// $Id:$
//
// Copyright (C) 2006-2007 by The Odamex Team.
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
//	Miscellaneous stuff for gui
//	AUTHOR:	Russell Rice, John D Corrado
//
//-----------------------------------------------------------------------------


#include "lst_custom.h"

#include <wx/settings.h>
#include <wx/menu.h>
#include <wx/statusbr.h>
#include <wx/msgdlg.h>
#include <wx/colour.h>

#include "net_packet.h"
#include "misc.h"

/*
    Takes a server structure and adds it to the list control
    if insert is 1, then add an item to the list, otherwise it will
    update the current item with new data
*/
void AddServerToList(wxListCtrl *list, Server &s, wxInt32 index, wxInt8 insert)
{
    wxInt32 i = 0, idx = 0;
    
    if (s.GetAddress() == _T("0.0.0.0:0"))
        return;
    
    wxListItem li;

    li.SetTextColour(*wxBLACK);
    li.SetBackgroundColour(*wxWHITE);
    
    li.SetMask(wxLIST_MASK_TEXT);
    
    // are we adding a new item?
    if (insert)    
    {
        li.SetColumn(0);
        li.SetText(s.info.name);
        
        idx = list->InsertItem(li);
    }
    else
    {
        idx = index;
        // update server name if we have a blank one
        list->SetItem(idx, 0, s.info.name, -1);
    }
    
    wxUint32 ping = s.GetPing();
    
    list->SetItem(idx, 7, s.GetAddress());
    
    list->SetItem(idx, 1, wxString::Format(_T("%u"),ping));   
    list->SetItem(idx, 2, wxString::Format(_T("%d/%d"),s.info.numplayers,s.info.maxplayers));       
    
    // build a list of pwads
    if (s.info.numpwads)
    {
        // pwad list
        wxString wadlist = _T("");
        wxString pwad = _T("");
            
        for (i = 0; i < s.info.numpwads; i++)
        {
            pwad = s.info.pwads[i].Mid(0, s.info.pwads[i].Find('.'));
            wadlist += wxString::Format(_T("%s "), pwad.c_str());
        }
            
        list->SetItem(idx, 3, wadlist);
    }
            
    list->SetItem(idx, 4, s.info.map.Upper());
    
    // what game type do we like to play
    wxString gmode = _T("");

    if (s.info.gametype == 0)
        gmode = _T("COOP");
    else if (s.info.gametype == 1)
        gmode = _T("DM");
    if(s.info.gametype && s.info.teamplay)
        gmode = _T("TEAM DM");
    if(s.info.ctf)
        gmode = _T("CTF");
              
    list->SetItem(idx, 5, gmode);

    // trim off the .wad
    wxString iwad = s.info.iwad.Mid(0, s.info.iwad.Find('.'));
        
    list->SetItem(idx, 6, iwad);
}

typedef enum
{
    playerlist_field_name
    ,playerlist_field_ping
    ,playerlist_field_timeingame
    ,playerlist_field_frags
    ,playerlist_field_killcount
    ,playerlist_field_deathcount
    ,playerlist_field_team
    ,playerlist_field_teamscore
    
    ,max_playerlist_fields
} playerlist_fields_t;

void AddPlayersToList(wxAdvancedListCtrl *list, Server &s)
{   
	list->DeleteAllColumns();
	
	list->InsertColumn(playerlist_field_name,_T("Player name"),wxLIST_FORMAT_LEFT,150);
	list->InsertColumn(playerlist_field_ping,_T("Ping"),wxLIST_FORMAT_LEFT,50);
	list->InsertColumn(playerlist_field_frags,_T("Frags"),wxLIST_FORMAT_LEFT,70);
    list->InsertColumn(playerlist_field_killcount,_T("Kill count"),wxLIST_FORMAT_LEFT,60);
    list->InsertColumn(playerlist_field_deathcount,_T("Death count"),wxLIST_FORMAT_LEFT,80);
    list->InsertColumn(playerlist_field_timeingame,_T("Time"),wxLIST_FORMAT_LEFT,50);

    if (s.info.teamplay)
    {
        list->InsertColumn(playerlist_field_team,
                           _T("Team"),
                           wxLIST_FORMAT_LEFT,
                           50);
        
        list->InsertColumn(playerlist_field_teamscore,
                           _T("Team score"),
                           wxLIST_FORMAT_LEFT,
                           80);
    }
    
    if (!s.info.numplayers)
        return;
        
    for (wxInt32 i = 0; i < s.info.numplayers; i++)
    {
        wxListItem li;
        
        li.SetColumn(playerlist_field_name);
        
        li.SetTextColour(*wxBLACK);
        li.SetBackgroundColour(*wxWHITE);
        
        li.SetMask(wxLIST_MASK_TEXT);
        
        if (s.info.spectating)
        {
            li.SetMask(wxLIST_MASK_TEXT | wxLIST_MASK_IMAGE);
            
            li.SetText(s.info.playerinfo[i].name);
            li.SetImage(s.info.playerinfo[i].spectator);
        }
        else
        {
            li.SetText(s.info.playerinfo[i].name);
        }
        
        li.SetId(list->InsertItem(li));
        
        li.SetColumn(playerlist_field_ping);
        li.SetMask(wxLIST_MASK_TEXT);

        li.SetText(wxString::Format(_T("%d"),
                                    s.info.playerinfo[i].ping));
        
        list->SetItem(li);
        
        li.SetColumn(playerlist_field_frags);        
        li.SetText(wxString::Format(_T("%d"),
                                    s.info.playerinfo[i].frags));
        
        list->SetItem(li);

        li.SetColumn(playerlist_field_killcount);        
        li.SetText(wxString::Format(_T("%d"),
                                    s.info.playerinfo[i].killcount));
        
        list->SetItem(li);

        li.SetColumn(playerlist_field_deathcount);        
        li.SetText(wxString::Format(_T("%d"),
                                    s.info.playerinfo[i].deathcount));
        
        list->SetItem(li);

        li.SetColumn(playerlist_field_timeingame);        
        li.SetText(wxString::Format(_T("%d"),
                                    s.info.playerinfo[i].timeingame));
        
        list->SetItem(li);
        
        if (s.info.teamplay)
		{
            wxString teamstr = _T("UNKNOWN");
            wxInt32 teamscore = 0;
            
            li.SetColumn(playerlist_field_team); 
            
            li.SetBackgroundColour(*wxWHITE);
            li.SetTextColour(*wxBLACK);
            
            switch(s.info.playerinfo[i].team)
			{
                case 0:
                    li.SetBackgroundColour(*wxBLUE);
                    li.SetTextColour(*wxWHITE);      
                    teamstr = _T("BLUE");
                    teamscore = s.info.teamplayinfo.bluescore;
                    break;
				case 1:
                    li.SetBackgroundColour(*wxRED);
                    li.SetTextColour(*wxWHITE);
                    teamstr = _T("RED");
					teamscore = s.info.teamplayinfo.redscore;
					break;
				case 2:
                    // no gold in 'dem mountains boy.
                    li.SetBackgroundColour(wxColor(255,200,40));
                    li.SetTextColour(*wxBLACK);
                    teamscore = s.info.teamplayinfo.goldscore;
                    teamstr = _T("GOLD");
					break;
				default:
					break;
			}

            li.SetText(teamstr);

            list->SetItem(li);
            
            li.SetColumn(playerlist_field_teamscore);
            
            li.SetText(wxString::Format(_T("%d/%d"), 
                                        teamscore, 
                                        s.info.teamplayinfo.scorelimit));
            
            list->SetItem(li);
        }
            
    }
    
}

wxString *CheckPWADS(wxString pwads, wxString waddirs)
{
    wxStringTokenizer wads(pwads, _T(' '));
    wxStringTokenizer dirs(waddirs, _T(' '));
    wxString wadfn = _T("");
    
    // validity array counter
    wxUint32 i = 0;
    
    // allocate enough space for all wads
    wxString *inv_wads = new wxString [wads.CountTokens()];
    
    if (!inv_wads)
        return NULL;
       
    // begin checking
    while (dirs.HasMoreTokens())
    {
        while (wads.HasMoreTokens())
        {
            #ifdef __WXMSW__
            wadfn = wxString::Format(_T("%s\%s"), dirs.GetNextToken().c_str(), wads.GetNextToken().c_str());
            #else
            wadfn = wxString::Format(_T("%s/%s"), dirs.GetNextToken().c_str(), wads.GetNextToken().c_str());
            #endif
            
            if (wxFileExists(wadfn))
            {
                inv_wads[i] = wadfn;                

                i++;                
            }
        }
    }
    
    if (i)
        return inv_wads;
    else
    {
        delete[] inv_wads;
        
        return NULL;
    }
}

void LaunchGame(wxString Address, wxString ODX_Path, wxString waddirs)
{
    if (ODX_Path.IsEmpty())
    {
        wxMessageBox(_T("Your Odamex path is empty!"));
        
        return;
    }
    
    #ifdef __WXMSW__
    wxString binname = ODX_Path + _T('\\') + _T("odamex");
    #else
    wxString binname = ODX_Path + _T("/odamex");
    #endif

    wxString cmdline = _T("");

    // when adding waddir string, return 1 less, to get rid of extra delimiter
    wxString dirs = waddirs.Mid(0, waddirs.Length() - 1);
    
    cmdline += wxString::Format(_T("%s"), binname.c_str());
    
    if (!Address.IsEmpty())
		cmdline += wxString::Format(_T(" -connect %s"),
									Address.c_str());
	
	// this is so the client won't mess up parsing
	if (!dirs.IsEmpty())
        cmdline += wxString::Format(_T(" -waddir \"%s\""), 
                                    dirs.c_str());

	if (wxExecute(cmdline, wxEXEC_ASYNC, NULL) == -1)
        wxMessageBox(wxString::Format(_T("Could not start %s!"), 
                                        binname.c_str()));
	
}
