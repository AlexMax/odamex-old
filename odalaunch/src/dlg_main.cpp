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
//	User interface
//	AUTHOR:	Russell Rice, John D Corrado
//
//-----------------------------------------------------------------------------


#include "dlg_main.h"
#include "query_thread.h"

#include <wx/settings.h>
#include <wx/menu.h>
#include <wx/statusbr.h>
#include <wx/msgdlg.h>
#include <wx/utils.h>
#include <wx/tipwin.h>
#include <wx/recguard.h>
#include <wx/app.h>

#include "misc.h"

// Control ID assignments for events
// application icon

// lists
static wxInt32 ID_LSTSERVERS = XRCID("ID_LSTSERVERS");
static wxInt32 ID_LSTPLAYERS = XRCID("ID_LSTPLAYERS");

// menus
static wxInt32 ID_MNUCONMAN = XRCID("ID_MNUCONMAN");
static wxInt32 ID_MNUSERVERS = XRCID("ID_MNUSERVERS");

static wxInt32 ID_MNULAUNCH = XRCID("ID_MNULAUNCH");
static wxInt32 ID_MNUQLAUNCH = XRCID("ID_MNUQLAUNCH");
static wxInt32 ID_MNUGETLIST = XRCID("ID_MNUGETLIST");
static wxInt32 ID_MNUREFRESHSERVER = XRCID("ID_MNUREFRESHSERVER");
static wxInt32 ID_MNUREFRESHALL = XRCID("ID_MNUREFRESHALL");
static wxInt32 ID_MNUABOUT = XRCID("ID_MNUABOUT");
static wxInt32 ID_MNUSETTINGS = XRCID("ID_MNUSETTINGS");
static wxInt32 ID_MNUEXIT = XRCID("ID_MNUEXIT");

static wxInt32 ID_MNUWEBSITE = XRCID("ID_MNUWEBSITE");
static wxInt32 ID_MNUFORUM = XRCID("ID_MNUFORUM");
static wxInt32 ID_MNUWIKI = XRCID("ID_MNUWIKI");
static wxInt32 ID_MNUCHANGELOG = XRCID("ID_MNUCHANGELOG");
static wxInt32 ID_MNUREPORTBUG = XRCID("ID_MNUREPORTBUG");

// custom events
DEFINE_EVENT_TYPE(wxEVT_THREAD_MONITOR_SIGNAL)
DEFINE_EVENT_TYPE(wxEVT_THREAD_WORKER_SIGNAL)

// Event handlers
BEGIN_EVENT_TABLE(dlgMain,wxFrame)
	EVT_MENU(wxID_EXIT, dlgMain::OnExit)
	
	// menu item events
    EVT_MENU(ID_MNUSERVERS, dlgMain::OnMenuServers)
    EVT_MENU(ID_MNUCONMAN, dlgMain::OnManualConnect)

	EVT_MENU(ID_MNULAUNCH, dlgMain::OnLaunch)
	EVT_MENU(ID_MNUQLAUNCH, dlgMain::OnQuickLaunch)

	EVT_MENU(ID_MNUGETLIST, dlgMain::OnGetList)
	EVT_MENU(ID_MNUREFRESHSERVER, dlgMain::OnRefreshServer)
	EVT_MENU(ID_MNUREFRESHALL, dlgMain::OnRefreshAll)

	EVT_MENU(ID_MNUABOUT, dlgMain::OnAbout)
	EVT_MENU(ID_MNUSETTINGS, dlgMain::OnOpenSettingsDialog)

	EVT_MENU(ID_MNUWEBSITE, dlgMain::OnOpenWebsite)
	EVT_MENU(ID_MNUFORUM, dlgMain::OnOpenForum)
	EVT_MENU(ID_MNUWIKI, dlgMain::OnOpenWiki)
    EVT_MENU(ID_MNUCHANGELOG, dlgMain::OnOpenChangeLog)
    EVT_MENU(ID_MNUREPORTBUG, dlgMain::OnOpenReportBug)

    // thread events
    EVT_COMMAND(-1, wxEVT_THREAD_MONITOR_SIGNAL, dlgMain::OnMonitorSignal)    
    EVT_COMMAND(-1, wxEVT_THREAD_WORKER_SIGNAL, dlgMain::OnWorkerSignal)  

    // misc events
    EVT_LIST_ITEM_SELECTED(ID_LSTSERVERS, dlgMain::OnServerListClick)
    EVT_LIST_ITEM_ACTIVATED(ID_LSTSERVERS, dlgMain::OnServerListDoubleClick)
    EVT_LIST_ITEM_RIGHT_CLICK(ID_LSTSERVERS, dlgMain::OnServerListRightClick)
END_EVENT_TABLE()

// Main window creation
dlgMain::dlgMain(wxWindow* parent, wxWindowID id)
{
    launchercfg_s.get_list_on_start = 1;
    launchercfg_s.show_blocked_servers = 1;
    launchercfg_s.wad_paths = wxGetCwd();
    launchercfg_s.odamex_directory = wxGetCwd();

	wxXmlResource::Get()->LoadFrame(this, parent, _T("dlgMain")); 
  
    SERVER_LIST = wxStaticCast(FindWindow(ID_LSTSERVERS), wxAdvancedListCtrl);
    PLAYER_LIST = wxStaticCast(FindWindow(ID_LSTPLAYERS), wxAdvancedListCtrl);

    // spectator states.
    PLAYER_LIST->AssignImageList(new wxImageList(16, 15), wxIMAGE_LIST_SMALL);
    PLAYER_LIST->GetImageList(wxIMAGE_LIST_SMALL)->Add(wxArtProvider::GetBitmap(wxART_GO_FORWARD));
    PLAYER_LIST->GetImageList(wxIMAGE_LIST_SMALL)->Add(wxArtProvider::GetBitmap(wxART_FIND));   

	// set up the master server information
	MServer = new MasterServer;
    
    /* Init sub dialogs and load settings */
    config_dlg = new dlgConfig(&launchercfg_s, this);
    server_dlg = new dlgServers(MServer, this);

	// set up the list controls
    SERVER_LIST->InsertColumn(0,_T("Server name"),wxLIST_FORMAT_LEFT,150);
	SERVER_LIST->InsertColumn(1,_T("Ping"),wxLIST_FORMAT_LEFT,50);
	SERVER_LIST->InsertColumn(2,_T("Players"),wxLIST_FORMAT_LEFT,50);
	SERVER_LIST->InsertColumn(3,_T("WADs"),wxLIST_FORMAT_LEFT,150);
	SERVER_LIST->InsertColumn(4,_T("Map"),wxLIST_FORMAT_LEFT,50);
	SERVER_LIST->InsertColumn(5,_T("Type"),wxLIST_FORMAT_LEFT,80);
	SERVER_LIST->InsertColumn(6,_T("Game IWAD"),wxLIST_FORMAT_LEFT,80);
	SERVER_LIST->InsertColumn(7,_T("Address : Port"),wxLIST_FORMAT_LEFT,130);
    
    QServer = NULL;

    // Create monitor thread and run it
    if (this->wxThreadHelper::Create() != wxTHREAD_NO_ERROR)
    {
        wxMessageBox(_T("Could not create monitor thread!"), 
                     _T("Error"), 
                     wxOK | wxICON_ERROR);
                     
        wxExit();
    }
    
    GetThread()->Run();
    
    // get master list on application start
    if (launchercfg_s.get_list_on_start)
    {
        wxCommandEvent event(wxEVT_COMMAND_TOOL_CLICKED, ID_MNUGETLIST);
    
        wxPostEvent(this, event);
    }
}

// Window Destructor
dlgMain::~dlgMain()
{
    // Close our monitor thread
    mtcs_Request.Signal = mtcs_exit;
    GetThread()->Wait();

    if (MServer != NULL)
        delete MServer;
        
    if (QServer != NULL)
        delete[] QServer;
        
    if (config_dlg != NULL)
        config_dlg->Destroy();

    if (server_dlg != NULL)
        server_dlg->Destroy();
}

void dlgMain::OnExit(wxCommandEvent& event)
{
    Close();
}

// manually connect to a server
void dlgMain::OnManualConnect(wxCommandEvent &event)
{
    wxString ted_result = _T("");
    wxTextEntryDialog ted(this, 
                            _T("Please enter IP Address and Port"), 
                            _T("Please enter IP Address and Port"),
                            _T("0.0.0.0:0"));
                            
    // show it
    ted.ShowModal();
    ted_result = ted.GetValue();
    
    if (!ted_result.IsEmpty() && ted_result != _T("0.0.0.0:0"))
        LaunchGame(ted_result, 
                    launchercfg_s.odamex_directory, 
                    launchercfg_s.wad_paths);
}

// [Russell] - Monitor thread entry point
void *dlgMain::Entry()
{
    bool Running = true;
           
    while (Running)
    {
        // Can I order a master server request with a list of server addresses
        // to go with that kthx?
        if (mtcs_Request.Signal == mtcs_getmaster)
        {
            static const wxString masters[2] = 
            {
                _T("odamex.net"),
                _T("odamex.org")
            };
            
            static int index = 0;
            
            mtcs_Request.Signal = mtcs_getservers;
          
            MServer->SetAddress(masters[index], 15000);

            // TODO: Clean this up
            if (!MServer->Query(9999))
            {
                index = !index;
        
                MServer->SetAddress(masters[index], 15000);
           
                if (!MServer->Query(9999))
                {
                    mtrs_struct_t *Result = new mtrs_struct_t;

                    Result->Signal = mtrs_master_timeout;                
                    Result->Index = -1;
                    Result->ServerListIndex = -1;
                    
                    wxCommandEvent event(wxEVT_THREAD_MONITOR_SIGNAL, -1);
                    event.SetClientData(Result);
                  
                    wxPostEvent(this, event); 
                }
                else
                {      
                    mtrs_struct_t *Result = new mtrs_struct_t;

                    Result->Signal = mtrs_master_success;                
                    Result->Index = -1;
                    Result->ServerListIndex = -1;
                
                    wxCommandEvent event(wxEVT_THREAD_MONITOR_SIGNAL, -1);
                    event.SetClientData(Result);
                  
                    wxPostEvent(this, event);                     
                }
            }
            else
            {                 
                mtrs_struct_t *Result = new mtrs_struct_t;

                Result->Signal = mtrs_master_success;                
                Result->Index = -1;
                Result->ServerListIndex = -1;
                
                wxCommandEvent event(wxEVT_THREAD_MONITOR_SIGNAL, -1);
                event.SetClientData(Result);
                  
                wxPostEvent(this, event);               
            }

            if (QServer != NULL && MServer->GetServerCount())
            {
                delete[] QServer;
            
                QServer = new Server [MServer->GetServerCount()];
            }
            else
                QServer = new Server [MServer->GetServerCount()];

        }
    
        // get a new list of servers
        if (mtcs_Request.Signal == mtcs_getservers)
        {
            wxInt32 count = 0;
            wxInt32 serverNum = 0;
            std::vector<QueryThread*> threadVector;
            wxString Address = _T("");
            wxUint16 Port = 0;
   
            mtcs_Request.Signal = mtcs_none;

            // [Russell] - This includes custom servers.
            if (!MServer->GetServerCount())
            {
                mtrs_struct_t *Result = new mtrs_struct_t;

                Result->Signal = mtrs_server_noservers;                
                Result->Index = -1;
                Result->ServerListIndex = -1;
                
                wxCommandEvent event(wxEVT_THREAD_MONITOR_SIGNAL, -1);
                event.SetClientData(Result);
                  
                wxPostEvent(this, event);                  
            }

            /* 
                Thread pool manager:
                Executes a number of threads that contain the same amount of
                servers, when a thread finishes, it gets deleted and another
                gets executed with a different server, eventually all the way
                down to 0 servers.
            */
            while(count < MServer->GetServerCount())
            {
                for(wxInt32 i = 0; i < NUM_THREADS; i++)
                {
                    if((threadVector.size() != 0) && ((threadVector.size() - 1) >= i))
                    {
                        // monitor our thread vector, delete ONLY if the thread is
                        // finished
                        if(threadVector[i]->IsRunning())
                            continue;
                        else
                        {
                            threadVector[i]->Wait();
                            delete threadVector[i];
                            threadVector.erase(threadVector.begin() + i);
                            count++;
                        }
                    }
                    if(serverNum < MServer->GetServerCount())
                    {
                        MServer->GetServerAddress(serverNum, Address, Port);
                        QServer[serverNum].SetAddress(Address, Port);

                        // add the thread to the vector
                        threadVector.push_back(new QueryThread(this, &QServer[serverNum], serverNum));

                        // create and run the thread
                        if(threadVector[threadVector.size() - 1]->Create() == wxTHREAD_NO_ERROR)
                            threadVector[threadVector.size() - 1]->Run();

                        // DUMB: our next server will be this incremented value
                        serverNum++;
                    }

                    GetThread()->Sleep(1);            
                }
                // Give everything else some cpu time, please be considerate!
                GetThread()->Sleep(1);              
            }
        }
        
        // User requested single server to be refreshed
        if (mtcs_Request.Signal == mtcs_getsingleserver)
        {            
            mtcs_Request.Signal = mtcs_none;

            if (MServer->GetServerCount())
            if (QServer[mtcs_Request.Index].Query(300))
            {
                mtrs_struct_t *Result = new mtrs_struct_t;

                Result->Signal = mtrs_server_singlesuccess;                
                Result->Index = mtcs_Request.Index;
                Result->ServerListIndex = mtcs_Request.ServerListIndex;
                
                wxCommandEvent event(wxEVT_THREAD_MONITOR_SIGNAL, -1);
                event.SetClientData(Result);
                    
                wxPostEvent(this, event);      
            }
            else
            {
                mtrs_struct_t *Result = new mtrs_struct_t;

                Result->Signal = mtrs_server_singletimeout;                
                Result->Index = mtcs_Request.Index;
                Result->ServerListIndex = mtcs_Request.ServerListIndex;
                
                wxCommandEvent event(wxEVT_THREAD_MONITOR_SIGNAL, mtrs_server_singletimeout);
                event.SetClientData(Result);
                    
                wxPostEvent(this, event);    
            }                     
        }
    
        // Give everything else some cpu time, please be considerate!
        GetThread()->Sleep(1);
               
        // Something requested the thread to exit
        if (mtcs_Request.Signal == mtcs_exit || Running == false)
            break;
    }
    
    return NULL;
}

void dlgMain::OnMonitorSignal(wxCommandEvent& event)
{
    mtrs_struct_t *Result = (mtrs_struct_t *)event.GetClientData();
    wxInt32 i;
    
    switch (Result->Signal)
    {
        case mtrs_master_timeout:
            if (!MServer->GetServerCount())           
                break;
        case mtrs_master_success:
            break;
        case mtrs_server_noservers:
            wxMessageBox(_T("There are no servers to query"), _T("Error"), wxOK | wxICON_ERROR);
            break;
        case mtrs_server_singletimeout:
            i = FindServerInList(QServer[Result->Index].GetAddress());

            PLAYER_LIST->DeleteAllItems();
            
            QServer[Result->Index].ResetData();
            
            if (launchercfg_s.show_blocked_servers)
            if (i == -1)
                AddServerToList(SERVER_LIST, QServer[Result->Index], Result->Index);
            else
                AddServerToList(SERVER_LIST, QServer[Result->Index], i, 0);
            
            break;
        case mtrs_server_singlesuccess:
            PLAYER_LIST->DeleteAllItems();
            
            AddServerToList(SERVER_LIST, QServer[Result->Index], Result->ServerListIndex, 0);
            
            AddPlayersToList(PLAYER_LIST, QServer[Result->Index]);
            
            TotalPlayers += QServer[Result->Index].info.numplayers;
           
            break;
        default:
            break;
    }

    GetStatusBar()->SetStatusText(wxString::Format(_T("Master Ping: %u"), MServer->GetPing()), 1);
    GetStatusBar()->SetStatusText(wxString::Format(_T("Total Players: %d"), TotalPlayers), 3);

    delete Result;
}

// worker threads post to this callback
void dlgMain::OnWorkerSignal(wxCommandEvent& event)
{
    wxInt32 i;
    switch (event.GetId())
    {
        case 0: // server query timed out
        {
            i = FindServerInList(QServer[event.GetInt()].GetAddress());

            PLAYER_LIST->DeleteAllItems();
            
            QServer[event.GetInt()].ResetData();
            
            if (launchercfg_s.show_blocked_servers)
            if (i == -1)
                AddServerToList(SERVER_LIST, QServer[event.GetInt()], event.GetInt());
            else
                AddServerToList(SERVER_LIST, QServer[event.GetInt()], i, 0);
            
            break;                 
        }
        case 1: // server queried successfully
        {
            AddServerToList(SERVER_LIST, QServer[event.GetInt()], event.GetInt());
            
            TotalPlayers += QServer[event.GetInt()].info.numplayers;
            
            break;      
        }
    }

    ++QueriedServers;
    
    GetStatusBar()->SetStatusText(wxString::Format(_T("Queried Server %d of %d"), 
                                                   QueriedServers, 
                                                   MServer->GetServerCount()), 
                                                   2);
                                                   
    GetStatusBar()->SetStatusText(wxString::Format(_T("Total Players: %d"), 
                                                   TotalPlayers), 
                                                   3);   
}

// display extra information for a server
void dlgMain::OnServerListRightClick(wxListEvent& event)
{
    if (!SERVER_LIST->GetItemCount() || !SERVER_LIST->GetSelectedItemCount())
        return;
  
    wxInt32 i = SERVER_LIST->GetNextItem(-1, 
                                        wxLIST_NEXT_ALL, 
                                        wxLIST_STATE_SELECTED);
        
    wxListItem item;
    item.SetId(i);
    item.SetColumn(7);
    item.SetMask(wxLIST_MASK_TEXT);
        
    SERVER_LIST->GetItem(item);
        
    i = FindServer(item.GetText());

    static wxString text = _T("");
    
    text = wxString::Format(_T("Extra information:\n\n"
                              "Protocol version: %d\n"
                              "Email: %s\n"
                              "Website: %s\n\n"
                              "Timeleft: %d\n"
                              "Timelimit: %d\n"
                              "Fraglimit: %d\n\n"
                              "Item respawn: %s\n"
                              "Weapons stay: %s\n"
                              "Friendly fire: %s\n"
                              "Allow exiting: %s\n"
                              "Infinite ammo: %s\n"
                              "No monsters: %s\n"
                              "Monsters respawn: %s\n"
                              "Fast monsters: %s\n"
                              "Allow jumping: %s\n"
                              "Allow freelook: %s\n"
                              "WAD downloading: %s\n"
                              "Empty reset: %s\n"
                              "Clean maps: %s\n"
                              "Frag on exit: %s\n"
                              "Spectating: %s\n"),
                              QServer[i].info.version,
                              
                              QServer[i].info.emailaddr.c_str(),
                              QServer[i].info.webaddr.c_str(),
                              QServer[i].info.timeleft,
                              QServer[i].info.timelimit,
                              QServer[i].info.fraglimit,
                              
                              BOOLSTR(QServer[i].info.itemrespawn),
                              BOOLSTR(QServer[i].info.weaponstay),
                              BOOLSTR(QServer[i].info.friendlyfire),
                              BOOLSTR(QServer[i].info.allowexit),
                              BOOLSTR(QServer[i].info.infiniteammo),
                              BOOLSTR(QServer[i].info.nomonsters),
                              BOOLSTR(QServer[i].info.monstersrespawn),
                              BOOLSTR(QServer[i].info.fastmonsters),
                              BOOLSTR(QServer[i].info.allowjump),
                              BOOLSTR(QServer[i].info.allowfreelook),
                              BOOLSTR(QServer[i].info.waddownload),
                              BOOLSTR(QServer[i].info.emptyreset),
                              BOOLSTR(QServer[i].info.cleanmaps),
                              BOOLSTR(QServer[i].info.fragonexit),
                              BOOLSTR(QServer[i].info.spectating));
    
    static wxTipWindow *tw = NULL;
                              
    if (tw)
	{
		tw->SetTipWindowPtr(NULL);
		tw->Close();
	}
	
	tw = NULL;

	if (!text.empty())
		tw = new wxTipWindow(SERVER_LIST, text, 120, &tw);
}


// Custom Servers menu item
void dlgMain::OnMenuServers(wxCommandEvent &event)
{
    if (server_dlg)
        server_dlg->Show();
}


void dlgMain::OnOpenSettingsDialog(wxCommandEvent &event)
{
    if (config_dlg)
        config_dlg->Show();
}

// About information
void dlgMain::OnAbout(wxCommandEvent& event)
{
    wxString strAbout = _T("Odamex Launcher 1.0 - "
                            "Copyright 2007 The Odamex Team");
    
    wxMessageBox(strAbout, strAbout);
}

// Quick-Launch button click
void dlgMain::OnQuickLaunch(wxCommandEvent &event)
{
	LaunchGame(_T(""), 
				launchercfg_s.odamex_directory, 
				launchercfg_s.wad_paths);
}

// Launch button click
void dlgMain::OnLaunch(wxCommandEvent &event)
{
    if (!SERVER_LIST->GetItemCount() || !SERVER_LIST->GetSelectedItemCount())
        return;
        
    wxInt32 i = SERVER_LIST->GetNextItem(-1, wxLIST_NEXT_ALL, wxLIST_STATE_SELECTED);
        
    wxListItem item;
    item.SetId(i);
    item.SetColumn(7);
    item.SetMask(wxLIST_MASK_TEXT);
        
    SERVER_LIST->GetItem(item);
        
    i = FindServer(item.GetText()); 
       
    if (i > -1)
    {
        LaunchGame(QServer[i].GetAddress(), 
                    launchercfg_s.odamex_directory, 
                    launchercfg_s.wad_paths);
    }
}

// Get Master List button click
void dlgMain::OnGetList(wxCommandEvent &event)
{
    // prevent reentrancy
    static wxRecursionGuardFlag s_rgf;
    wxRecursionGuard recursion_guard(s_rgf);
    
    if (recursion_guard.IsInside())
        return;	
	
    SERVER_LIST->DeleteAllItems();
    PLAYER_LIST->DeleteAllItems();
        
    QueriedServers = 0;
    TotalPlayers = 0;
	
	mtcs_Request.Signal = mtcs_getmaster;
}

void dlgMain::OnRefreshServer(wxCommandEvent &event)
{   
    // prevent reentrancy
    static wxRecursionGuardFlag s_rgf;
    wxRecursionGuard recursion_guard(s_rgf);
    
    if (recursion_guard.IsInside())
        return;	
    
    if (!SERVER_LIST->GetItemCount() || !SERVER_LIST->GetSelectedItemCount())
        return;
        
    PLAYER_LIST->DeleteAllItems();
        
    wxInt32 listindex = SERVER_LIST->GetNextItem(-1, wxLIST_NEXT_ALL, wxLIST_STATE_SELECTED);
        
    wxListItem item;
    item.SetId(listindex);
    item.SetColumn(7);
    item.SetMask(wxLIST_MASK_TEXT);
        
    SERVER_LIST->GetItem(item);
        
    wxInt32 arrayindex = FindServer(item.GetText()); 
        
    if (arrayindex == -1)
        return;
                
    TotalPlayers -= QServer[arrayindex].info.numplayers;
    
    mtcs_Request.Signal = mtcs_getsingleserver;
    mtcs_Request.ServerListIndex = listindex;
    mtcs_Request.Index = arrayindex; 
}

void dlgMain::OnRefreshAll(wxCommandEvent &event)
{
    // prevent reentrancy
    static wxRecursionGuardFlag s_rgf;
    wxRecursionGuard recursion_guard(s_rgf);
    
    if (recursion_guard.IsInside())
        return;	

    if (!MServer->GetServerCount())
        return;
        
    SERVER_LIST->DeleteAllItems();
    PLAYER_LIST->DeleteAllItems();
    
    QueriedServers = 0;
    TotalPlayers = 0;
    
    mtcs_Request.Signal = mtcs_getservers;
    mtcs_Request.ServerListIndex = -1;
    mtcs_Request.Index = -1; 
}

// when the user clicks on the server list
void dlgMain::OnServerListClick(wxListEvent& event)
{
    // clear any tooltips remaining
    SERVER_LIST->SetToolTip(_T(""));
    
    if ((SERVER_LIST->GetItemCount() > 0) && (SERVER_LIST->GetSelectedItemCount() == 1))
    {
        PLAYER_LIST->DeleteAllItems();
        
        wxInt32 i = SERVER_LIST->GetNextItem(-1, wxLIST_NEXT_ALL, wxLIST_STATE_SELECTED);
        
        wxListItem item;
        item.SetId(i);
        item.SetColumn(7);
        item.SetMask(wxLIST_MASK_TEXT);
        
        SERVER_LIST->GetItem(item);
        
        i = FindServer(item.GetText()); 
        
        if (i > -1)
            AddPlayersToList(PLAYER_LIST, QServer[i]);
    }
}

// when the user double clicks on the server list
void dlgMain::OnServerListDoubleClick(wxListEvent& event)
{
    if ((SERVER_LIST->GetItemCount() > 0) && (SERVER_LIST->GetSelectedItemCount() == 1))
    {
        wxCommandEvent event(wxEVT_COMMAND_TOOL_CLICKED, ID_MNULAUNCH);
    
        wxPostEvent(this, event);
    }
}

// returns a index of the server address as the internal array index
wxInt32 dlgMain::FindServer(wxString Address)
{
    for (wxInt32 i = 0; i < MServer->GetServerCount(); i++)
        if (QServer[i].GetAddress().IsSameAs(Address))
            return i;
    
    return -1;
}

// Finds an index in the server list, via Address
wxInt32 dlgMain::FindServerInList(wxString Address)
{
    if (!SERVER_LIST->GetItemCount())
        return -1;
    
    for (wxInt32 i = 0; i < SERVER_LIST->GetItemCount(); i++)
    {
        wxListItem item;
        item.SetId(i);
        item.SetColumn(7);
        item.SetMask(wxLIST_MASK_TEXT);
        
        SERVER_LIST->GetItem(item);
        
        if (item.GetText().IsSameAs(Address))
            return i;
    }
    
    return -1;
}

void dlgMain::OnOpenWebsite(wxCommandEvent &event)
{
    wxLaunchDefaultBrowser(_T("http://odamex.net"));
}

void dlgMain::OnOpenForum(wxCommandEvent &event)
{
    wxLaunchDefaultBrowser(_T("http://odamex.net/boards"));
}

void dlgMain::OnOpenWiki(wxCommandEvent &event)
{
    wxLaunchDefaultBrowser(_T("http://odamex.net/wiki"));
}

void dlgMain::OnOpenChangeLog(wxCommandEvent &event)
{
    wxLaunchDefaultBrowser(_T("http://odamex.net/changelog"));
}

void dlgMain::OnOpenReportBug(wxCommandEvent &event)
{
    wxLaunchDefaultBrowser(_T("http://odamex.net/bugs/enter_bug.cgi"));
}
