// Emacs style mode select   -*- C++ -*-
//-----------------------------------------------------------------------------
//
// $Id: str_utils.cpp 2302 2011-06-27 03:17:21Z hypereye $
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
//  std::string utility functions
//
//-----------------------------------------------------------------------------

#include <algorithm>

#include "str_utils.h"

using namespace std;

string stdstr_toupper(const string& s)
{
	string upper(s);

	transform(upper.begin(), upper.end(), upper.begin(), ::toupper);

	return s;
}

// These can be removed for wx 2.9/3
string wxstr_tostdstr(const wxString& s)
{
	return string(s.mb_str());
}

wxString stdstr_towxstr(const string& s)
{
	return wxString(s.c_str(), wxConvUTF8);
}
