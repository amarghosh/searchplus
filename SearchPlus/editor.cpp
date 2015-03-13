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

/*
Notepad++ related functionalities and interfaces.

Keep UI and Npp separate.
*/


#include <cstdio>

#include "PluginInterface.h"

#include "searchplus.h"


static bool setCommand(size_t index, TCHAR *cmdName, PFUNCPLUGINCMD pFunc, ShortcutKey *sk = NULL, bool check0nInit = false);
static int SearchPlus_int(SearchPattern *patterns);
static HWND get_current_scintilla();

static COLORREF g_style_color_map[LP_MAX_STYLE_ID] = {RGB(255, 0, 0), RGB(0, 0, 255), RGB(34, 139, 34), RGB(255, 140, 0), RGB(208, 32, 144)};



struct SP_data_t{
	HWND npp;
	HWND scintilla_main;
	HWND scintilla_sec;
	bool exit_flag;
	bool search_flag;
	HANDLE search_thread;
	HANDLE search_event;
	TCHAR current_file[MAX_PATH];
};

static SP_data_t npp_data;


//
// The plugin data that Notepad++ needs
//
FuncItem funcItem[nbFunc];

ShortcutKey shortcuts[nbFunc] = {{false, true, false, 'Q'}};

BOOL APIENTRY DllMain( HANDLE hModule, 
                       DWORD  reasonForCall, 
                       LPVOID lpReserved )
{
    switch (reasonForCall)
    {
      case DLL_PROCESS_ATTACH:
        break;

      case DLL_PROCESS_DETACH:
        /* cleanup here */
        break;

      case DLL_THREAD_ATTACH:
        break;

      case DLL_THREAD_DETACH:
        break;
    }

    return TRUE;
}

void LP_SaveConfig(TCHAR *config_path)
{
	TCHAR path[MAX_PATH] = TEXT("");
	int length = 0, count = 0, loop = 0;
	FILE *fp = NULL;
	int version = 0;

	SearchPattern *pat_list = PAT_GetList();
	SearchPattern *temp_pat = pat_list;

	if(!config_path){
		DBG_MSG("Failed to get config path");
		return;
	}

	wcscpy(path, config_path);
	wcscat(path, TEXT("\\"));
	wcscat(path, SP_CONF_FILE_NAME);

	fp = _wfopen(path, TEXT("wb"));

	if(!fp){
		DBG_MSG("failed to open conf file for writing");
		return;
	}

	version = (SEARCH_PLUS_PRIM_VERSION << 16) | SEARCH_PLUS_SEC_VERSION;

	fwrite(&version, sizeof(int), 1, fp);

	count = 0;

	while(temp_pat){
		temp_pat = temp_pat->next;
		count++;
	}

	fwrite(&count, sizeof(int), 1, fp);

	temp_pat = pat_list;

	while(temp_pat){
		length = wcslen(temp_pat->getText());
		fwrite(&length, sizeof(int), 1, fp);
		fwrite(temp_pat->getText(), sizeof(TCHAR), length, fp);
		temp_pat = temp_pat->next;
	}

	fclose(fp);
}


void LP_LoadConfig(TCHAR *config_path)
{
	TCHAR path[MAX_PATH] = TEXT("");
	TCHAR data[SP_MAX_PATTERN_LENGTH];
	int length = 0, count = 0, loop = 0;
	int file_version = 0, expected_version = 0;
	FILE *fp = NULL;
	
	if(!config_path){
		DBG_MSG("Failed to get config path");
		return;
	}

	wcscpy(path, config_path);
	wcscat(path, TEXT("\\"));
	wcscat(path, SP_CONF_FILE_NAME);

	fp = _wfopen(path, TEXT("rb"));
	
	if(!fp){
		return;
	}

	expected_version = (SEARCH_PLUS_PRIM_VERSION << 16) | SEARCH_PLUS_SEC_VERSION;

	/* validate version  */
	if(1 != fread(&file_version, sizeof(int), 1, fp)){
		DBG_MSG("failed to read version");
		goto exit;
	}

	if(file_version != expected_version){
		DBG_MSG("Search+ version mismatch");
		goto exit;
	}

	if(1 != fread(&count, sizeof(int), 1, fp)){
		DBG_MSG("Failed to read count");
		goto exit;
	}

	for(loop = 0; loop < count; loop++){

		if(1 != fread(&length, sizeof(int), 1, fp)){
			DBG_MSG("Failed to read length");
			break;
		}

		if(length != fread(data, sizeof(TCHAR), length, fp)){
			DBG_MSG("Failed to read text");
		}

		data[length] = 0;
		PAT_Add(data);
	}

exit:
	fclose(fp);
}

void Ed_InitHighlightMatches()
{
	HWND scintilla = get_current_scintilla();
	TCHAR temp_currentfile[MAX_PATH] = {};
	int style_count;
	int doclength = 0;

	SendMessage(npp_data.npp, NPPM_GETFULLCURRENTPATH, MAX_PATH, LPARAM(temp_currentfile));

	if(wcscmp(temp_currentfile, npp_data.current_file)){
		SendMessage(npp_data.npp, NPPM_SWITCHTOFILE, 0, LPARAM(npp_data.current_file));
	}

	doclength = SendMessage(scintilla, SCI_GETLENGTH, 0, 0);

	SendMessage(scintilla, SCI_STARTSTYLING, 0, 0x1F);
	SendMessage(scintilla, SCI_SETSTYLING, doclength, 0);
	
	style_count = SendMessage(scintilla, SCI_GETSTYLEBITS, 0, 0);
	style_count = (1 << style_count) - 1;

	for(int i = 1; i < style_count; i++){
		SendMessage(scintilla, SCI_STYLESETBACK, i, Ed_GetColorFromStyle(i - 1));
		SendMessage(scintilla, SCI_STYLESETFORE, i, 0xFFFFFF);
	}
}

void Ed_HighlightWord(int line_number, int start_index, int match_length, int style)
{
	HWND scintilla = get_current_scintilla();
	int pos;

	pos = SendMessage(scintilla, SCI_POSITIONFROMLINE, line_number - 1, 0);
	pos += start_index;

	SendMessage(scintilla, SCI_STARTSTYLING, pos, 0x1F);
	SendMessage(scintilla, SCI_SETSTYLING, match_length, style + 1);
}


DWORD WINAPI SearchThread(LPVOID param)
{
	DWORD wait_res;
	ULONGLONG before, after;

	while(true){
		
		wait_res = WaitForSingleObject(npp_data.search_event, INFINITE);

		if(npp_data.exit_flag)
			break;

		if(wait_res == WAIT_OBJECT_0){

			SendMessage(npp_data.npp, NPPM_GETFULLCURRENTPATH, MAX_PATH, LPARAM(npp_data.current_file));

			before = GetTickCount64();
			npp_data.search_flag = true;

			PAT_ResetMatchCount();
			SearchPlus_int(PAT_GetList());

			npp_data.search_flag = false;
			after = GetTickCount64();
			after -= before;
		}
	}
	
	CloseHandle(npp_data.search_event);
	CloseHandle(npp_data.search_thread);

	return 0;
}

static void LP_Initialize(NppData notpadPlusData)
{
	TCHAR path[MAX_PATH] = TEXT("");
	DWORD threadid = -1;

	npp_data.npp = notpadPlusData._nppHandle;
	npp_data.scintilla_main = notpadPlusData._scintillaMainHandle;
	npp_data.scintilla_sec = notpadPlusData._scintillaSecondHandle;
	
	if(SendMessage(npp_data.npp, NPPM_GETPLUGINSCONFIGDIR, MAX_PATH, LPARAM(path))){
		LP_LoadConfig(path);
	}
	
	npp_data.search_event = CreateEvent(NULL, false, false, NULL);

	npp_data.search_thread = CreateThread(NULL, 0, SearchThread, NULL, 0, &threadid);

	UI_Initialize(notpadPlusData._nppHandle);

	setCommand(0, TEXT("Search+"), UI_ShowSettingsWindow, shortcuts, false);
}

static void LP_Terminate()
{
	TCHAR path[MAX_PATH] = TEXT("");
	DWORD thread_exit = 0;
	
	if(SendMessage(npp_data.npp, NPPM_GETPLUGINSCONFIGDIR, MAX_PATH, LPARAM(path))){
		LP_SaveConfig(path);
	}

	if(npp_data.search_flag){
		/* force quit the search thread */
		TerminateThread(npp_data.search_thread, thread_exit);
		CloseHandle(npp_data.search_event);
		CloseHandle(npp_data.search_thread);
	}
	else{
		npp_data.exit_flag = true;
		SetEvent(npp_data.search_event);
	}

	UI_Terminate();
	PAT_DeleteAll();
}

extern "C" __declspec(dllexport) void setInfo(NppData notpadPlusData)
{
	LP_Initialize(notpadPlusData);
}

extern "C" __declspec(dllexport) const TCHAR * getName()
{
	return SP_PLUGIN_NAME;
}

extern "C" __declspec(dllexport) FuncItem * getFuncsArray(int *nbF)
{
	*nbF = nbFunc;
	return funcItem;
}


extern "C" __declspec(dllexport) void beNotified(SCNotification *notifyCode)
{
	switch (notifyCode->nmhdr.code) 
	{
		case NPPN_SHUTDOWN:
		{
			LP_Terminate();
		}
		break;

		default:
			return;
	}
}


// Here you can process the Npp Messages 
// I will make the messages accessible little by little, according to the need of plugin development.
// Please let me know if you need to access to some messages :
// http://sourceforge.net/forum/forum.php?forum_id=482781
//
extern "C" __declspec(dllexport) LRESULT messageProc(UINT Message, WPARAM wParam, LPARAM lParam)
{
	return TRUE;
}

#ifdef UNICODE
extern "C" __declspec(dllexport) BOOL isUnicode()
{
    return TRUE;
}
#endif
//
// This function help you to initialize your plugin commands
//
static bool setCommand(size_t index, TCHAR *cmdName, PFUNCPLUGINCMD pFunc, ShortcutKey *sk, bool check0nInit) 
{
    if (index >= nbFunc)
        return false;

    if (!pFunc)
        return false;

    lstrcpy(funcItem[index]._itemName, cmdName);
    funcItem[index]._pFunc = pFunc;
    funcItem[index]._init2Check = check0nInit;
    funcItem[index]._pShKey = sk;

    return true;
}

static HWND get_current_scintilla()
{
	int res = 0;

	SendMessage(npp_data.npp, NPPM_GETCURRENTSCINTILLA, 0, WPARAM(&res));

	if(res == -1){
		DBG_MSG("Oops: Error in Scintilla handle : contact developer of Search+ plugin");
		return NULL;
	}

	if(res){
		return npp_data.scintilla_sec;
	}
	else{
		return npp_data.scintilla_main;
	}
}

int Ed_GetLineCount()
{
	HWND scintilla = get_current_scintilla();

	if(scintilla){
		return SendMessage(scintilla, SCI_GETLINECOUNT, 0, 0);
	}
	else{
		return -1;
	}
}

void Ed_GotoMatch(int line_number, int start_index, int match_length)
{
	HWND scintilla = get_current_scintilla();
	int pos = 0;

	TCHAR temp_currentfile[MAX_PATH] = {};

	SendMessage(npp_data.npp, NPPM_GETFULLCURRENTPATH, MAX_PATH, LPARAM(temp_currentfile));

	if(wcscmp(temp_currentfile, npp_data.current_file)){
		SendMessage(npp_data.npp, NPPM_SWITCHTOFILE, 0, LPARAM(npp_data.current_file));
	}

	SendMessage(scintilla, SCI_GOTOLINE, line_number - 1, 0);

	pos = SendMessage(scintilla, SCI_POSITIONFROMLINE, line_number - 1, 0);
	SendMessage(scintilla, SCI_SETSELECTION, pos + start_index + match_length,
		pos + start_index);

	SetFocus(scintilla);
}

int Ed_GetCurrentWord(TCHAR *word, int& max_length)
{
	CHAR *selected_text = NULL;
	HWND scintilla;
	int sel_length = 0;

	scintilla = get_current_scintilla();

	/* populate input field with selected word or word under cursor */

	sel_length = SendMessage(scintilla, SCI_GETSELECTIONEND, 0, 0) - 
		SendMessage(scintilla, SCI_GETSELECTIONSTART, 0, 0);

	if(sel_length){
		selected_text = new CHAR[sel_length + 1];
		SendMessage(scintilla, SCI_GETSELTEXT, 0, WPARAM(selected_text));
	}
	else{
		int cur, start, end;
		Sci_TextRange text_range;

		cur = SendMessage(scintilla, SCI_GETCURRENTPOS, 0, 0);
		start = SendMessage(scintilla, SCI_WORDSTARTPOSITION, cur, true);
		end = SendMessage(scintilla, SCI_WORDENDPOSITION, start, true);
		sel_length = end - start + 1;

		selected_text = new CHAR[sel_length + 1];

		text_range.chrg.cpMin = start;
		text_range.chrg.cpMax = end;
		text_range.lpstrText = selected_text;

		SendMessage(scintilla, SCI_GETTEXTRANGE, 0, WPARAM(&text_range));
	}

	if(sel_length >= max_length){
		selected_text[max_length - 1] = 0;
		sel_length = max_length - 1;
	}

	mbstowcs(word, selected_text, sel_length);

	max_length = sel_length;

	return 0;
}

void Ed_SetFocusOnEditor()
{
	SetFocus(get_current_scintilla());
}


#if 1
#define CHUNK_SIZE (1023)

struct search_res_t{
	SearchPattern *pat;
	int from; /* char index from start of document */
	int length;
	search_res_t *next;

	search_res_t(SearchPattern *pattern, int match_from, int match_length){
		pat = pattern;
		from = match_from;
		length = match_length;
		next = NULL;
	};
};

static void add_match(SearchPattern *pat, int from, int length, search_res_t **results)
{
	search_res_t *res;
	search_res_t *parent = NULL;
	search_res_t *iter = *results;

	res = new search_res_t(pat, from, length);

	if(!(*results)){
		*results = res;
	}
	else{
		
		while(iter != NULL && iter->from <= res->from){
			parent = iter;
			iter = iter->next;
		}

		if(!parent){
			res->next = iter;
			*results = res;
		}
		else{
			res->next = parent->next;
			parent->next = res;
		}
	}
}

static int handle_matches(HWND scintilla, char *text, search_res_t **results)
{
	search_res_t *iter = *results;
	int match_count = 0;
	int line_number, line_start, line_length;

	CHAR *buffer = NULL;
	int buflength = 1024;

	buffer = new CHAR[buflength + 1];

	while(iter){

		match_count++;

		line_number = SendMessage(scintilla, SCI_LINEFROMPOSITION, iter->from, 0);
		line_start = SendMessage(scintilla, SCI_POSITIONFROMLINE, line_number, 0);
		line_length = SendMessage(scintilla, SCI_LINELENGTH, line_number, 0);

		if(line_length > buflength){
			delete buffer;
			buflength = line_length;
			buffer = new CHAR[line_length + 1];
		}

		SendMessage(scintilla, SCI_GETLINE, line_number, LPARAM(buffer));

		UI_HandleMatchingLine(line_number + 1, line_length, buffer, iter->pat, iter->from - line_start, iter->length);

		*results = iter->next;
		delete iter;
		iter = *results;
	}

	delete buffer;

	return match_count;
}


int Ed_Search()
{
	SearchPattern *pat_list = PAT_GetList();

	if(!pat_list){
		DBG_MSG("No patterns to search for");
		return -1;
	}

	SetEvent(npp_data.search_event);	

	return 0;
}

COLORREF Ed_GetColorFromStyle(int style)
{
	return g_style_color_map[style % LP_MAX_STYLE_ID];
}



/*
Fetch a chunk of text from scintilla (line aligned) and search thru it.
Store the matched positions in ascending order of their start positions.
At the end of each iteration, send the matches to the UI and reset the list.

With appropriate value for CHUNK_SIZE, number of fetches from scintilla can be 
reduced while keeping copying from becoming a factor that slows down the process.

This approach is faster than fetching line by line.

Too high or too low values of chunk sizes slowed down this approach. 
Anywhere between 1k to 10k seems good.
*/

/*
TODO: Check if scintilla search is faster when searching thru larger files for a rarely occurring keyword.
Such searches are seen to be at least three times faster using npp native search than this logic.
So why reinvent a hexagonal wheel?
- Search results to be kept in a list with a head and tail to avoid Painter's algorithm
	- Order is relevant: otherwise user will feel search is happening backwards
- Each keyword will have to be searched separately; UI can no longer assume that lines are 
	fed in order. It needs to insert them in order (use binary search as number of results can be high)

*/
static int SearchPlus_int(SearchPattern *pat_list)
{
	SearchPattern *temp_pat = pat_list;

	/* total number of characters and lines in the document */
	int docsize = 0, linecount = 0;

	CHAR *buffer = NULL;
	CHAR *search_buffer = NULL;

	HWND scintilla = NULL;
	int handle = -1;
	Sci_TextRange trange;

	int current_chunk_size = 0;

	/* current position in the entire document (number of characters covered so far)*/
	int doc_pos = 0;
	/* position in the current buffer (number of characters covered so far)*/
	int buf_pos = 0;

	int line, pos;

	int match_count = 0, match_count_temp;
	int match_start = 0, match_length = 0;

	/* list of results found in the current iteration */
	search_res_t *results = NULL;
	
	if(!temp_pat){
		DBG_MSG("No patterns to search for");
		return -1;
	}

	scintilla = get_current_scintilla();

	linecount = SendMessage(scintilla, SCI_GETLINECOUNT, 0, 0);
	docsize = SendMessage(scintilla, SCI_GETLENGTH, 0, 0);

	buffer = new CHAR[(CHUNK_SIZE + 1)* sizeof(CHAR)];

	trange.chrg.cpMin = 0;
	trange.chrg.cpMax = CHUNK_SIZE;
	trange.lpstrText = buffer;

	while(doc_pos != docsize){

		/* Fetch next N lines of text so that number of bytes <= CHUNK_SIZE */
		line = SendMessage(scintilla, SCI_LINEFROMPOSITION, doc_pos + CHUNK_SIZE, 0);
		pos = SendMessage(scintilla, SCI_POSITIONFROMLINE, line, 0);

		if(pos == doc_pos){
			/* handle last line */
			pos += (docsize - doc_pos > CHUNK_SIZE) ? CHUNK_SIZE : docsize - doc_pos;
		}

		trange.chrg.cpMin = doc_pos;
		trange.chrg.cpMax = pos;

		buffer[0] = 0;
		current_chunk_size = SendMessage(scintilla, SCI_GETTEXTRANGE, 0, LPARAM(&trange));

		temp_pat = pat_list;

		while(temp_pat){

			search_buffer = buffer;
			buf_pos = 0;

			while(temp_pat->search(search_buffer, match_start, match_length)){

				add_match(temp_pat, doc_pos + buf_pos + match_start, match_length, &results);
				search_buffer += match_start + match_length;
				buf_pos += match_start + match_length;
			}

			temp_pat = temp_pat->next;
		}

		match_count_temp = handle_matches(scintilla, buffer, &results);

		if(match_count_temp){
			match_count += match_count_temp;
			UI_UpdateResultCount(match_count);
		}
		doc_pos += current_chunk_size;
	}
	
	delete buffer;

	UI_HandleSearchComplete(match_count, pat_list);

	return match_count;
}
#else

/* 
Fetch one line at a time from scintilla and search for each pattern in it.
Too much copying involved if the document contains a huge number of lines
*/
int SearchPlus_int()
{
	CHAR *line_buff = NULL;
	int handle = -1;
	int linecount = 0, loop, line_length;
	HWND scintilla = NULL;
	int current_buffer_length = 1024;
	SearchPattern *pat_list = PAT_GetList();
	SearchPattern *temp_pat = pat_list;
	int match_count = -1;
	int match_start, match_length;

	if(!temp_pat){
		DBG_MSG("No patterns to search for");
		return -1;
	}

	SendMessage(nppData._nppHandle, NPPM_GETCURRENTSCINTILLA, 0, LPARAM(&handle));
	 
	if(handle == -1){
		DBG_MSG("Failed to get current scintilla");
		return -1;
	}

	scintilla = handle ? nppData._scintillaSecondHandle : nppData._scintillaMainHandle;

	linecount = SendMessage(scintilla, SCI_GETLINECOUNT, 0, 0);

	if(!linecount){
		goto exit;
	}

	line_buff = (CHAR*)calloc(current_buffer_length + 1, sizeof(TCHAR));

	if(!line_buff){
		DBG_MSG("Memory Error 0 : Please contact the developer");
		goto exit;
	}

	match_count = 0;
	
	for(loop = 0; loop < linecount; loop++){

		line_length = ::SendMessage(scintilla, SCI_LINELENGTH, loop, 0);

		if(!line_length)
			continue;

		if(line_length > current_buffer_length){
			current_buffer_length = line_length;
			line_buff = (CHAR*)realloc(line_buff, line_length + 1);

			if(!line_buff){
				DBG_MSG("Memory Error 1 : Please contact the developer");
				goto exit;
			}
		}

		::SendMessage(scintilla, SCI_GETLINE, loop, (LPARAM)line_buff);

		line_buff[line_length] = 0;

#if 0 /* trim eol chars */
		while(line_length && (line_buff[line_length - 1] == TEXT('\n') || line_buff[line_length - 1] == TEXT('\r'))){
			line_buff[line_length - 1] = TEXT('\0');
			line_length--;
		}

		if(!line_length){
			continue;
		}
#endif		
		temp_pat = pat_list;


		while(temp_pat){

			match_start = match_length = 0;

			if(temp_pat->search(line_buff, match_start, match_length)){

				handle_matching_line(loop, line_length, line_buff, temp_pat, match_start, match_length);
				match_count++;
				break;
			}

			temp_pat = temp_pat->next;
		}
	}

exit:
	if(line_buff)
		free(line_buff);

	return match_count;
}



#endif 

