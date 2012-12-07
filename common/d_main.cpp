// Emacs style mode select   -*- C++ -*-
//-----------------------------------------------------------------------------
//
// $Id: d_main.cpp 3426 2012-11-19 17:25:28Z dr_sean $
//
// Copyright (C) 1993-1996 by id Software, Inc.
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
//	Common functions to determine game mode (shareware, registered),
//	parse command line parameters, and handle wad changes.
//
//-----------------------------------------------------------------------------

#include "version.h"

#include <sstream>
#include <string>
#include <vector>
#include <algorithm>

#ifdef WIN32
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#else
#include <sys/stat.h>
#endif

#ifdef UNIX
#include <unistd.h>
#include <dirent.h>
#endif

#include <stdlib.h>

#include "errors.h"

#include "m_alloc.h"
#include "doomdef.h"
#include "gstrings.h"
#include "z_zone.h"
#include "w_wad.h"
#include "m_argv.h"
#include "m_fileio.h"
#include "c_console.h"
#include "i_system.h"
#include "g_game.h"
#include "p_setup.h"
#include "r_local.h"
#include "d_main.h"
#include "d_dehacked.h"
#include "s_sound.h"
#include "gi.h"

EXTERN_CVAR (waddirs)

std::vector<std::string> wadfiles, wadhashes;		// [RH] remove limit on # of loaded wads
std::vector<std::string> patchfiles, patchhashes;	// [RH] remove limit on # of loaded wads
std::vector<std::string> missingfiles, missinghashes;

extern gameinfo_t SharewareGameInfo;
extern gameinfo_t RegisteredGameInfo;
extern gameinfo_t RetailGameInfo;
extern gameinfo_t CommercialGameInfo;
extern gameinfo_t RetailBFGGameInfo;
extern gameinfo_t CommercialBFGGameInfo;

bool lastWadRebootSuccess = true;

//
// BaseFileSearchDir
// denis - Check single paths for a given file with a possible extension
// Case insensitive, but returns actual file name
//
static std::string BaseFileSearchDir(std::string dir, std::string file, std::string ext, std::string hash = "")
{
	std::string found;

	if(dir[dir.length() - 1] != PATHSEPCHAR)
		dir += PATHSEP;

	std::transform(hash.begin(), hash.end(), hash.begin(), toupper);
	std::string dothash = ".";
	if(hash.length())
		dothash += hash;
	else
		dothash = "";

	// denis - list files in the directory of interest, case-desensitize
	// then see if wanted wad is listed
#ifdef UNIX
	struct dirent **namelist = 0;
	int n = scandir(dir.c_str(), &namelist, 0, alphasort);

	for(int i = 0; i < n && namelist[i]; i++)
	{
		std::string d_name = namelist[i]->d_name;

		M_Free(namelist[i]);

		if(!found.length())
		{
			if(d_name == "." || d_name == "..")
				continue;

			std::string tmp = d_name;
			std::transform(tmp.begin(), tmp.end(), tmp.begin(), toupper);

			if(file == tmp || (file + ext) == tmp || (file + dothash) == tmp || (file + ext + dothash) == tmp)
			{
				std::string local_file = (dir + d_name).c_str();
				std::string local_hash = W_MD5(local_file.c_str());

				if(!hash.length() || hash == local_hash)
				{
					found = d_name;
				}
				else if(hash.length())
				{
					Printf (PRINT_HIGH, "WAD at %s does not match required copy\n", local_file.c_str());
					Printf (PRINT_HIGH, "Local MD5: %s\n", local_hash.c_str());
					Printf (PRINT_HIGH, "Required MD5: %s\n\n", hash.c_str());
				}
			}
		}
	}

	M_Free(namelist);
#else
	std::string all_ext = dir + "*";
	//all_ext += ext;

	WIN32_FIND_DATA FindFileData;
	HANDLE hFind = FindFirstFile(all_ext.c_str(), &FindFileData);
	DWORD dwError = GetLastError();

	if (hFind == INVALID_HANDLE_VALUE)
	{
		Printf (PRINT_HIGH, "FindFirstFile failed for %s\n", all_ext.c_str());
		Printf (PRINT_HIGH, "GetLastError: %d\n", dwError);
		return "";
	}

	do
	{
		if(FindFileData.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)
			continue;

		std::string tmp = FindFileData.cFileName;
		std::transform(tmp.begin(), tmp.end(), tmp.begin(), toupper);

		if(file == tmp || (file + ext) == tmp || (file + dothash) == tmp || (file + ext + dothash) == tmp)
		{
			std::string local_file = (dir + FindFileData.cFileName).c_str();
			std::string local_hash = W_MD5(local_file.c_str());

			if(!hash.length() || hash == local_hash)
			{
				found = FindFileData.cFileName;
				break;
			}
			else if(hash.length())
			{
				Printf (PRINT_HIGH, "WAD at %s does not match required copy\n", local_file.c_str());
				Printf (PRINT_HIGH, "Local MD5: %s\n", local_hash.c_str());
				Printf (PRINT_HIGH, "Required MD5: %s\n\n", hash.c_str());
			}
		}
	} while(FindNextFile(hFind, &FindFileData));

	dwError = GetLastError();

	// Note: As documented, FindNextFile sets ERROR_NO_MORE_FILES as the error
	// code, but when this function "fails" it does not set it we have to assume
	// that it completed successfully (this is actually  bad practice, because
    // it says in the docs that it does not set ERROR_SUCCESS, even though
    // GetLastError returns 0) WTF DO WE DO?!
	if(dwError != ERROR_SUCCESS && dwError != ERROR_NO_MORE_FILES)
		Printf (PRINT_HIGH, "FindNextFile failed. GetLastError: %d\n", dwError);

	FindClose(hFind);
#endif

	return found;
}

//
// D_AddSearchDir
// denis - Split a new directory string using the separator and append results to the output
//
void D_AddSearchDir(std::vector<std::string> &dirs, const char *dir, const char separator)
{
	if(!dir)
		return;

	// search through dwd
	std::stringstream ss(dir);
	std::string segment;

	while(!ss.eof())
	{
		std::getline(ss, segment, separator);

		if(!segment.length())
			continue;

		FixPathSeparator(segment);
		I_ExpandHomeDir(segment);

		if(segment[segment.length() - 1] != PATHSEPCHAR)
			segment += PATHSEP;

		dirs.push_back(segment);
	}
}

//
// BaseFileSearch
// denis - Check all paths of interest for a given file with a possible extension
//
static std::string BaseFileSearch(std::string file, std::string ext = "", std::string hash = "")
{
	#ifdef WIN32
		// absolute path?
		if(file.find(':') != std::string::npos)
			return file;

		const char separator = ';';
	#else
		// absolute path?
		if(file[0] == PATHSEPCHAR || file[0] == '~')
			return file;

		const char separator = ':';
	#endif

    // [Russell] - Bit of a hack. (since BaseFileSearchDir should handle this)
    // return file if it contains a path already
	if (M_FileExists(file))
		return file;

	std::transform(file.begin(), file.end(), file.begin(), toupper);
	std::transform(ext.begin(), ext.end(), ext.begin(), toupper);
	std::vector<std::string> dirs;

	dirs.push_back(startdir);
	dirs.push_back(progdir);

	D_AddSearchDir(dirs, Args.CheckValue("-waddir"), separator);
	D_AddSearchDir(dirs, getenv("DOOMWADDIR"), separator);
	D_AddSearchDir(dirs, getenv("DOOMWADPATH"), separator);
	D_AddSearchDir(dirs, getenv("HOME"), separator);
	D_AddSearchDir(dirs, waddirs.cstring(), separator);

	dirs.erase(std::unique(dirs.begin(), dirs.end()), dirs.end());

	for(size_t i = 0; i < dirs.size(); i++)
	{
		std::string found = BaseFileSearchDir(dirs[i], file, ext);

		if(found.length())
		{
			std::string &dir = dirs[i];

			if(dir[dir.length() - 1] != PATHSEPCHAR)
				dir += PATHSEP;

			return dir + found;
		}
	}

	// Not found
	return "";
}

//
// CheckIWAD
//
// Tries to find an IWAD from a set of know IWAD names, and checks the first
// one found's contents to determine whether registered/commercial features
// should be executed (notably loading PWAD's).
//
static bool CheckIWAD (std::string suggestion, std::string &titlestring)
{
	static const char *doomwadnames[] =
	{
		"doom2f.wad",
		"doom2.wad",
		"plutonia.wad",
		"tnt.wad",
		"doomu.wad", // Hack from original Linux version. Not necessary, but I threw it in anyway.
		"doom.wad",
		"doom1.wad",
		"freedoom.wad",
		"freedm.wad",
		"chex.wad",		// [ML] 1/7/10: Hello Chex Quest!
		NULL
	};

	std::string iwad;
	std::string iwad_file;
	int i;

	if(suggestion.length())
	{
		std::string found = BaseFileSearch(suggestion, ".WAD");

		if(found.length())
			iwad = found;
		else
		{
			if(M_FileExists(suggestion.c_str()))
				iwad = suggestion;
		}
		/*	[ML] Removed 1/13/10: we can trust the user to provide an iwad
		if(iwad.length())
		{
			FILE *f;

			if ( (f = fopen (iwad.c_str(), "rb")) )
			{
				wadinfo_t header;
				fread (&header, sizeof(header), 1, f);
				header.identification = LELONG(header.identification);
				if (header.identification != IWAD_ID)
				{
					if(header.identification == PWAD_ID)
					{
						Printf(PRINT_HIGH, "Suggested file is a PWAD, not an IWAD: %s \n", iwad.c_str());
					}
					else
					{
						Printf(PRINT_HIGH, "Suggested file is not an IWAD: %s \n", iwad.c_str());
					}
					iwad = "";
				}
				fclose(f);
			}
		}
		*/
	}

	if(!iwad.length())
	{
		// Search for a pre-defined IWAD from the list above
		for (i = 0; doomwadnames[i]; i++)
		{
			std::string found = BaseFileSearch(doomwadnames[i]);

			if(found.length())
			{
				iwad = found;
				break;
			}
		}
	}

	// Now scan the contents of the IWAD to determine which one it is
	if (iwad.length())
	{
#define NUM_CHECKLUMPS 10
		static const char checklumps[NUM_CHECKLUMPS][8] = {
			"E1M1", "E2M1", "E4M1", "MAP01",
			{ 'A','N','I','M','D','E','F','S'},
			"FINAL2", "REDTNT2", "CAMO1",
			{ 'E','X','T','E','N','D','E','D'},
			{ 'D','M','E','N','U','P','I','C'}
		};
		int lumpsfound[NUM_CHECKLUMPS];
		wadinfo_t header;
		FILE *f;
		M_ExtractFileName(iwad,iwad_file);

		memset (lumpsfound, 0, sizeof(lumpsfound));
		if ( (f = fopen (iwad.c_str(), "rb")) )
		{
			fread (&header, sizeof(header), 1, f);
			header.identification = LELONG(header.identification);
			if (header.identification == IWAD_ID ||
				header.identification == PWAD_ID)
			{
				header.numlumps = LELONG(header.numlumps);
				if (0 == fseek (f, LELONG(header.infotableofs), SEEK_SET))
				{
					for (i = 0; i < header.numlumps; i++)
					{
						filelump_t lump;
						int j;

						if (0 == fread (&lump, sizeof(lump), 1, f))
							break;
						for (j = 0; j < NUM_CHECKLUMPS; j++)
							if (!strnicmp (lump.name, checklumps[j], 8))
								lumpsfound[j]++;
					}
				}
			}
			fclose (f);
		}

		gamemode = undetermined;

		if (lumpsfound[3])
		{
            if (lumpsfound[9])
            {
                gameinfo = CommercialBFGGameInfo;
                gamemode = commercial_bfg;
            }
            else
            {
                gameinfo = CommercialGameInfo;
                gamemode = commercial;
            }

			if (lumpsfound[6])
			{
				gamemission = pack_tnt;
				titlestring = "DOOM 2: TNT - Evilution";
			}
			else if (lumpsfound[7])
			{
				gamemission = pack_plut;
				titlestring = "DOOM 2: Plutonia Experiment";
			}
			else
			{
				gamemission = doom2;

				if (lumpsfound[9])
                    titlestring = "DOOM 2: Hell on Earth (BFG Edition)";
                else
                    titlestring = "DOOM 2: Hell on Earth";
			}
		}
		else if (lumpsfound[0])
		{
			gamemission = doom;
			if (lumpsfound[1])
			{
				if (lumpsfound[2])
				{
					if (!StdStringCompare(iwad_file,"chex.wad",true))	// [ML] 1/7/10: HACK - There's no unique lumps in the chex quest
					{													// iwad.  It's ultimate doom with their stuff replacing most things.
						gamemission = chex;
						gamemode = retail_chex;
						gameinfo = RetailGameInfo;
						titlestring = "Chex Quest";
					}
					else
					{
					    gamemode = retail;

					    if (lumpsfound[9])
                        {
                            gameinfo = RetailBFGGameInfo;
                            titlestring = "The Ultimate DOOM (BFG Edition)";
                        }
                        else
                        {
                            gameinfo = RetailGameInfo;
                            titlestring = "The Ultimate DOOM";
                        }
					}
				}
				else
				{
					gamemode = registered;
					gameinfo = RegisteredGameInfo;
					titlestring = "DOOM Registered";
				}
			}
			else
			{
				gamemode = shareware;
				gameinfo = SharewareGameInfo;
				titlestring = "DOOM Shareware";
			}
		}
	}

	if (gamemode == undetermined)
	{
		gameinfo = SharewareGameInfo;
	}

	if (iwad.length())
		wadfiles.push_back(iwad);
	else if (clientside)
		I_Error("Cannot find IWAD (try -waddir)");

	return iwad.length() ? true : false;
}

//
// IdentifyVersion
//
static std::string IdentifyVersion (std::string custwad)
{
	std::string titlestring = "Public DOOM - ";

	if (clientside)
	{
		Printf(PRINT_HIGH, "\n\35\36\36\36\36\36\36\36\36\36\36\36\36\36\36\36\36\36\36\36"
    	                   "\36\36\36\36\36\36\36\36\36\36\36\36\37\n");

		if (!CheckIWAD(custwad, titlestring))
			Printf_Bold("Game mode indeterminate, no standard wad found.\n\n");
		else
			Printf_Bold("%s\n\n", titlestring.c_str());
	}
	else
	{
		if (!CheckIWAD (custwad, titlestring))
			Printf (PRINT_HIGH, "Game mode indeterminate, no standard wad found.\n");

		Printf (PRINT_HIGH, "%s\n", titlestring.c_str());
	}

	return titlestring;
}

//
// D_AddDefWads
//
void D_AddDefWads (std::string iwad)
{
	// [RH] Make sure zdoom.wad is always loaded,
	// as it contains magic stuff we need.
	// [SL] Err... odamex.wad!
	std::string wad = BaseFileSearch ("odamex.wad");
	if (wad.length())
		wadfiles.push_back(wad);
	else
		I_FatalError ("Cannot find odamex.wad");

	I_SetTitleString (IdentifyVersion(iwad).c_str());
}

//
// D_DoDefDehackedPatch
//
// [Russell] - Change the meaning, this will load multiple patch files if
//             specified
void D_DoDefDehackedPatch (const std::vector<std::string> &newpatchfiles)
{
    DArgs files;
    BOOL noDef = false;
    BOOL chexLoaded = false;
    QWORD i;

    if (!newpatchfiles.empty())
    {
        std::string f;
        std::string ext;

        // we want the extension of the file
        for (i = 0; i < newpatchfiles.size(); i++)
        {
            if (M_ExtractFileExtension(newpatchfiles[i], ext))
            {
                f = BaseFileSearch(newpatchfiles[i], ext);

                if (f.length())
                {
                    if (DoDehPatch (f.c_str(), false))
                    {
                        std::string Filename;
                        M_ExtractFileName(f, Filename);
                        patchfiles.push_back(Filename);
                    }

                    noDef = true;
                }
            }
        }
    }
    else // [Russell] - Only load if newpatchfiles is empty
    {
        // try .deh files on command line

        files = Args.GatherFiles ("-deh", ".deh", false);

        if (files.NumArgs())
        {
            for (i = 0; i < files.NumArgs(); i++)
            {
                std::string f = BaseFileSearch (files.GetArg (i), ".DEH");

                if (f.length())
                {
                    if (DoDehPatch (f.c_str(), false))
                    {
                        std::string Filename;
                        M_ExtractFileName(f, Filename);
                        patchfiles.push_back(Filename);
                    }
  
					if (!strncmp(files.GetArg(i),"chex.deh", 8))
						chexLoaded = true;

                }
            }
            noDef = true;
        }

		if (gamemode == retail_chex && !multiplayer && !chexLoaded)
			Printf(PRINT_HIGH,"Warning: chex.deh not loaded, experience may differ from the original!\n");

        // remove the old arguments
        files.FlushArgs();

        // try .bex files on command line
        files = Args.GatherFiles ("-bex", ".bex", false);

        if (files.NumArgs())
        {
            for (i = 0; i < files.NumArgs(); i++)
            {
                std::string f = BaseFileSearch (files.GetArg (i), ".BEX");

                if (f.length())
                {
                    if (DoDehPatch (f.c_str(), false))
                    {
                        std::string Filename;
                        M_ExtractFileName(f, Filename);
                        patchfiles.push_back(Filename);
                    }
                }
            }
            noDef = true;
        }
    }

    // try default patches
    if (!noDef)
        DoDehPatch (NULL, true);	// See if there's a patch in a PWAD

	for (size_t i = 0; i < patchfiles.size(); i++)
		patchhashes.push_back(W_MD5(patchfiles[i]));
}

//
// D_CleanseFileName
//
// Strips a file name of path information and transforms it into uppercase
//
std::string D_CleanseFileName(const std::string &filename, const std::string &ext)
{
	std::string newname(filename);

	FixPathSeparator(newname);
	if (ext.length())
		M_AppendExtension(newname, "." + ext);

	size_t slash = newname.find_last_of(PATHSEPCHAR);

	if (slash != std::string::npos)
		newname = newname.substr(slash + 1, newname.length() - slash);

	std::transform(newname.begin(), newname.end(), newname.begin(), toupper);
	
	return newname;
}

void D_NewWadInit();

//
// D_DoomWadReboot
// [denis] change wads at runtime
// Returns false if there are missing files and fills the missingfiles
// vector
//
// [SL] passing an IWAD as newwadfiles[0] is now optional
// TODO: hash checking for patchfiles
//
bool D_DoomWadReboot(
	const std::vector<std::string> &newwadfiles,
	const std::vector<std::string> &newpatchfiles,
	const std::vector<std::string> &newwadhashes,
	const std::vector<std::string> &newpatchhashes
)
{
	size_t i;

	bool hashcheck = (newwadfiles.size() == newwadhashes.size());

	missingfiles.clear();
	missinghashes.clear();

	// already loaded these?
	if (lastWadRebootSuccess &&	!wadhashes.empty() &&
		newwadhashes == std::vector<std::string>(wadhashes.begin()+1, wadhashes.end()))
	{
		// fast track if files have not been changed // denis - todo - actually check the file timestamps
		Printf (PRINT_HIGH, "Currently loaded WADs match server checksum\n\n");
		return true;
	}

	lastWadRebootSuccess = false;

	if (gamestate == GS_LEVEL)
		G_ExitLevel(0, 0);

	S_Stop();

	DThinker::DestroyAllThinkers();

	// Close all open WAD files
	W_Close();

	// [ML] 9/11/10: Reset custom wad level information from MAPINFO et al.
    // I have never used memset, I hope I am not invoking satan by doing this :(
	if (wadlevelinfos)
    {
		for (i = 0; i < numwadlevelinfos; i++)
			if (wadlevelinfos[i].snapshot)
			{
				delete wadlevelinfos[i].snapshot;
				wadlevelinfos[i].snapshot = NULL;
			}
        memset(wadlevelinfos,0,sizeof(wadlevelinfos));
        numwadlevelinfos = 0;
    }

    if (wadclusterinfos)
    {
        memset(wadclusterinfos,0,sizeof(wadclusterinfos));
        numwadclusterinfos = 0;
    }

	// Restart the memory manager
	Z_Init();

	SetLanguageIDs ();

	gamestate_t oldgamestate = gamestate;
	gamestate = GS_STARTUP; // prevent console from trying to use nonexistant font

	// [SL] 2012-12-06 - If we weren't provided with a new IWAD filename in
	// newwadfiles, use the previous IWAD.
	std::string iwad_filename;
	if ((newwadfiles.empty() || !W_IsIWAD(newwadfiles[0])) && (wadfiles.size() >= 2))
		iwad_filename = wadfiles[1];
	else if (!newwadfiles.empty())
		iwad_filename = newwadfiles[0];

	wadfiles.clear();
	D_AddDefWads(iwad_filename);	// add odamex.wad & IWAD

	for (i = 0; i < newwadfiles.size(); i++)
	{
		// already added the IWAD with D_AddDefWads
		if (W_IsIWAD(newwadfiles[i]))
			continue;

		std::string base_filename = D_CleanseFileName(newwadfiles[i], "wad");
		std::string full_filename;

		if (hashcheck)
			full_filename = BaseFileSearch(base_filename, ".WAD", newwadhashes[i]);
		else
			full_filename = BaseFileSearch(base_filename, ".WAD");

		if (base_filename.length())
		{
			if (full_filename.length())
				wadfiles.push_back(full_filename);
			else
			{
				Printf(PRINT_HIGH, "could not find WAD: %s\n", base_filename.c_str());
				missingfiles.push_back(base_filename);
				if (hashcheck)
					missinghashes.push_back(newwadhashes[i]);
			}
		}
	}

	modifiedgame = (wadfiles.size() > 2) || !patchfiles.empty();	// more than odamex.wad and IWAD?
	if (modifiedgame && (gameinfo.flags & GI_SHAREWARE))
		I_Error("\nYou cannot load additional WADs with the shareware version. Register!");

	wadhashes = W_InitMultipleFiles (wadfiles);

	// get skill / episode / map from parms
	strcpy(startmap, (gameinfo.flags & GI_MAPxx) ? "MAP01" : "E1M1");

    UndoDehPatch();
    patchfiles.clear();

	// [RH] Initialize localizable strings.
	GStrings.ResetStrings ();
	GStrings.Compact ();

	D_DoDefDehackedPatch(newpatchfiles);

	D_NewWadInit();

	// preserve state
	lastWadRebootSuccess = missingfiles.empty();

	gamestate = oldgamestate; // GS_STARTUP would prevent netcode connecting properly

	return missingfiles.empty();
}

//
// D_AddCmdParameterFiles
// Add the files specified with -file, do this only when it first loads
//
void D_AddCmdParameterFiles(void)
{
    modifiedgame = false;

	DArgs files = Args.GatherFiles ("-file", ".wad", true);
	if (files.NumArgs() > 0)
	{
		// the files gathered are wadfile/lump names
		modifiedgame = true;			// homebrew levels
		for (size_t i = 0; i < files.NumArgs(); i++)
		{
			std::string file = BaseFileSearch (files.GetArg (i), ".WAD");
			if (file.length())
				wadfiles.push_back(file);
		}
	}
}


VERSION_CONTROL (d_main_cpp, "$Id: d_main.cpp 3426 2012-11-19 17:25:28Z dr_sean $")
