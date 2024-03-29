// Emacs style mode select   -*- C++ -*-
//-----------------------------------------------------------------------------
//
// $Id$
//
// Copyright (C) 2006-2012 by The Odamex Team.
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
//  Player list control class
//
//-----------------------------------------------------------------------------

#include <wx/fileconf.h>
#include <wx/xrc/xmlres.h>
#include <wx/log.h>
#include <wx/dcmemory.h>

#include "lst_players.h"
#include "str_utils.h"

using namespace odalpapi;

IMPLEMENT_DYNAMIC_CLASS(LstOdaPlayerList, wxAdvancedListCtrl)

BEGIN_EVENT_TABLE(LstOdaPlayerList, wxAdvancedListCtrl)

	EVT_WINDOW_CREATE(LstOdaPlayerList::OnCreateControl)
END_EVENT_TABLE()

typedef enum
{
	playerlist_field_attr
	,playerlist_field_name
	,playerlist_field_ping
	,playerlist_field_timeingame
	,playerlist_field_frags
	,playerlist_field_kdrcount
	,playerlist_field_killcount
	,playerlist_field_deathcount
	,playerlist_field_team
	,playerlist_field_teamscore

	,max_playerlist_fields
} playerlist_fields_t;

static int ImageList_Spectator = -1;
static int ImageList_RedBullet = -1;
static int ImageList_BlueBullet = -1;

// Special case
static wxInt32 WidthTeam, WidthTeamScore;

bool LstOdaPlayerList::CreatePlayerIcon(const wxColour& In, wxBitmap& Out)
{
	wxMemoryDC DC;
	wxPoint Point(0,0);
	wxSize Size = Out.GetSize();

	if(!In.IsOk() || !Out.IsOk())
		return false;

	DC.SelectObject(Out);
	DC.SetBackground(*wxBLACK_BRUSH);
	DC.Clear();

	DC.SetBrush(*wxTheBrushList->FindOrCreateBrush(In, wxSOLID));
	DC.DrawRectangle(Point, Size);

	return true;
}

void LstOdaPlayerList::OnCreateControl(wxWindowCreateEvent& event)
{
	SetupPlayerListColumns();

	// Propagate the event to the base class as well
	event.Skip();
}

void LstOdaPlayerList::SetupPlayerListColumns()
{
	DeleteAllItems();
	DeleteAllColumns();
	ClearImageList();

	wxFileConfig ConfigInfo;
	wxInt32 PlayerListSortOrder, PlayerListSortColumn;

	// Read from the global configuration
	wxInt32 WidthAttr, WidthName, WidthPing, WidthFrags, WidthKDRCount,
	        WidthKillCount, WidthDeathCount, WidthTime;

	//ConfigInfo.Read("PlayerListWidthName"), &WidthName, 150);
	WidthAttr = 24; // fixed column size
	ConfigInfo.Read("PlayerListWidthName", &WidthName, 150);
	ConfigInfo.Read("PlayerListWidthPing", &WidthPing, 60);
	ConfigInfo.Read("PlayerListWidthFrags", &WidthFrags, 70);
	ConfigInfo.Read("PlayerListWidthKDRCount", &WidthKDRCount, 85);
	ConfigInfo.Read("PlayerListWidthKillCount", &WidthKillCount, 85);
	ConfigInfo.Read("PlayerListWidthDeathCount", &WidthDeathCount, 100);
	ConfigInfo.Read("PlayerListWidthTime", &WidthTime, 150);
	ConfigInfo.Read("PlayerListWidthTeam", &WidthTeam, 65);
	ConfigInfo.Read("PlayerListWidthTeamScore", &WidthTeamScore, 100);

	InsertColumn(playerlist_field_attr,
	             "",
	             wxLIST_FORMAT_LEFT,
	             WidthAttr);

	// We sort by the numerical value of the item data field, so we can sort
	// spectators/downloaders etc
	SetSortColumnIsSpecial((wxInt32)playerlist_field_attr);

	InsertColumn(playerlist_field_name,
	             "Player name",
	             wxLIST_FORMAT_LEFT,
	             WidthName);

	InsertColumn(playerlist_field_ping,
	             "Ping",
	             wxLIST_FORMAT_LEFT,
	             WidthPing);

	InsertColumn(playerlist_field_frags,
	             "Frags",
	             wxLIST_FORMAT_LEFT,
	             WidthFrags);

	InsertColumn(playerlist_field_killcount,
	             "K/D Ratio",
	             wxLIST_FORMAT_LEFT,
	             WidthKDRCount);

	InsertColumn(playerlist_field_killcount,
	             "Kill count",
	             wxLIST_FORMAT_LEFT,
	             WidthKillCount);

	InsertColumn(playerlist_field_deathcount,
	             "Death count",
	             wxLIST_FORMAT_LEFT,
	             WidthDeathCount);

	InsertColumn(playerlist_field_timeingame,
	             "Time (HH:MM)",
	             wxLIST_FORMAT_LEFT,
	             WidthTime);

	// Sorting info
	ConfigInfo.Read("PlayerListSortOrder", &PlayerListSortOrder, 1);
	ConfigInfo.Read("PlayerListSortColumn", &PlayerListSortColumn, 0);

	SetSortColumnAndOrder(PlayerListSortColumn, PlayerListSortOrder);

	ImageList_Spectator = AddImageSmall(wxXmlResource::Get()->LoadBitmap("spectator").ConvertToImage());
	ImageList_BlueBullet = AddImageSmall(wxXmlResource::Get()->LoadBitmap("bullet_blue").ConvertToImage());
	ImageList_RedBullet = AddImageSmall(wxXmlResource::Get()->LoadBitmap("bullet_red").ConvertToImage());
}

LstOdaPlayerList::~LstOdaPlayerList()
{
	wxFileConfig ConfigInfo;
	wxInt32 PlayerListSortOrder, PlayerListSortColumn;
	wxListItem li;

	// Write to the global configuration

	// Sorting info
	GetSortColumnAndOrder(PlayerListSortColumn, PlayerListSortOrder);

	ConfigInfo.Write("PlayerListSortOrder", PlayerListSortOrder);
	ConfigInfo.Write("PlayerListSortColumn", PlayerListSortColumn);

	wxInt32 WidthName, WidthPing, WidthFrags, WidthKillCount, WidthDeathCount,
	        WidthTime;

	WidthName = GetColumnWidth(playerlist_field_name);
	WidthPing = GetColumnWidth(playerlist_field_ping);
	WidthFrags = GetColumnWidth(playerlist_field_frags);
	WidthKillCount = GetColumnWidth(playerlist_field_killcount);
	WidthDeathCount = GetColumnWidth(playerlist_field_deathcount);
	WidthTime = GetColumnWidth(playerlist_field_timeingame);

	ConfigInfo.Write("PlayerListWidthName", WidthName);
	ConfigInfo.Write("PlayerListWidthPing", WidthPing);
	ConfigInfo.Write("PlayerListWidthFrags", WidthFrags);
	ConfigInfo.Write("PlayerListWidthKillCount", WidthKillCount);
	ConfigInfo.Write("PlayerListWidthDeathCount", WidthDeathCount);
	ConfigInfo.Write("PlayerListWidthTime", WidthTime);

	// Team and Team Scores are shown dynamically, so handle the case of them
	// not existing
	if(!GetColumn((int)playerlist_field_team, li) ||
	        !GetColumn((int)playerlist_field_teamscore, li))
	{
		return;
	}

	WidthTeam = GetColumnWidth(playerlist_field_team);
	WidthTeamScore = GetColumnWidth(playerlist_field_teamscore);

	ConfigInfo.Write("PlayerListWidthTeam", WidthTeam);
	ConfigInfo.Write("PlayerListWidthTeamScore", WidthTeamScore);
}

void LstOdaPlayerList::AddPlayersToList(const Server& s)
{
	SetupPlayerListColumns();

	if(s.Info.GameType == GT_TeamDeathmatch ||
	        s.Info.GameType == GT_CaptureTheFlag)
	{
		InsertColumn(playerlist_field_team,
		             "Team",
		             wxLIST_FORMAT_LEFT,
		             WidthTeam);

		InsertColumn(playerlist_field_teamscore,
		             "Team Score",
		             wxLIST_FORMAT_LEFT,
		             WidthTeamScore);
	}

	size_t PlayerCount = s.Info.Players.size();

	if(!PlayerCount)
		return;

	for(size_t i = 0; i < PlayerCount; ++i)
	{
		wxListItem li;

		float kdr;
		wxString Time;

		li.m_itemId = ALCInsertItem();

		li.SetMask(wxLIST_MASK_TEXT);

		li.SetColumn(playerlist_field_name);
		li.SetText(stdstr_towxstr(s.Info.Players[i].Name));

		SetItem(li);

		li.SetColumn(playerlist_field_ping);

		li.SetText(wxString::Format("%d",
		                            s.Info.Players[i].Ping));

		SetItem(li);

		li.SetColumn(playerlist_field_frags);
		li.SetText(wxString::Format("%d",
		                            s.Info.Players[i].Frags));

		SetItem(li);

		if(s.Info.Players[i].Frags <= 0)
			kdr = 0;
		else if(s.Info.Players[i].Frags >= 1 && !s.Info.Players[i].Deaths)
			kdr = (float)s.Info.Players[i].Frags;
		else
			kdr = (float)s.Info.Players[i].Frags / (float)s.Info.Players[i].Deaths;

		li.SetColumn(playerlist_field_kdrcount);
		li.SetText(wxString::Format("%2.1f", kdr));

		SetItem(li);

		li.SetColumn(playerlist_field_killcount);
		li.SetText(wxString::Format("%d",
		                            s.Info.Players[i].Kills));

		SetItem(li);

		li.SetColumn(playerlist_field_deathcount);
		li.SetText(wxString::Format("%d",
		                            s.Info.Players[i].Deaths));

		SetItem(li);


		if(s.Info.Players[i].Time)
		{
			Time = wxString::Format("%.2d:%.2d",
			                        s.Info.Players[i].Time / 60, s.Info.Players[i].Time % 60);
		}
		else
			Time = "00:00";

		li.SetColumn(playerlist_field_timeingame);
		li.SetText(Time);

		SetItem(li);

		// Draw an icon for the players selected colour
		wxUint32 PlayerColour = s.Info.Players[i].Colour;
		wxUint8 PC_Red = 0, PC_Green = 0, PC_Blue = 0;

		PC_Red = ((PlayerColour >> 16) & 0x00FFFFFF);
		PC_Green = ((PlayerColour >> 8) & 0x0000FFFF);
		PC_Blue = (PlayerColour & 0x000000FF);

		wxBitmap Out(16,16);
		wxColour In(PC_Red, PC_Green, PC_Blue);

		if(!CreatePlayerIcon(In, Out))
			wxLogError("Player list icon drawing failed");

		int idx = AddImageSmall(Out.ConvertToImage());

		SetItemColumnImage(li.m_itemId, playerlist_field_name,
		                   idx);

		if(s.Info.GameType == GT_TeamDeathmatch ||
		        s.Info.GameType == GT_CaptureTheFlag)
		{
			wxUint8 TeamId;
			wxString TeamName = "Unknown";
			wxUint32 TeamColour = 0;
			wxInt16 TeamScore = 0;

			li.SetColumn(playerlist_field_team);

			// Player has a team index it is associated with
			TeamId = s.Info.Players[i].Team;

			// Acquire info about the team the player is in
			// TODO: Hack alert, we only accept blue and red teams, since
			// dynamic teams are not implemented and GOLD/NONE teams exist
			// Just accept these 2 for now
			if((TeamId == 0) || (TeamId == 1))
			{
				wxUint8 TC_Red = 0, TC_Green = 0, TC_Blue = 0;

				TeamName = stdstr_towxstr(s.Info.Teams[TeamId].Name);
				TeamColour = s.Info.Teams[TeamId].Colour;
				TeamScore = s.Info.Teams[TeamId].Score;

				TC_Red = ((TeamColour >> 16) & 0x00FFFFFF);
				TC_Green = ((TeamColour >> 8) & 0x0000FFFF);
				TC_Blue = (TeamColour & 0x000000FF);

				li.SetTextColour(wxColour(TC_Red, TC_Green, TC_Blue));
			}
			else
				TeamId = 3;

			li.SetText(TeamName);

			SetItem(li);

			// Set the team colour and bullet icon for the player
			switch(TeamId)
			{
			case 0:
			{
				SetItemColumnImage(li.m_itemId, playerlist_field_team,
				                   ImageList_BlueBullet);
			}
			break;

			case 1:
			{
				SetItemColumnImage(li.m_itemId, playerlist_field_team,
				                   ImageList_RedBullet);
			}
			break;

			default:
			{
				li.SetTextColour(*wxBLACK);
			}
			break;
			}

			li.SetColumn(playerlist_field_teamscore);

			li.SetText(wxString::Format("%d", TeamScore));

			SetItem(li);
		}

		// Icons
		// -----

		bool IsSpectator = s.Info.Players[i].Spectator;

		// Magnifying glass icon for spectating players
		SetItemColumnImage(li.m_itemId, playerlist_field_attr,
		                   IsSpectator ? ImageList_Spectator : -1);

		// Allows us to sort by spectators
		SetItemData(li.m_itemId, (IsSpectator ? 1 : 0));
	}

	Sort();
}
