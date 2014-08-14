// Emacs style mode select   -*- C++ -*-
//-----------------------------------------------------------------------------
//
// $Id: d_main.cpp 3426 2012-11-19 17:25:28Z dr_sean $
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
//	Common functions to determine game mode (shareware, registered),
//	parse command line parameters, and handle wad changes.
//
//-----------------------------------------------------------------------------

#include "version.h"

#include <sstream>
#include <string>
#include <vector>
#include <algorithm>

#include "win32inc.h"
#ifndef _WIN32
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
#include "r_main.h"
#include "d_main.h"
#include "d_dehacked.h"
#include "s_sound.h"
#include "gi.h"

#ifdef GEKKO
#include "i_wii.h"
#endif

#ifdef _XBOX
#include "i_xbox.h"
#endif

#include "res_main.h"

EXTERN_CVAR (waddirs)

extern gameinfo_t SharewareGameInfo;
extern gameinfo_t RegisteredGameInfo;
extern gameinfo_t RetailGameInfo;
extern gameinfo_t CommercialGameInfo;
extern gameinfo_t RetailBFGGameInfo;
extern gameinfo_t CommercialBFGGameInfo;

const char *ParseString2(const char *data);

bool lastWadRebootSuccess = true;
extern bool step_mode;

bool capfps = true;
float maxfps = 35.0f;

#if defined(_WIN32) && !defined(_XBOX)

#define arrlen(array) (sizeof(array) / sizeof(*array))

typedef struct
{
	HKEY root;
	const char* path;
	const char* value;
} registry_value_t;

static const char* uninstaller_string = "\\uninstl.exe /S ";

// Keys installed by the various CD editions.  These are actually the
// commands to invoke the uninstaller and look like this:
//
// C:\Program Files\Path\uninstl.exe /S C:\Program Files\Path
//
// With some munging we can find where Doom was installed.

// [AM] From the persepctive of a 64-bit executable, 32-bit registry keys are
//      located in a different spot.
#if _WIN64
#define SOFTWARE_KEY "Software\\Wow6432Node"
#else
#define SOFTWARE_KEY "Software"
#endif

static registry_value_t uninstall_values[] =
{
	// Ultimate Doom, CD version (Depths of Doom trilogy)
	{
		HKEY_LOCAL_MACHINE,
		SOFTWARE_KEY "\\Microsoft\\Windows\\CurrentVersion\\"
			"Uninstall\\Ultimate Doom for Windows 95",
		"UninstallString",
	},

	// Doom II, CD version (Depths of Doom trilogy)
	{
		HKEY_LOCAL_MACHINE,
		SOFTWARE_KEY "\\Microsoft\\Windows\\CurrentVersion\\"
			"Uninstall\\Doom II for Windows 95",
		"UninstallString",
	},

	// Final Doom
	{
		HKEY_LOCAL_MACHINE,
		SOFTWARE_KEY "\\Microsoft\\Windows\\CurrentVersion\\"
			"Uninstall\\Final Doom for Windows 95",
		"UninstallString",
	},

	// Shareware version
	{
		HKEY_LOCAL_MACHINE,
		SOFTWARE_KEY "\\Microsoft\\Windows\\CurrentVersion\\"
			"Uninstall\\Doom Shareware for Windows 95",
		"UninstallString",
	},
};

// Value installed by the Collector's Edition when it is installed
static registry_value_t collectors_edition_value =
{
	HKEY_LOCAL_MACHINE,
	SOFTWARE_KEY "\\Activision\\DOOM Collector's Edition\\v1.0",
	"INSTALLPATH",
};

// Subdirectories of the above install path, where IWADs are installed.
static const char* collectors_edition_subdirs[] =
{
	"Doom2",
	"Final Doom",
	"Ultimate Doom",
};

// Location where Steam is installed
static registry_value_t steam_install_location =
{
	HKEY_LOCAL_MACHINE,
	SOFTWARE_KEY "\\Valve\\Steam",
	"InstallPath",
};

// Subdirs of the steam install directory where IWADs are found
static const char* steam_install_subdirs[] =
{
	"steamapps\\common\\doom 2\\base",
	"steamapps\\common\\final doom\\base",
	"steamapps\\common\\ultimate doom\\base",
	"steamapps\\common\\DOOM 3 BFG Edition\\base\\wads",
	"steamapps\\common\\master levels of doom\\master\\wads", //Let Odamex find the Master Levels pwads too
};


static char *GetRegistryString(registry_value_t *reg_val)
{
	HKEY key;
	DWORD len;
	DWORD valtype;
	char* result;

	// Open the key (directory where the value is stored)

	if (RegOpenKeyEx(reg_val->root, reg_val->path,
	    0, KEY_READ, &key) != ERROR_SUCCESS)
	{
		return NULL;
	}

	result = NULL;

	// Find the type and length of the string, and only accept strings.

	if (RegQueryValueEx(key, reg_val->value,
	    NULL, &valtype, NULL, &len) == ERROR_SUCCESS && valtype == REG_SZ)
	{
		// Allocate a buffer for the value and read the value
		result = static_cast<char*>(malloc(len));

		if (RegQueryValueEx(key, reg_val->value, NULL, &valtype,
		    (unsigned char *)result, &len) != ERROR_SUCCESS)
		{
			free(result);
			result = NULL;
		}
	}

	// Close the key

	RegCloseKey(key);

	return result;
}

#endif

//
// BaseFileSearchDir
// denis - Check single paths for a given file with a possible extension
// Case insensitive, but returns actual file name
//
static std::string BaseFileSearchDir(std::string dir, std::string file, std::string ext, std::string hash = "")
{
	std::string found;

	if (dir[dir.length() - 1] != PATHSEPCHAR)
		dir += PATHSEP;

	hash = StdStringToUpper(hash);
	std::string dothash = ".";
	if (hash.length())
		dothash += hash;
	else
		dothash = "";

	// denis - list files in the directory of interest, case-desensitize
	// then see if wanted wad is listed
#ifdef UNIX
	struct dirent **namelist = 0;
	int n = scandir(dir.c_str(), &namelist, 0, alphasort);

	for (int i = 0; i < n && namelist[i]; i++)
	{
		std::string d_name = namelist[i]->d_name;

		M_Free(namelist[i]);

		if (found.empty())
		{
			if (d_name == "." || d_name == "..")
				continue;

			std::string tmp = StdStringToUpper(d_name);

			if (file == tmp || (file + ext) == tmp || (file + dothash) == tmp || (file + ext + dothash) == tmp)
			{
				std::string local_file(dir + d_name);
				std::string local_hash(W_MD5(local_file));

				if (hash.empty() || hash == local_hash)
				{
					found = d_name;
				}
				else if (!hash.empty())
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

	if (hFind == INVALID_HANDLE_VALUE)
		return "";

	do
	{
		if (FindFileData.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)
			continue;

		std::string tmp = StdStringToUpper(FindFileData.cFileName);

		if (file == tmp || (file + ext) == tmp || (file + dothash) == tmp || (file + ext + dothash) == tmp)
		{
			std::string local_file(dir + FindFileData.cFileName);
			std::string local_hash(W_MD5(local_file));

			if (hash.empty() || hash == local_hash)
			{
				found = FindFileData.cFileName;
				break;
			}
			else if (!hash.empty())
			{
				Printf (PRINT_HIGH, "WAD at %s does not match required copy\n", local_file.c_str());
				Printf (PRINT_HIGH, "Local MD5: %s\n", local_hash.c_str());
				Printf (PRINT_HIGH, "Required MD5: %s\n\n", hash.c_str());
			}
		}
	} while (FindNextFile(hFind, &FindFileData));

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

// [AM] Add platform-sepcific search directories
static void D_AddPlatformSearchDirs(std::vector<std::string> &dirs)
{
#if defined(_WIN32) && !defined(_XBOX)

	// Doom 95
	{
		unsigned int i;

		for (i = 0;i < arrlen(uninstall_values);++i)
		{
			char* val;
			char* path;
			char* unstr;

			val = GetRegistryString(&uninstall_values[i]);

			if (val == NULL)
				continue;

			unstr = strstr(val, uninstaller_string);

			if (unstr == NULL)
			{
				free(val);
			}
			else
			{
				path = unstr + strlen(uninstaller_string);

				const char* cpath = path;
				D_AddSearchDir(dirs, cpath, SEARCHPATHSEPCHAR);
			}
		}
	}

	// Doom Collectors Edition
	{
		char* install_path;
		char* subpath;
		unsigned int i;

		install_path = GetRegistryString(&collectors_edition_value);

		if (install_path != NULL)
		{
			for (i = 0;i < arrlen(collectors_edition_subdirs);++i)
			{
				subpath = static_cast<char*>(malloc(strlen(install_path)
				                             + strlen(collectors_edition_subdirs[i])
				                             + 5));
				sprintf(subpath, "%s\\%s", install_path, collectors_edition_subdirs[i]);

				const char* csubpath = subpath;
				D_AddSearchDir(dirs, csubpath, SEARCHPATHSEPCHAR);
			}

			free(install_path);
		}
	}

	// Doom on Steam
	{
		char* install_path;
		char* subpath;
		size_t i;

		install_path = GetRegistryString(&steam_install_location);

		if (install_path != NULL)
		{
			for (i = 0;i < arrlen(steam_install_subdirs);++i)
			{
				subpath = static_cast<char*>(malloc(strlen(install_path)
				                             + strlen(steam_install_subdirs[i]) + 5));
				sprintf(subpath, "%s\\%s", install_path, steam_install_subdirs[i]);

				const char* csubpath = subpath;
				D_AddSearchDir(dirs, csubpath, SEARCHPATHSEPCHAR);
			}

			free(install_path);
		}
	}

	// DOS Doom via DEICE
	D_AddSearchDir(dirs, "\\doom2", SEARCHPATHSEPCHAR);    // Doom II
	D_AddSearchDir(dirs, "\\plutonia", SEARCHPATHSEPCHAR); // Final Doom
	D_AddSearchDir(dirs, "\\tnt", SEARCHPATHSEPCHAR);
	D_AddSearchDir(dirs, "\\doom_se", SEARCHPATHSEPCHAR);  // Ultimate Doom
	D_AddSearchDir(dirs, "\\doom", SEARCHPATHSEPCHAR);     // Shareware / Registered Doom
	D_AddSearchDir(dirs, "\\dooms", SEARCHPATHSEPCHAR);    // Shareware versions
	D_AddSearchDir(dirs, "\\doomsw", SEARCHPATHSEPCHAR);

#elif defined(UNIX)

	#if defined(INSTALL_PREFIX) && defined(INSTALL_DATADIR) 
	D_AddSearchDir(dirs, INSTALL_PREFIX "/" INSTALL_DATADIR "/odamex", SEARCHPATHSEPCHAR);
	D_AddSearchDir(dirs, INSTALL_PREFIX "/" INSTALL_DATADIR "/games/odamex", SEARCHPATHSEPCHAR);
	#endif

	D_AddSearchDir(dirs, "/usr/share/games/doom", SEARCHPATHSEPCHAR);
	D_AddSearchDir(dirs, "/usr/local/share/games/doom", SEARCHPATHSEPCHAR);
	D_AddSearchDir(dirs, "/usr/local/share/doom", SEARCHPATHSEPCHAR);

#endif
}

//
// BaseFileSearch
// denis - Check all paths of interest for a given file with a possible extension
//
static std::string BaseFileSearch(std::string file, std::string ext = "", std::string hash = "")
{
	#ifdef _WIN32
		// absolute path?
		if (file.find(':') != std::string::npos)
			return file;
	#else
		// absolute path?
		if (file[0] == PATHSEPCHAR || file[0] == '~')
			return file;
	#endif

    // [Russell] - Bit of a hack. (since BaseFileSearchDir should handle this)
    // return file if it contains a path already
	if (M_FileExists(file))
		return file;

	file = StdStringToUpper(file);
	ext = StdStringToUpper(ext);
	std::vector<std::string> dirs;

	dirs.push_back(startdir);
	dirs.push_back(progdir);

	D_AddSearchDir(dirs, Args.CheckValue("-waddir"), SEARCHPATHSEPCHAR);
	D_AddSearchDir(dirs, getenv("DOOMWADDIR"), SEARCHPATHSEPCHAR);
	D_AddSearchDir(dirs, getenv("DOOMWADPATH"), SEARCHPATHSEPCHAR);
	D_AddSearchDir(dirs, getenv("HOME"), SEARCHPATHSEPCHAR);

	// [AM] Search additional paths based on platform
	D_AddPlatformSearchDirs(dirs);

	D_AddSearchDir(dirs, waddirs.cstring(), SEARCHPATHSEPCHAR);

	dirs.erase(std::unique(dirs.begin(), dirs.end()), dirs.end());

	for (size_t i = 0; i < dirs.size(); i++)
	{
		std::string found = BaseFileSearchDir(dirs[i], file, ext, hash);

		if (!found.empty())
		{
			std::string &dir = dirs[i];

			if (dir[dir.length() - 1] != PATHSEPCHAR)
				dir += PATHSEP;

			return dir + found;
		}
	}

	// Not found
	return "";
}


//
// D_ConfigureGameInfo
//
// Opens the specified file name and sets gameinfo, gamemission, and gamemode
// to the appropriate values for the IWAD file.
//
// gamemode will be set to undetermined if the file is not a valid IWAD.
//
static void D_ConfigureGameInfo(const std::string& iwad_filename)
{
	gamemode = undetermined;

	static const int NUM_CHECKLUMPS = 11;
	static const char checklumps[NUM_CHECKLUMPS][8] = {
		{ 'E','1','M','1' },					// 0
		{ 'E','2','M','1' },					// 1
		{ 'E','4','M','1' },					// 2
		{ 'M','A','P','0','1' },				// 3
		{ 'A','N','I','M','D','E','F','S' },	// 4
		{ 'F','I','N','A','L','2' },			// 5
		{ 'R','E','D','T','N','T','2' },		// 6
		{ 'C','A','M','O','1' },				// 7
		{ 'E','X','T','E','N','D','E','D' },	// 8
		{ 'D','M','E','N','U','P','I','C' },	// 9
		{ 'F','R','E','E','D','O','O','M' }		// 10
	};

	int lumpsfound[NUM_CHECKLUMPS] = { 0 };

	FILE* fp = fopen(iwad_filename.c_str(), "rb");
	if (fp)
	{
		wadinfo_t header;
		fread(&header, sizeof(header), 1, fp);

		// [SL] Allow both IWAD & PWAD identifiers since chex.wad is a PWAD
		header.identification = LELONG(header.identification);
		if (header.identification == IWAD_ID ||
			header.identification == PWAD_ID)
		{
			header.numlumps = LELONG(header.numlumps);
			if (0 == fseek(fp, LELONG(header.infotableofs), SEEK_SET))
			{
				for (int i = 0; i < header.numlumps; i++)
				{
					filelump_t lump;

					if (0 == fread(&lump, sizeof(lump), 1, fp))
						break;
					for (int j = 0; j < NUM_CHECKLUMPS; j++)
						if (!strnicmp(lump.name, checklumps[j], 8))
							lumpsfound[j]++;
				}
			}
		}
		fclose(fp);
		fp = NULL;
	}

	// [SL] Check for FreeDoom / Ultimate FreeDoom
	if (lumpsfound[10])
	{
		if (lumpsfound[0])
		{
			gamemode = retail;
			gameinfo = RetailGameInfo;
			gamemission = retail_freedoom;
		}
		else
		{
			gamemode = commercial;
			gameinfo = CommercialGameInfo;
			gamemission = commercial_freedoom;
		}
		return;
	}

	// Check for Doom 2 or TNT / Plutonia
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
			gamemission = pack_tnt;
		else if (lumpsfound[7])
			gamemission = pack_plut;
		else
			gamemission = doom2;

		return;
	}

	// Check for Registered Doom / Ultimate Doom / Chex Quest / Shareware Doom
	if (lumpsfound[0])
	{
		gamemission = doom;
		if (lumpsfound[1])
		{
			if (lumpsfound[2])
			{
				// [ML] 1/7/10: HACK - There's no unique lumps in the chex quest
				// iwad.  It's ultimate doom with their stuff replacing most things.
				std::string base_filename;
				M_ExtractFileName(iwad_filename, base_filename);
				if (iequals(base_filename, "chex.wad"))
				{
					gamemission = chex;
					gamemode = retail_chex;
					gameinfo = RetailGameInfo;
				}
				else
				{
					if (lumpsfound[9])
					{
						gamemode = retail_bfg;
						gameinfo = RetailBFGGameInfo;
					}
					else
					{
						gamemode = retail;
						gameinfo = RetailGameInfo;
					}
				}
			}
			else
			{
				gamemode = registered;
				gameinfo = RegisteredGameInfo;
			}
		}
		else
		{
			gamemode = shareware;
			gameinfo = SharewareGameInfo;
		}

		return;
	}

	if (gamemode == undetermined)
		gameinfo = SharewareGameInfo;
}


//
// D_GetTitleString
//
// Returns the proper name of the game currently loaded into gameinfo & gamemission
//
std::string D_GetTitleString()
{
	if (gamemission == pack_tnt)
		return "DOOM 2: TNT - Evilution";
	if (gamemission == pack_plut)
		return "DOOM 2: Plutonia Experiment";
	if (gamemission == chex)
		return "Chex Quest";
	if (gamemission == retail_freedoom)
		return "Ultimate FreeDoom";
	if (gamemission == commercial_freedoom)
		return "FreeDoom";

	return gameinfo.titleString;
}


//
// D_CheckIWAD
//
// Tries to find an IWAD from a set of know IWAD names, and checks the first
// one found's contents to determine whether registered/commercial features
// should be executed (notably loading PWAD's).
//
static std::string D_CheckIWAD(const std::string& suggestion)
{
	std::string iwad;
	std::string iwad_file;

	if (!suggestion.empty())
	{
		std::string found = BaseFileSearch(suggestion, ".WAD");

		if (!found.empty())
			iwad = found;
		else
		{
			if (M_FileExists(suggestion))
				iwad = suggestion;
		}
	}

	if (iwad.empty())
	{
		// Search for a pre-defined IWAD from the list above
		for (int i = 0; !doomwadnames[i].name.empty(); i++)
		{
			std::string found = BaseFileSearch(doomwadnames[i].name);

			if (!found.empty())
			{
				iwad = found;
				break;
			}
		}
	}

	return iwad;
}


//
// D_PrintIWADIdentity
//
static void D_PrintIWADIdentity()
{
	if (clientside)
	{
		Printf(PRINT_HIGH, "\n\35\36\36\36\36\36\36\36\36\36\36\36\36\36\36\36\36\36\36\36"
    	                   "\36\36\36\36\36\36\36\36\36\36\36\36\37\n");

		if (gamemode == undetermined)
			Printf_Bold("Game mode indeterminate, no standard wad found.\n\n");
		else
			Printf_Bold("%s\n\n", D_GetTitleString().c_str());
	}
	else
	{
		if (gamemode == undetermined)
			Printf(PRINT_HIGH, "Game mode indeterminate, no standard wad found.\n");
		else 
			Printf(PRINT_HIGH, "%s\n", D_GetTitleString().c_str()); 
	}
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
		newname = M_AppendExtension(newname, "." + ext);

	size_t slash = newname.find_last_of(PATHSEPCHAR);

	if (slash != std::string::npos)
		newname = newname.substr(slash + 1, newname.length() - slash);

	std::transform(newname.begin(), newname.end(), newname.begin(), toupper);

	return newname;
}


//
// D_VerifyFile
//
// Searches for a given file name, setting base_filename and full_filename
// to the full path of the file. Returns true if the file was found and it
// matched the supplied hash (or has was empty).
//
bool D_VerifyFile(
		const std::string& filename,
		std::string& base_filename,
		std::string& full_filename,
		const std::string& hash)
{
	base_filename.clear();
	full_filename.clear();

	std::string ext;
	M_ExtractFileExtension(filename, ext);

	base_filename = D_CleanseFileName(filename);
	if (base_filename.empty())
		return false;

	// was a path to the file supplied?
	std::string dir;
	M_ExtractFilePath(filename, dir);
	if (!dir.empty())
	{
		std::string found = BaseFileSearchDir(dir, base_filename, ext, hash);
		if (!found.empty())
		{
			if (dir[dir.length() - 1] != PATHSEPCHAR)
				dir += PATHSEP;
			full_filename = dir + found;
			return true;
		}
	}

	// is there an exact match for the filename and hash in WADDIRS?
	full_filename = BaseFileSearch(base_filename, "." + ext, hash);
	if (!full_filename.empty())
		return true;

	// is there a file with matching name even if the hash is incorrect in WADDIRS?
	full_filename = BaseFileSearch(base_filename, "." + ext);
	if (full_filename.empty())
		return false;

	// if it's an IWAD, check if we have a valid alternative hash
	std::string found_hash = W_MD5(full_filename);
	if (W_IsIWAD(base_filename, found_hash))
		return true;

	return false;
}


//
// D_VerifyResourceFiles
//
void D_VerifyResourceFiles(
		const std::vector<std::string>& resource_file_names,
		const std::vector<std::string>& resource_file_hashes,
		std::vector<std::string>& missing_file_names)
{
	missing_file_names.clear();

	for (size_t i = 0; i < resource_file_names.size(); i++)
	{
		std::string base_filename, full_filename;
		const std::string& file_hash = (resource_file_hashes.size() > i) ?  resource_file_hashes[i] : "";

		if (!D_VerifyFile(resource_file_names[i], base_filename, full_filename, file_hash))
			missing_file_names.push_back(base_filename);
	}
}


//
// D_LoadResourceFiles
//
// Loads the given set of resource file names. If the file names do not
// include full paths, the default search paths will be used to find the
// files.
// It is expected that resource_file_names[0] be ODAMEX.WAD and
// resource_file_names[1] be an IWAD.
//
void D_LoadResourceFiles(const std::vector<std::string>& resource_file_names)
{
	// If the given files are already loaded, bail out early.
	if (resource_file_names == Res_GetResourceFileNames())
		return;

	gamestate_t oldgamestate = gamestate;
	gamestate = GS_STARTUP;		// prevent console from trying to use nonexistant font

	size_t resource_file_count = resource_file_names.size();

	// Require ODAMEX.WAD and an IWAD
	if (resource_file_count < 2)
		I_Error("Invalid resource file list: expected ODAMEX.WAD and an IWAD file.\n");

	const std::string odamex_wad_filename = M_ExtractFileName(resource_file_names[0]);
	if (!iequals(odamex_wad_filename, "ODAMEX.WAD"))
		I_Error("Invalid resource file list: expected ODAMEX.WAD instead of %s\n", odamex_wad_filename.c_str());

	std::string iwad_filename = resource_file_names[1], base_filename, full_filename;
	if (D_VerifyFile(iwad_filename, base_filename, full_filename))
		iwad_filename = full_filename;
	else if (D_VerifyFile(M_AppendExtension(iwad_filename, ".WAD"), base_filename, full_filename))
		iwad_filename = full_filename;
	else
		I_Error("Invalid resource file list: expected an IWAD file.\n");

	if (!W_IsIWAD(iwad_filename))
		I_Error("Invalid resource file list: expected an IWAD file instead of %s\n", iwad_filename.c_str());

	// Now scan the contents of the IWAD to determine which one it is.
	D_ConfigureGameInfo(iwad_filename);

	// Print info about the IWAD to the console.
	D_PrintIWADIdentity();

	// Set the window title based on which IWAD we're using.
	I_SetTitleString(D_GetTitleString().c_str());

	// Don't load PWADS with the shareware IWAD.
	if (gameinfo.flags & GI_SHAREWARE && resource_file_count > 2)
	{
		Printf(PRINT_HIGH, "You cannot load additional resource files with the shareware version. Register!\n");
		resource_file_count = 2;
	}

	// Load the resource files
	for (size_t i = 0; i < resource_file_count; i++)
	{
		std::string base_filename, full_filename;
		if (D_VerifyFile(resource_file_names[i], base_filename, full_filename))
			Res_OpenResourceFile(full_filename);
	}


	// TODO: delete this section once we're fully migrated to the ResourceFile system
	{
		std::vector<std::string> temp_resource_file_names;
		for (size_t i = 0; i < resource_file_count; i++)
			if (D_VerifyFile(resource_file_names[i], base_filename, full_filename))
				temp_resource_file_names.push_back(full_filename);

		W_InitMultipleFiles(temp_resource_file_names);
	}



	// [RH] Initialize localizable strings.
	// [SL] It is necessary to load the strings here since a dehacked patch
	// might change the strings
	ResourceId language_res_id = Res_GetResourceId("LANGUAGE");
	size_t language_length = Res_GetLumpLength(language_res_id);
	byte* language_data = new byte[language_length];
	Res_ReadLump(language_res_id, language_data);
	GStrings.LoadStrings(language_data, language_length, STRING_TABLE_SIZE, false);
	GStrings.Compact();
	delete [] language_data;

	// Load all DeHackEd files
	std::vector<ResourceId> dehacked_res_ids;
	Res_QueryLumpName(dehacked_res_ids, "DEHACKED");
	for (size_t i = 0; i < dehacked_res_ids.size(); i++)
		D_LoadDehLump(dehacked_res_ids[i]);

	// check for ChexQuest
	if (gamemode == retail_chex)
	{
		bool chex_deh_loaded = false;
		for (size_t i = 0; i < resource_file_count; i++)
			if (iequals(M_ExtractFileName(resource_file_names[i]), "CHEX.DEH"))
				chex_deh_loaded = true;
	
		if (!chex_deh_loaded)
			Printf(PRINT_HIGH, "Warning: CHEX.DEH not loaded, experience may differ from the original!\n");
	}

	// get skill / episode / map from parms
	strcpy(startmap, (gameinfo.flags & GI_MAPxx) ? "MAP01" : "E1M1");

	// preserve state
	gamestate = oldgamestate;
}

					

//
// D_AddResourceFileFromArgs
//
// Adds the full path of all the file names given on the command line
// following the "-iwad", "-file", or "-deh" parameters.
//
void D_AddResourceFilesFromArgs(std::vector<std::string>& resource_file_names)
{
	std::string base_filename, full_filename;

	resource_file_names.clear();

	if (D_VerifyFile("ODAMEX.WAD", base_filename, full_filename))
		resource_file_names.push_back(full_filename);
	else
		return;

	std::string iwad_filename;

	size_t i = Args.CheckParm("-iwad");
	if (i > 0 && i < Args.NumArgs() - 1)
	{
		// Get IWAD file name
		std::string filename(Args.GetArg(i + 1));
		if (D_VerifyFile(filename, base_filename, full_filename))
			iwad_filename = full_filename;
		else if (D_VerifyFile(M_AppendExtension(filename, ".WAD"), base_filename, full_filename))
			iwad_filename = full_filename;
	}

	iwad_filename = D_CheckIWAD(iwad_filename);
	if (!iwad_filename.empty())
		resource_file_names.push_back(iwad_filename);
	else
		return;

	size_t arg_count = Args.NumArgs();

	// [SL] the first parameter should be treated as a file name and
	// doesn't need to be preceeded by -file
	bool is_filename = true;

	for (size_t i = 1; i < arg_count; i++)
	{
		const char* arg_value = Args.GetArg(i);

		if (arg_value[0] == '-' || arg_value[0] == '+')
		{
			is_filename = (stricmp(arg_value, "-file") == 0 || stricmp(arg_value, "-deh") == 0);
		}
		else if (is_filename)
		{
			std::string filename(arg_value);

			if (D_VerifyFile(filename, base_filename, full_filename))
				resource_file_names.push_back(full_filename);
			else if (D_VerifyFile(M_AppendExtension(filename, ".WAD"), base_filename, full_filename))
				resource_file_names.push_back(full_filename);
			else if (D_VerifyFile(M_AppendExtension(filename, ".DEH"), base_filename, full_filename))
				resource_file_names.push_back(full_filename);
			else if (D_VerifyFile(M_AppendExtension(filename, ".BEX"), base_filename, full_filename))
				resource_file_names.push_back(full_filename);
		}
	}
}


//
// D_AddResourceFilesFromString
//
// Parses a string of resource file names (separated by spaces) and
// validates them.
//
// Note: if a file name does not include an extension, this will attempt to
// find the file using .WAD, .DEH, or .BEX.
//
void D_AddResourceFilesFromString(std::vector<std::string>& resource_file_names, const std::string &str)
{
	resource_file_names.clear();

	const char* data = str.c_str();

	// this shouldn't ever happen
	if (Res_GetResourceFileNames().size() < 2)
		return;

	for (size_t argv = 0; (data = ParseString2(data)); argv++)
	{
		std::string filename(com_token), base_filename, full_filename;

		// ODAMEX.WAD wasn't specified so add it
		if (resource_file_names.size() == 0 && 
			!iequals(M_ExtractFileName(M_AppendExtension(filename, ".WAD")), "ODAMEX.WAD"))
			resource_file_names.push_back(Res_GetResourceFileNames()[0]);

		// No IWAD specified so use the currently loaded one.
		if (resource_file_names.size() == 1 && !W_IsIWAD(M_AppendExtension(filename, ".WAD")))
			resource_file_names.push_back(Res_GetResourceFileNames()[1]);

		if (D_VerifyFile(filename, base_filename, full_filename))
			resource_file_names.push_back(full_filename);
		else if (D_VerifyFile(M_AppendExtension(filename, ".WAD"), base_filename, full_filename))
			resource_file_names.push_back(full_filename);
		else if (D_VerifyFile(M_AppendExtension(filename, ".DEH"), base_filename, full_filename))
			resource_file_names.push_back(full_filename);
		else if (D_VerifyFile(M_AppendExtension(filename, ".BEX"), base_filename, full_filename))
			resource_file_names.push_back(full_filename);
	}
}


//
// D_ReloadResourceFiles
//
// Loads a new set of resource files if they are not currently loaded.
//
void D_ReloadResourceFiles(const std::vector<std::string>& new_resource_file_names)
{
	const std::vector<std::string>& resource_file_names = Res_GetResourceFileNames();
	bool reload = false;

	if (new_resource_file_names.size() != resource_file_names.size())
	{
		reload = true;
	}
	else
	{
		for (size_t i = 0; i < new_resource_file_names.size(); i++)
		{
			if (!iequals(D_CleanseFileName(new_resource_file_names[i]), D_CleanseFileName(resource_file_names[i])))
			{
				reload = true;
				break;
			}
		}
	}

	if (reload)
	{
		D_Shutdown();
		D_LoadResourceFiles(new_resource_file_names);
		D_Init();
	}
}


// ============================================================================
//
// TaskScheduler class
//
// ============================================================================
//
// Attempts to schedule a task (indicated by the function pointer passed to
// the concrete constructor) at a specified interval. For uncapped rates, that
// interval is simply as often as possible. For capped rates, the interval is
// specified by the rate parameter.
//

class TaskScheduler
{
public:
	virtual ~TaskScheduler() { }
	virtual void run() = 0;
	virtual dtime_t getNextTime() const = 0;
	virtual float getRemainder() const = 0;
};

class UncappedTaskScheduler : public TaskScheduler
{
public:
	UncappedTaskScheduler(void (*task)()) :
		mTask(task)
	{ }

	virtual ~UncappedTaskScheduler() { }

	virtual void run()
	{
		mTask();
	}

	virtual dtime_t getNextTime() const
	{
		return I_GetTime();
	}

	virtual float getRemainder() const
	{
		return 0.0f;
	}

private:
	void				(*mTask)();
};

class CappedTaskScheduler : public TaskScheduler
{
public:
	CappedTaskScheduler(void (*task)(), float rate, int max_count) :
		mTask(task), mMaxCount(max_count),
		mFrameDuration(I_ConvertTimeFromMs(1000) / rate),
		mAccumulator(mFrameDuration),
		mPreviousFrameStartTime(I_GetTime())
	{
	}

	virtual ~CappedTaskScheduler() { }

	virtual void run()
	{
		mFrameStartTime = I_GetTime();
		mAccumulator += mFrameStartTime - mPreviousFrameStartTime;
		mPreviousFrameStartTime = mFrameStartTime;

		int count = mMaxCount;

		while (mAccumulator >= mFrameDuration && count--)
		{
			mTask();
			mAccumulator -= mFrameDuration;
		}
	}

	virtual dtime_t getNextTime() const
	{
		return mFrameStartTime + mFrameDuration - mAccumulator;
	}

	virtual float getRemainder() const
	{
		// mAccumulator can be greater than mFrameDuration so only get the
		// time remaining until the next frame
		dtime_t remaining_time = mAccumulator % mFrameDuration;
		return (float)(double(remaining_time) / mFrameDuration);
	}

private:
	void				(*mTask)();
	const int			mMaxCount;
	const dtime_t		mFrameDuration;
	dtime_t				mAccumulator;
	dtime_t				mFrameStartTime;
	dtime_t				mPreviousFrameStartTime;
};

static TaskScheduler* simulation_scheduler;
static TaskScheduler* display_scheduler;

//
// D_InitTaskSchedulers
//
// Checks for external changes to the rate for the simulation and display
// tasks and instantiates the appropriate TaskSchedulers.
//
static void D_InitTaskSchedulers(void (*sim_func)(), void(*display_func)())
{
	bool capped_simulation = !timingdemo;
	bool capped_display = !timingdemo && capfps;

	static bool previous_capped_simulation = !capped_simulation;
	static bool previous_capped_display = !capped_display;
	static float previous_maxfps = -1.0f;

	if (capped_simulation != previous_capped_simulation)
	{
		previous_capped_simulation = capped_simulation;

		delete simulation_scheduler;

		if (capped_simulation)
			simulation_scheduler = new CappedTaskScheduler(sim_func, TICRATE, 4);
		else
			simulation_scheduler = new UncappedTaskScheduler(sim_func);
	}

	if (capped_display != previous_capped_display || maxfps != previous_maxfps)
	{
		previous_capped_display = capped_display;
		previous_maxfps = maxfps;

		delete display_scheduler;

		if (capped_display)
			display_scheduler = new CappedTaskScheduler(display_func, maxfps, 1);
		else
			display_scheduler = new UncappedTaskScheduler(display_func);
	}
}

void STACK_ARGS D_ClearTaskSchedulers()
{
	delete simulation_scheduler;
	delete display_scheduler;
	simulation_scheduler = NULL;
	display_scheduler = NULL;
}

//
// D_RunTics
//
// The core of the main game loop.
// This loop allows the game simulation timing to be decoupled from the display
// timing. If the the user selects a capped framerate and isn't using the
// -timedemo parameter, both the simulation and display functions will be called
// TICRATE times a second. If the framerate is uncapped, the simulation function
// will still be called TICRATE times a second but the display function will
// be called as often as possible. After each iteration through the loop,
// the program yields briefly to the operating system.
//
void D_RunTics(void (*sim_func)(), void(*display_func)())
{
	D_InitTaskSchedulers(sim_func, display_func);

	simulation_scheduler->run();

	// Use linear interpolation for rendering entities if the display
	// framerate is not synced with the simulation frequency.
	if ((maxfps == TICRATE && capfps) || timingdemo || paused || menuactive || step_mode)
		render_lerp_amount = FRACUNIT;
	else
		render_lerp_amount = simulation_scheduler->getRemainder() * FRACUNIT;

	display_scheduler->run();

	if (timingdemo)
		return;

	// Sleep until the next scheduled task.
	dtime_t simulation_wake_time = simulation_scheduler->getNextTime();
	dtime_t display_wake_time = display_scheduler->getNextTime();

	do
	{
		I_Yield();
	} while (I_GetTime() < MIN(simulation_wake_time, display_wake_time));			
}

VERSION_CONTROL (d_main_cpp, "$Id: d_main.cpp 3426 2012-11-19 17:25:28Z dr_sean $")
