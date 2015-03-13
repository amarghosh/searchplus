//this file is part of notepad++
//Copyright (C)2003 Don HO <donho@altern.org>
//
//This program is free software; you can redistribute it and/or
//modify it under the terms of the GNU General Public License
//as published by the Free Software Foundation; either
//version 2 of the License, or (at your option) any later version.
//
//This program is distributed in the hope that it will be useful,
//but WITHOUT ANY WARRANTY; without even the implied warranty of
//MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
//GNU General Public License for more details.
//
//You should have received a copy of the GNU General Public License
//along with this program; if not, write to the Free Software
//Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.

/*
Search+ Plugin
amarghosh @ gmail dot com
*/

#ifndef SEARCHPLUS_NPP_PLUGIN_H
#define SEARCHPLUS_NPP_PLUGIN_H

#include <Windows.h>

#include <regex>

using namespace std;

const TCHAR SP_PLUGIN_NAME[] = TEXT("Search+");

const int nbFunc = 2;

#define SEARCH_PLUS_PRIM_VERSION (0)
#define SEARCH_PLUS_SEC_VERSION (1)

#define SP_CUR_VER TEXT("0.1.5")

#define SP_MAX_PATTERN_LENGTH (100)
#define SP_MAX_STYLE_ID 5

/* Its a binary file now - should it be a text file of some kind? */
#define SP_CONF_FILE_NAME TEXT("searchplus.dat")


#define DBG_MSG(str) \
::MessageBox(NULL, TEXT(str), SP_PLUGIN_NAME, MB_OK);

class SearchPattern{
	TCHAR* text;
	regex *reg;
	bool case_sensitive;
	bool whole_words_flag;
	bool is_regex;
	int style_id;

	void generate_pattern(TCHAR *str, bool case_sensitive, bool whole_words_only, bool use_regex);
	void destroy();
	SearchPattern * next;

public:
	int count;
	int GetStyle();
	SearchPattern();
	~SearchPattern();
	SearchPattern(TCHAR *str, bool case_sensitive, bool whole_words_only, bool use_regex);
	SearchPattern *Clone();

	TCHAR * GetText();
	bool GetCaseSensitivity();
	bool GetWholeWordStatus();
	bool GetRegexStatus();
	bool Search(CHAR *text, int &from, int &length);

	void Update(TCHAR *str, bool case_sensitive, bool whole_words_only, bool use_regex);

	SearchPattern *GetNext();
	void SetNext(SearchPattern *pattern);

};

typedef enum{
	SP_SUCCESS,
	SP_FAILURE,
	SP_REGEX_ERR,
	SP_INVALID_ARGS,
	SP_ALREADY_EXISTS
}pat_err_t;

/*
** UI functions.
*/
void UI_Initialize(HWND parent_window);
void UI_Terminate();
void UI_ShowSettingsWindow();
void UI_ShowAboutWindow();
void UI_LoadPatterns(SearchPattern *pat_list);
void UI_HandleMatchingLine(int line_number, int line_length, SearchPattern *pat, int from, int match_length);
void UI_UpdateResultCount(int count);
void UI_HandleSearchComplete(int count, SearchPattern *pat_list);

/*
** Pattern related APIs
*/

/*
Returns -1 if the pattern doesn't exist.
else return the zero based index of the pattern in the list
*/
int PAT_GetIndex(TCHAR *str);

SearchPattern * PAT_Add(TCHAR *str, bool case_sensitive, bool whole_words_only, bool regex_flag);
SearchPattern * PAT_Update(int index, TCHAR *str, bool case_sensitive, bool whole_words_only, bool regex_flag);

pat_err_t PAT_Delete(int index);
pat_err_t PAT_DeleteAll();
pat_err_t PAT_ResetMatchCount();

int PAT_GetCount();

void PAT_GetPatterns(SearchPattern **pat_list);
void PAT_FreePatterns(SearchPattern *pat_list);

#if 0
/*
TODO: Return a copy of list instead of the original copy of the Head.
*/
SearchPattern *PAT_GetList();
#endif

/*
Interfaces to be implemented by the editor side of the plugin.

This is to make sure that npp specific functions are abstracted out so that UI/Pattern code
may be later reused for another editor as long as it provides these functionalities.
*/

/* Search for currently present patterns in the current document */
int Ed_Search();

int Ed_StopSearch();

/*
Number of lines in current document

TODO: interface to be updated to support searching in multiple tabs.
*/
int Ed_GetLineCount();

/* Fetch line */
int Ed_GetLine(int line_number, TCHAR *text, int max_length);

/*
Scroll the `line_number` to view and select `match_length` characters
from `start_index` in that particular line.

If editor supports tabs, switch to correct tab (based on last searched tab).

TODO: interface to be updated to support searching in multiple tabs.
*/
void Ed_GotoMatch(int line_number, int start_index, int match_length);

void Ed_HighlightWord(int line_number, int start_index, int match_length, int style);

/*
Get default word to be populated in the input text field.
`max_length` is an in-out variable that gives max characters allocated for `word`
and returns actual number of characters copied.

UI should be able to proceed seemlessly if this function is just a stub
*/
int Ed_GetCurrentWord(TCHAR *word, int& max_length);

void Ed_SetFocusOnEditor();

COLORREF Ed_GetColorFromStyle(int style);

void Ed_InitHighlightMatches();

#endif //SEARCHPLUS_NPP_PLUGIN_H

