/*
Search+ Plugin
amarghosh @ gmail dot com
*/


#include <Windows.h>
#include <CommCtrl.h>

#include "searchplus.h"
#include "ui_int.h"

#define EMPTY_STRING TEXT("")


static const TCHAR sp_wnd_classname[] = TEXT("SP_Window");
static const TCHAR about_wnd_classname[] = TEXT("SP_AboutWindow");

/* plugin window procedure */
static LRESULT CALLBACK WndProc(HWND hWnd, UINT iMessage, WPARAM wParam, LPARAM lParam);

/* about window procedure */
static LRESULT CALLBACK AboutWndProc(HWND hWnd, UINT iMessage, WPARAM wParam, LPARAM lParam);

DWORD WINAPI UI_Thread(LPVOID param);

/*
window procedures to handle tabbed navigation, simulate button click on enter key
and to hide window on hitting escape.
*/
static LRESULT CALLBACK TabbedControlProc(HWND hWnd, UINT iMessage, WPARAM wParam, LPARAM lParam);

static SearchPlusUI *ui_data;


/* draw the text from left edge of the given rectangle and moves rectangles horizontal edges */
static void draw_text(HDC hdc, TCHAR *text, int length, LPRECT rect, COLORREF textcolor)
{
	int rightend = rect->right;

	SetTextColor(hdc, textcolor);
	DrawText(hdc, text, length, rect, DT_FLAGS | DT_CALCRECT);
	DrawText(hdc, text, length, rect, DT_FLAGS);
	rect->left = rect->right;
	rect->right = rightend;
}


MatchedLine::MatchedLine(int line_number, int total_length, int match_from, int match_length, SearchPattern *pattern)
{
	this->line_number = line_number;
	this->line_length = total_length;

	this->text = NULL;

	this->match_list = new MatchedSegment(match_from, match_length, pattern);
}

MatchedLine::~MatchedLine()
{
	if(text)
		delete text;

	delete match_list;
}

void MatchedLine::FetchLine()
{
	if(text){
		return;
	}

	text = new TCHAR[line_length + 1];

	text[0] = 0;

	Ed_GetLine(line_number, text, line_length + 1);
}

void MatchedLine::AddMatch(int match_from, int match_length, SearchPattern *pattern)
{
	MatchedSegment *last_match = this->match_list;

	if(!last_match){
		this->match_list = new MatchedSegment(match_from, match_length, pattern);
	}
	else{

		while(last_match->next){
			last_match = last_match->next;
		}

		if(match_from < last_match->from){
			/* matches are expected in ascending order of starting position */
			return;
		}

		if((match_from >= last_match->from) &&
				(match_from + match_length <= last_match->from + last_match->length)){

			/*
			if multiple keywords match the same sequence or subsequence of characters,
			highlight only the first one and ignore the rest.
			*/
			return;
		}

		if((match_from < last_match->from + last_match->length) &&
				(match_from + match_length > last_match->from + last_match->length)){

			match_length = match_from + match_length - (last_match->from + last_match->length);
			match_from = last_match->from + last_match->length;
		}

		last_match->next = new MatchedSegment(match_from, match_length, pattern);
	}
}

MatchedSegment::MatchedSegment(int match_start, int match_length, SearchPattern *pattern)
{
	from = match_start;
	length = match_length;
	this->pattern = pattern;
	next = NULL;
}

MatchedSegment::~MatchedSegment()
{
	if(next){
		delete next;
	}
}

void MatchedLine::Draw(HDC hdc, LPRECT full_rect)
{
	int cur_pos = 0;
	int segment_length = 0;
	RECT segment_rect = *full_rect;
	MatchedSegment *current_segment = this->match_list;
	TCHAR *text_segment;

	text_segment = new TCHAR[this->line_length + 1];

	if(!text){
		FetchLine();
	}

	while(current_segment){

		if(cur_pos < current_segment->from){

			segment_length = current_segment->from - cur_pos;
			wcsncpy_s(text_segment, line_length + 1, text + cur_pos, segment_length);
			text_segment[segment_length] = 0;

			draw_text(hdc, text_segment, segment_length, &segment_rect, COLOR_NORMAL);

			cur_pos += segment_length;
		}

		wcsncpy_s(text_segment, line_length + 1, text + cur_pos, current_segment->length);
		text_segment[current_segment->length] = 0;

		draw_text(hdc, text_segment, current_segment->length, &segment_rect, 
			Ed_GetColorFromStyle(current_segment->pattern->GetStyle()));

		cur_pos += current_segment->length;

		current_segment = current_segment->next;
	}

	segment_length = this->line_length - cur_pos;

	if(segment_length){
		wcsncpy_s(text_segment, line_length + 1, text + cur_pos, segment_length);
		draw_text(hdc, text_segment, segment_length, &segment_rect, COLOR_NORMAL);
	}

	delete text_segment;
}

SearchPlusUI::SearchPlusUI(HWND parent)
{
	close_flag = false;
	stop_flag = false;

	editor_handle = parent;

	about_window = NULL;
	main_window = NULL;

	font = CreateFont(14, 0, 0, 0, FW_DONTCARE, FALSE, FALSE, FALSE, DEFAULT_CHARSET,
		OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS, DEFAULT_QUALITY, DEFAULT_PITCH | FF_SWISS,
		TEXT("Verdana"));

	register_window_classes();

	thread_handle = NULL;
}

void SearchPlusUI::CreatePluginWindow()
{
	SearchPattern *pat_list = NULL;

	create_sp_window();

	PAT_GetPatterns(&pat_list);
	load_keywords(pat_list);
	PAT_FreePatterns(pat_list);
}

int SearchPlusUI::LaunchSearchPlusWindow()
{
	if(thread_handle == NULL){
		thread_handle = CreateThread(NULL, 0, UI_Thread, this, 0, &thread_id);
	}
	else{
		LoadCurrentWord();
	}

	return 0;
}

void SearchPlusUI::LoadCurrentWord()
{
	TCHAR current_word[SP_MAX_PATTERN_LENGTH] = {0};
	int sel_length = SP_MAX_PATTERN_LENGTH;

	Ed_GetCurrentWord(current_word, sel_length);

	SendMessage(input_field, WM_SETTEXT, 0, WPARAM(current_word));
	SendMessage(input_field, EM_SETSEL, sel_length, sel_length);

	ShowWindow(main_window, SW_SHOW);
	SetFocus(input_field);
}

SearchPlusUI::~SearchPlusUI()
{
	if(!close_flag){
		close_window();
	}

	DeleteObject(font);
}

void SearchPlusUI::Terminate()
{
	PostMessage(main_window, SPM_TERMINATE, 0, 0);
}

void SearchPlusUI::register_window_classes()
{
	WNDCLASS wndclass;
	WNDCLASS abtclass;

	wndclass.style = CS_HREDRAW | CS_VREDRAW;
	wndclass.lpfnWndProc = WndProc;
	wndclass.cbClsExtra = 0;
	wndclass.cbWndExtra = 0;
	wndclass.hInstance = HINSTANCE(SP_InstanceSPWndClass);
	wndclass.hIcon = 0;
	wndclass.hCursor = NULL;
	wndclass.hbrBackground = (HBRUSH) GetStockObject(WHITE_BRUSH);
	wndclass.lpszMenuName = NULL;
	wndclass.lpszClassName = sp_wnd_classname;

	if (!::RegisterClass(&wndclass)){
		::MessageBox(NULL, TEXT("register class failed"), TEXT("Err: Search+ plugin"), MB_OK);
	}

	abtclass.style = CS_HREDRAW | CS_VREDRAW;
	abtclass.lpfnWndProc = AboutWndProc;
	abtclass.cbClsExtra = 0;
	abtclass.cbWndExtra = 0;
	abtclass.hInstance = HINSTANCE(SP_InstanceAboutWindowClass);
	abtclass.hIcon = 0;
	abtclass.hCursor = NULL;
	abtclass.hbrBackground = (HBRUSH) GetStockObject(WHITE_BRUSH);
	abtclass.lpszMenuName = NULL;
	abtclass.lpszClassName = about_wnd_classname;

	if (!::RegisterClass(&abtclass)){
		::MessageBox(NULL, TEXT("about wnd register class failed"), TEXT("Err: Search+ plugin"), MB_OK);
	}
}

bool SearchPlusUI::create_sp_window()
{
	RECT rect = {0, 0, 0, 0};
	HWND desktop;
	int x, y;

	for(int loop = 0; loop < MAX_TAB_CONTROLS; loop++){

		tab_controls[loop].hwnd = 0;
		tab_controls[loop].proc = NULL;
	}

	desktop = GetDesktopWindow();

	GetClientRect(desktop, &rect);

	x = (rect.right - SP_UIPANEL_MIN_W) / 2;
	y = (rect.bottom - SP_WINDOW_HEIGHT) / 2;


	main_window = ::CreateWindowEx(
		WS_EX_TOOLWINDOW | WS_EX_LAYERED,
		sp_wnd_classname,
		TEXT("Search+"),
		WS_SYSMENU | WS_SIZEBOX | WS_TABSTOP | WS_GROUP | WS_MAXIMIZEBOX | WS_MINIMIZEBOX | WS_CLIPCHILDREN,
		x, y, SP_UIPANEL_MIN_W, SP_WINDOW_HEIGHT,
		this->editor_handle, NULL,
		(HINSTANCE)SP_InstanceSPWndClass,
		0);

	mode = UI_MODE_NORMAL;

	return true;
}

void SearchPlusUI::handle_window_create(HWND hwnd)
{
	create_controls(hwnd);

	/*
	Update the window procedures of the controls to be included in tab navigation here
	Keep the original and call it from the custom procedure
	The procedures for similar controls can be different (not all buttons have same proc)
	for ex, a button with a tooltip has a different procedure than the normal button
	*/
	tab_controls[0].hwnd = input_field;
	tab_controls[1].hwnd  = reset_button;
	tab_controls[2].hwnd  = remove_button;
	tab_controls[3].hwnd  = add_button;
	tab_controls[4].hwnd  = search_button;
	tab_controls[5].hwnd  = cancel_button;
	tab_controls[6].hwnd  = update_button;
	tab_controls[7].hwnd  = keyword_listbox;

	for(int loop = 0; loop < MAX_TAB_CONTROLS; loop++){
		tab_controls[loop].proc = (WNDPROC)SetWindowLongPtr(tab_controls[loop].hwnd, GWLP_WNDPROC, (LONG_PTR)TabbedControlProc);
	}
}

void SearchPlusUI::create_controls(HWND parent)
{
	TOOLINFO toolinfo = {0};

	input_field = ::CreateWindowEx(WS_EX_CLIENTEDGE,
		WC_EDIT, EMPTY_STRING,
		WS_VISIBLE | WS_CHILD | WS_TABSTOP | WS_GROUP | ES_AUTOHSCROLL,
		CW_USEDEFAULT, CW_USEDEFAULT,
		CW_USEDEFAULT, CW_USEDEFAULT,
		parent, HMENU(SP_InstanceInputField),
		HINSTANCE(SP_InstanceInputField), 0);

	add_button = ::CreateWindowEx(0, WC_BUTTON,	BTN_LBL_ADD,
		WS_VISIBLE | WS_CHILD | WS_TABSTOP,
		CW_USEDEFAULT, CW_USEDEFAULT,
		CW_USEDEFAULT, CW_USEDEFAULT,
		parent, HMENU(SP_InstanceAddButton),
		HINSTANCE(SP_InstanceAddButton), 0);

	keyword_listbox = ::CreateWindowEx(WS_EX_CLIENTEDGE, WC_LISTBOX, EMPTY_STRING,
		WS_VISIBLE | WS_CHILD | WS_TABSTOP | WS_VSCROLL | LBS_WANTKEYBOARDINPUT |
			LBS_OWNERDRAWFIXED | LBS_NOTIFY,
		CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT,
		parent,	HMENU(SP_InstancePatternList),
		HINSTANCE(SP_InstancePatternList), 0);

	output_area  = ::CreateWindowEx(WS_EX_CLIENTEDGE,
		WC_LISTBOX, EMPTY_STRING,
		WS_VISIBLE | WS_CHILD | WS_TABSTOP |  WS_HSCROLL | WS_VSCROLL |
			LBS_OWNERDRAWFIXED | LBS_NOTIFY | LBS_WANTKEYBOARDINPUT,
		CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT,
		parent, HMENU(SP_InstanceOutputArea),
		HINSTANCE(SP_InstanceOutputArea), 0);

	remove_button = ::CreateWindowEx(0, WC_BUTTON, BTN_LBL_DELETE,
		WS_VISIBLE | WS_CHILD | WS_TABSTOP,
		CW_USEDEFAULT, CW_USEDEFAULT,
		CW_USEDEFAULT, CW_USEDEFAULT,
		parent, HMENU(SP_InstanceRemoveButton),
		HINSTANCE(SP_InstanceRemoveButton), 0);

	reset_button = ::CreateWindowEx(0, WC_BUTTON, BTN_LBL_RESET,
		WS_VISIBLE | WS_CHILD | WS_TABSTOP,
		CW_USEDEFAULT, CW_USEDEFAULT,
		CW_USEDEFAULT, CW_USEDEFAULT,
		parent, HMENU(SP_InstanceClearButton),
		HINSTANCE(SP_InstanceClearButton), 0);

	search_button = CreateWindowEx(0, WC_BUTTON, BTN_LBL_SEARCH,
		WS_VISIBLE | WS_CHILD | WS_TABSTOP,
		CW_USEDEFAULT, CW_USEDEFAULT,
		CW_USEDEFAULT, CW_USEDEFAULT,
		parent, HMENU(SP_InstanceSearchButton),
		HINSTANCE(SP_InstanceSearchButton), 0);

	list_label = CreateWindowEx(0, WC_STATIC, LIST_LBL_NORMAL,
		WS_VISIBLE | WS_CHILD | SS_EDITCONTROL,
		CW_USEDEFAULT, CW_USEDEFAULT,
		CW_USEDEFAULT, CW_USEDEFAULT,
		parent, NULL, NULL, 0);

	result_label = CreateWindowEx(0, WC_STATIC, EMPTY_STRING,
		WS_VISIBLE | WS_CHILD | SS_EDITCONTROL,
		CW_USEDEFAULT, CW_USEDEFAULT,
		CW_USEDEFAULT, CW_USEDEFAULT,
		parent, NULL, NULL, 0);

	copy_button = CreateWindowEx(0, WC_BUTTON, BTN_LBL_COPY,
		WS_VISIBLE | WS_CHILD | WS_TABSTOP | WS_DISABLED,
		CW_USEDEFAULT, CW_USEDEFAULT,
		CW_USEDEFAULT, CW_USEDEFAULT,
		parent, HMENU(SP_InstanceClipboardButton),
		HINSTANCE(SP_InstanceClipboardButton), 0);

	highlight_button = CreateWindowEx(0, WC_BUTTON, BTN_LBL_HIGHLIGHT,
		WS_VISIBLE | WS_CHILD | WS_TABSTOP | WS_DISABLED,
		CW_USEDEFAULT, CW_USEDEFAULT,
		CW_USEDEFAULT, CW_USEDEFAULT,
		parent, HMENU(SP_InstanceHighlightButton),
		HINSTANCE(SP_InstanceHighlightButton), 0);

	update_button = CreateWindowEx(0, WC_BUTTON, BTN_LBL_UPDATE,
		WS_CHILD | WS_TABSTOP,
		CW_USEDEFAULT, CW_USEDEFAULT,
		CW_USEDEFAULT, CW_USEDEFAULT,
		parent, HMENU(SP_InstanceUpdateButton),
		HINSTANCE(SP_InstanceUpdateButton), 0);

	cancel_button = CreateWindowEx(0, WC_BUTTON, BTN_LBL_CANCEL,
		WS_CHILD | WS_TABSTOP,
		CW_USEDEFAULT, CW_USEDEFAULT,
		CW_USEDEFAULT, CW_USEDEFAULT,
		parent, HMENU(SP_InstanceCancelButton),
		HINSTANCE(SP_InstanceCancelButton), 0);

	next_button = CreateWindowEx(0, WC_BUTTON, BTN_LBL_NEXT,
		WS_CHILD | WS_TABSTOP | WS_VISIBLE,
		CW_USEDEFAULT, CW_USEDEFAULT,
		CW_USEDEFAULT, CW_USEDEFAULT,
		parent, HMENU(SP_InstanceNextButton),
		HINSTANCE(SP_InstanceNextButton), 0);

	prev_button = CreateWindowEx(0, WC_BUTTON, BTN_LBL_PREV,
		WS_CHILD | WS_TABSTOP | WS_VISIBLE,
		CW_USEDEFAULT, CW_USEDEFAULT,
		CW_USEDEFAULT, CW_USEDEFAULT,
		parent, HMENU(SP_InstancePrevButton),
		HINSTANCE(SP_InstancePrevButton), 0);


	case_sensitivity_checkbox = ::CreateWindowEx(0, WC_BUTTON, CHK_BOX_LABEL_CASE,
		WS_VISIBLE | WS_CHILD | WS_TABSTOP | BS_AUTOCHECKBOX,
		CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT,
		parent, HMENU(SP_InstanceCaseSensitivityCB),
		HINSTANCE(SP_InstanceCaseSensitivityCB), 0);

	whole_word_checkbox = ::CreateWindowEx(0, WC_BUTTON, CHK_BOX_LABEL_WORD,
		WS_VISIBLE | WS_CHILD | WS_TABSTOP | BS_AUTOCHECKBOX,
		CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT,
		parent, HMENU(SP_InstanceMatchWordCB),
		HINSTANCE(SP_InstanceMatchWordCB), 0);

	plaintext_checkbox = ::CreateWindowEx(0, WC_BUTTON, CHK_BOX_LABEL_REGEX,
		WS_VISIBLE | WS_CHILD | WS_TABSTOP | BS_AUTOCHECKBOX,
		CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT,
		parent, HMENU(SP_InstanceRegexCB),
		HINSTANCE(SP_InstanceRegexCB), 0);

	clipboard_tooltip = assign_tooltip(copy_button, parent, TEXT("Copy results to clipboard"));
	highlight_tooltip = assign_tooltip(highlight_button, parent, TEXT("Highlight results in Notepad++"));
	add_tooltip = assign_tooltip(add_button, parent, TEXT("Add keywords to search for"));
	whole_word_cb_tooltip = assign_tooltip(whole_word_checkbox, parent, TEXT("Adds \\b at start and end of keyword"));
	regex_cb_tooltip = assign_tooltip(plaintext_checkbox, parent, TEXT("Treat keyword as plain text (escapes regex special chars)"));
	keyword_tooltip = assign_tooltip(keyword_listbox, parent, TEXT("Click to view flags. Double click to edit."));

	next_btn_tooltip = assign_tooltip(next_button, parent, TEXT("Goto next match of the selected keyword."));
	prev_btn_tooltip = assign_tooltip(prev_button, parent, TEXT("Goto previous match of the selected keyword."));

	if(font){
		SendMessage(add_button, WM_SETFONT, WPARAM(font), 0);
		SendMessage(search_button, WM_SETFONT, WPARAM(font), 0);
		SendMessage(reset_button, WM_SETFONT, WPARAM(font), 0);
		SendMessage(highlight_button, WM_SETFONT, WPARAM(font), 0);
		SendMessage(copy_button, WM_SETFONT, WPARAM(font), 0);
		SendMessage(remove_button, WM_SETFONT, WPARAM(font), 0);
		SendMessage(input_field, WM_SETFONT, WPARAM(font), 0);

		SendMessage(keyword_listbox, WM_SETFONT, WPARAM(font), 0);
		SendMessage(list_label, WM_SETFONT, WPARAM(font), 0);
		SendMessage(result_label, WM_SETFONT, WPARAM(font), 0);
		SendMessage(output_area, WM_SETFONT, WPARAM(font), 0);

		SendMessage(case_sensitivity_checkbox, WM_SETFONT, WPARAM(font), 0);
		SendMessage(plaintext_checkbox, WM_SETFONT, WPARAM(font), 0);
		SendMessage(whole_word_checkbox, WM_SETFONT, WPARAM(font), 0);

		SendMessage(update_button, WM_SETFONT, WPARAM(font), 0);
		SendMessage(cancel_button, WM_SETFONT, WPARAM(font), 0);

		SendMessage(next_button, WM_SETFONT, WPARAM(font), 0);
		SendMessage(prev_button, WM_SETFONT, WPARAM(font), 0);
	}
}

void SearchPlusUI::resize_controls()
{
	RECT parent_rect;
	int ui_panel_width, width, height;

	/*
	Mid point between input list and input field
	The buttons are aligned based on this point.
	*/
	int x_offset;

	::GetClientRect(main_window, &parent_rect);

	width = parent_rect.right - parent_rect.left;
	height = parent_rect.bottom - parent_rect.top;

	ui_panel_width = SP_UIPANEL_MIN_W;

	x_offset = SP_UIPANEL_MARGIN_H + KW_INPUT_W + SP_PADDING_H;

	if(width > SP_UIPANEL_MIN_W){
		x_offset += (width - SP_UIPANEL_MIN_W) / 2;
	}

	MoveWindow(output_area, OUTPUT_LIST_X, OUTPUT_LIST_Y,
		parent_rect.right, parent_rect.bottom - OUTPUT_LIST_Y, true);

	MoveWindow(input_field, KW_INPUT_X(x_offset), KW_INPUT_Y,
		KW_INPUT_W, KW_INPUT_H, true);

	MoveWindow(search_button, BTN_SEARCH_X(x_offset), BTN_SEARCH_Y,
		BTN_SEARCH_W, BTN_SEARCH_H, true);

	MoveWindow(add_button, BTN_ADD_X(x_offset), BTN_ADD_Y,
		BTN_ADD_W, BTN_ADD_H, true);

	MoveWindow(remove_button, BTN_REMOVE_X(x_offset), BTN_REMOVE_Y,
		BTN_REMOVE_W, BTN_REMOVE_H, true);

	MoveWindow(reset_button, BTN_RESET_X(x_offset), BTN_RESET_Y,
		BTN_RESET_W, BTN_RESET_H, true);

	MoveWindow(update_button, BTN_UPDATE_X(x_offset), BTN_UPDATE_Y,
		BTN_UPDATE_W, BTN_UPDATE_H, true);

	MoveWindow(cancel_button, BTN_CANCEL_X(x_offset), BTN_CANCEL_Y,
		BTN_CANCEL_W, BTN_CANCEL_H, true);

	MoveWindow(copy_button, BTN_COPY_X(x_offset), BTN_COPY_Y,
		BTN_COPY_W, BTN_COPY_H, true);

	MoveWindow(highlight_button, BTN_HIGHLIGHT_X(x_offset), BTN_HIGHLIGHT_Y,
		BTN_HIGHLIGHT_W, BTN_HIGHLIGHT_H, true);

	MoveWindow(keyword_listbox, KW_LISTBOX_X(x_offset), KW_LISTBOX_Y,
		KW_LISTBOX_W, KW_LISTBOX_H, true);

	MoveWindow(list_label, KW_LABEL_X(x_offset), KW_LABEL_Y,
		KW_LABEL_W, KW_LABEL_H, true);

	MoveWindow(result_label, RESULT_LABEL_X, RESULT_LABEL_Y,
		RESULT_LABEL_W, RESULT_LABEL_H, true);

	MoveWindow(case_sensitivity_checkbox, CHKBOX_CASE_X(x_offset),
		CHKBOX_CASE_Y, CHKBOX_CASE_W, CHKBOX_CASE_H, true);

	MoveWindow(whole_word_checkbox, CHKBOX_WHOLEWORD_X(x_offset),
		CHKBOX_WHOLEWORD_Y, CHKBOX_WHOLEWORD_W, CHKBOX_WHOLEWORD_H, true);

	MoveWindow(plaintext_checkbox, CHKBOX_REGEX_X(x_offset),
		CHKBOX_REGEX_Y, CHKBOX_REGEX_W, CHKBOX_REGEX_H, true);

	MoveWindow(next_button, NEXT_BTN_X(x_offset),
		NEXT_BTN_Y, NEXT_BTN_W, NEXT_BTN_H, true);

	MoveWindow(prev_button, PREV_BTN_X(x_offset),
		PREV_BTN_Y, PREV_BTN_W, PREV_BTN_H, true);

	InvalidateRect(main_window, NULL, true);
}

void SearchPlusUI::close_window()
{
	close_flag = true;

	for(int loop = 0; loop < MAX_TAB_CONTROLS; loop++){
		SetWindowLongPtr(tab_controls[loop].hwnd, GWLP_WNDPROC, (LONG_PTR)(tab_controls[loop].proc));
	}

	CloseWindow(main_window);
}

void SearchPlusUI::handle_window_close()
{
	stop_search();
	DestroyWindow(main_window);
	Ed_SetFocusOnEditor();
	main_window = NULL;
	CloseHandle(thread_handle);
	thread_handle = NULL;
}

void SearchPlusUI::stop_search()
{
	if(mode != UI_MODE_SEARCH){
		return;
	}

	stop_flag = true;
	Ed_StopSearch();
	exit_search();
}

void SearchPlusUI::send_search_over()
{
	PostMessage(main_window, SPM_SEARCH_OVER, 0, 0);
}

void SearchPlusUI::send_match_msg(int line_number, int line_length, SearchPattern *pat, int match_start, int match_length)
{
	match_msg_t *msg = new match_msg_t();

	msg->line_length = line_length;
	msg->line_number = line_number;
	msg->pattern = pat;
	msg->match_length = match_length;
	msg->match_start = match_start;

	PostMessage(main_window, SPM_MATCH_FOUND, WPARAM(msg), 0);
}

void SearchPlusUI::exit_search()
{
	TCHAR text[RESULT_LABEL_TEXT_LENGTH] = {0};
	int matched_line_count;
	MatchedLine *line = NULL;

	if(mode != UI_MODE_SEARCH)
		return;

	matched_line_count = SendMessage(output_area, LB_GETCOUNT, 0, 0);

	wsprintf(text, TEXT("%d matches in %d lines"), total_matches, matched_line_count);

	SendMessage(result_label, WM_SETTEXT, 0, LPARAM(text));

	for(int loop = 0; loop < matched_line_count; loop++){

		line = (MatchedLine*)SendMessage(output_area, LB_GETITEMDATA, loop, 0);
		line->FetchLine();
	}

	InvalidateRect(keyword_listbox, NULL, true);
	UpdateWindow(keyword_listbox);

	EnableWindow(reset_button, true);
	EnableWindow(add_button, true);
	EnableWindow(remove_button, true);
	EnableWindow(copy_button, true);
	EnableWindow(highlight_button, true);

	SendMessage(search_button, WM_SETTEXT, 0, (LPARAM)BTN_LBL_SEARCH);

	mode = UI_MODE_NORMAL;
}

void SearchPlusUI::handle_search_complete()
{
	if(stop_flag)
		return;

	exit_search();
}

void SearchPlusUI::handle_matching_line(match_msg_t *msg)
{
	MatchedLine *list_item = NULL;
	int item_count;
	RECT item_rect = {0, 0, 0, 0};
	TCHAR text[RESULT_LABEL_TEXT_LENGTH] = {0};

	if(!msg){
		DBG_MSG("NULL in <handle_matching_line>: Contact developer");
		return;
	}

	if(stop_flag){
		goto exit;
	}

	total_matches++;
	wsprintf(text, TEXT("%d matches.."), total_matches);
	SendMessage(result_label, WM_SETTEXT, 0, LPARAM(text));

	msg->pattern->count++;

	item_count = SendMessage(output_area, LB_GETCOUNT, 0, 0);

	/* reverse linear search better suits current use case */
	while(--item_count >= 0){

		list_item = (MatchedLine*)SendMessage(output_area, LB_GETITEMDATA, item_count, 0);

		if(list_item->line_number == msg->line_number){

			list_item->AddMatch(msg->match_start, msg->match_length, msg->pattern);
			SendMessage(output_area, LB_GETITEMRECT, item_count, LPARAM(&item_rect));
			InvalidateRect(output_area, &item_rect, true);
			goto exit;
		}

		if(list_item->line_number < msg->line_number)
			break;
	}

	total_strlen += msg->line_length;
	list_item = new MatchedLine(msg->line_number, msg->line_length,
		msg->match_start, msg->match_length, msg->pattern);

	SendMessage(output_area, LB_ADDSTRING, 0, (LPARAM)list_item);

	InvalidateRect(keyword_listbox, NULL, false);
	UpdateWindow(keyword_listbox);

exit:
	delete msg;
}

void SearchPlusUI::handle_add_button()
{
	TCHAR txt[SP_MAX_PATTERN_LENGTH] = {0};
	int index, length, count;
	SearchPattern *keyword;
	SearchPattern *parent = NULL;
	bool case_flag = false;
	bool regex_flag = false;
	bool word_flag = false;

	::GetWindowText(input_field, txt, SP_MAX_PATTERN_LENGTH);

	length = wcslen(txt);

	if(!length){
		return;
	}

	index = PAT_GetIndex(txt);

	if(index == -1){

		count = SendMessage(keyword_listbox, LB_GETCOUNT, 0, 0);

		if(count){
			parent = (SearchPattern*) SendMessage(keyword_listbox, LB_GETITEMDATA, count - 1, 0);
		}

		case_flag = BST_CHECKED == SendMessage(case_sensitivity_checkbox, BM_GETCHECK, 0, 0);
		regex_flag = BST_CHECKED != SendMessage(plaintext_checkbox, BM_GETCHECK, 0, 0);
		word_flag = BST_CHECKED == SendMessage(whole_word_checkbox, BM_GETCHECK, 0, 0);

		keyword = PAT_Add(txt, case_flag, word_flag, regex_flag);

		if(!keyword){
			DBG_MSG("Check the keyword for regex error");
			return;
		}

		SearchPattern *ui_kw = keyword->Clone();
		index = SendMessage(keyword_listbox, LB_ADDSTRING, 0, LPARAM(ui_kw));

		if(parent){
			parent->SetNext(ui_kw);
		}
	}


	SendMessage(keyword_listbox, LB_SETCARETINDEX, index, FALSE);
	SetWindowText(input_field, EMPTY_STRING);
	SetFocus(input_field);
}

void SearchPlusUI::handle_remove_button()
{
	int index = SendMessage(keyword_listbox, LB_GETCURSEL, 0, 0);
	SearchPattern *prev = NULL;
	SearchPattern *pattern = NULL;

	if(index == LB_ERR){
		return;
	}

	PAT_Delete(index);

	if(index != 0){

		pattern = (SearchPattern*) SendMessage(keyword_listbox, LB_GETITEMDATA, index, 0);
		prev = (SearchPattern*) SendMessage(keyword_listbox, LB_GETITEMDATA, index - 1, 0);

		prev->SetNext(pattern->GetNext());
		pattern->SetNext(NULL);
	}

	SendMessage(keyword_listbox, LB_DELETESTRING, index, 0);
}

void SearchPlusUI::handle_reset_button()
{
	PAT_DeleteAll();
	output_width = 0;
	SendMessage(keyword_listbox, LB_RESETCONTENT, 0, 0);
	SendMessage(output_area, LB_RESETCONTENT, 0, 0);
	SendMessage(result_label, WM_SETTEXT, 0, (LPARAM)EMPTY_STRING);
	EnableWindow(copy_button, false);
	EnableWindow(highlight_button, false);
}

void SearchPlusUI::handle_copy_button()
{
	int count = 0;
	MatchedLine *item;
	TCHAR * clipboard_string = NULL;
	int clipboard_len,  ret;
	HGLOBAL hgl;
	HANDLE hnd;
	TCHAR *buf_pos;

	count = SendMessage(output_area, LB_GETCOUNT, 0, 0);

	if(count <= 0){
		return;
	}

	/* allocate memory for newline chars as well */
	clipboard_len = total_strlen + count + 1;
	hgl = GlobalAlloc(GMEM_MOVEABLE, clipboard_len * sizeof(TCHAR));
	clipboard_string = (LPWSTR)GlobalLock(hgl);

	clipboard_string[0] = 0;
	buf_pos = clipboard_string;

	for(int index = 0; index < count; index++){

		item = (MatchedLine *)SendMessage(output_area, LB_GETITEMDATA, index, 0);

		if(!item->text){
			item->FetchLine();
		}

		memcpy(buf_pos, item->text, sizeof(TCHAR) * item->line_length);
		buf_pos += item->line_length;
	}

	*buf_pos = 0;

	ret = OpenClipboard(main_window);
	ret = EmptyClipboard();
	hnd = SetClipboardData(CF_UNICODETEXT, clipboard_string);
	ret = CloseClipboard();
}

void SearchPlusUI::handle_highlight_button()
{
	MatchedLine *lbitem = NULL;
	MatchedSegment *segment;
	int match_count = 0;

	ShowWindow(main_window, SW_HIDE);

	Ed_InitHighlightMatches();

	match_count = SendMessage(output_area, LB_GETCOUNT, 0, 0);

	for(int i = 0; i < match_count; i++){

		SendMessage(output_area, LB_GETTEXT, i, WPARAM(&lbitem));

		if(!lbitem)
			continue;

		segment = lbitem->match_list;

		while(segment){

			Ed_HighlightWord(lbitem->line_number, segment->from, segment->length, segment->pattern->GetStyle());
			segment = segment->next;
		}
	}

}

void SearchPlusUI::handle_search_button()
{
	TCHAR temp[NUM_LENGTH] = {0};
	int result = -1;
	SearchPattern *pattern = NULL;
	int count = 0;

	if(mode == UI_MODE_SEARCH){

		/* search button moonlights as stop button when search is in progress */
		stop_search();
		return;
	}

	if(mode != UI_MODE_NORMAL){
		return;
	}

	linecount = Ed_GetLineCount();

	if(linecount > MAX_LINES_SUPPORTED){
		DBG_MSG("File is too big: split it and try again");
		return;
	}

	_itow_s(linecount, temp, 10, 10);
	linecount_strlen = wcslen(temp);

	total_strlen = 0;

	SendMessage(output_area, LB_RESETCONTENT, 0, 0);

	count = SendMessage(keyword_listbox, LB_GETCOUNT, 0, 0);

	if(count == 0){
		DBG_MSG("Add patterns");
		return;
	}

	for(int loop = 0; loop < count; loop++){
		pattern = (SearchPattern*) SendMessage(keyword_listbox, LB_GETITEMDATA, loop, 0);
		pattern->count = 0;
	}

	InvalidateRect(keyword_listbox, NULL, true);
	UpdateWindow(keyword_listbox);

	pattern = (SearchPattern*) SendMessage(keyword_listbox, LB_GETITEMDATA, 0, 0);

	stop_flag = false;
	total_matches = 0;

	result = Ed_Search(pattern);

	if(result == 0){
		SendMessage(result_label, WM_SETTEXT, 0, (LPARAM)TEXT("Searching.."));
		EnableWindow(reset_button, false);
		EnableWindow(add_button, false);
		EnableWindow(remove_button, false);
		SendMessage(search_button, WM_SETTEXT, 0, (LPARAM)BTN_LBL_STOP);
		output_width = 0;
		mode = UI_MODE_SEARCH;
	}
	else{
		SendMessage(result_label, WM_SETTEXT, 0, (LPARAM)EMPTY_STRING);
	}
}

void SearchPlusUI::handle_escape_key()
{
	if(mode == UI_MODE_EDIT_KW){
		exit_edit();
	}
	else{
		ShowWindow(main_window, SW_HIDE);
		Ed_SetFocusOnEditor();
	}
}

void SearchPlusUI::load_keywords(SearchPattern *pat_list)
{
	SearchPattern *temp_pat = pat_list;
	SearchPattern *ui_kw = NULL;
	SearchPattern *parent = NULL;

	while(temp_pat){

		ui_kw = temp_pat->Clone();
		::SendMessage(keyword_listbox, LB_ADDSTRING, 0, LPARAM(ui_kw));


		if(parent){
			parent->SetNext(ui_kw);
		}
		parent = ui_kw;

		temp_pat = temp_pat->GetNext();
	}
}

void SearchPlusUI::draw_keyword(LPDRAWITEMSTRUCT pDis)
{
	SearchPattern *uipat = (SearchPattern*)(pDis->itemData);
	TCHAR temp[SP_MAX_PATTERN_LENGTH + NUM_LENGTH] = {0};
	RECT text_rect = pDis->rcItem;
	int length;

	if(!uipat){
		return;
	}

	SetBkMode(pDis->hDC, TRANSPARENT);

	if(pDis->itemAction == ODA_SELECT || (pDis->itemState | ODS_SELECTED)){

		HBRUSH brush = CreateSolidBrush((pDis->itemState & ODS_SELECTED) ? COLOR_SEL_BKG : COLOR_WHITE);
		FillRect(pDis->hDC, &pDis->rcItem, brush);
		DeleteObject(brush);
	}

	length = wcslen(uipat->GetText());

	if(uipat->count){
		wsprintf(temp, TEXT("%s (%d)"), uipat->GetText(), uipat->count);
	}
	else{
		wcscpy_s(temp, SP_MAX_PATTERN_LENGTH + NUM_LENGTH, uipat->GetText());
	}

	length = wcslen(temp);

	draw_text(pDis->hDC, temp, length, &text_rect, Ed_GetColorFromStyle(uipat->GetStyle()));

	if(pDis->itemAction == ODA_FOCUS){

		SetTextColor(pDis->hDC, RGB(30, 30, 30));
		text_rect = pDis->rcItem;
		DrawFocusRect(pDis->hDC, &text_rect);
	}

}

void SearchPlusUI::draw_matched_string(LPDRAWITEMSTRUCT pDis)
{
	TCHAR num_w[NUM_LENGTH] = {0};
	TCHAR temp[NUM_LENGTH] = {0};
	MatchedLine *lbitem = (MatchedLine*)(pDis->itemData);
	RECT text_rect = pDis->rcItem;
	int i = 0, j = 0;
	MatchedSegment *matchlist = NULL;

	if(!lbitem){
		return;
	}

	SetBkMode(pDis->hDC, TRANSPARENT);

	if((pDis->itemAction == ODA_SELECT) || (pDis->itemState | ODS_SELECTED)){

		HBRUSH brush = CreateSolidBrush((pDis->itemState & ODS_SELECTED) ? COLOR_SEL_BKG : COLOR_WHITE);
		FillRect(pDis->hDC, &pDis->rcItem, brush);
		DeleteObject(brush);
	}

	_itow_s(lbitem->line_number, temp, NUM_LENGTH - 4, 10);
	j = wcslen(temp) - 1;

	for(i = this->linecount_strlen - 1; i >= 0; i--){

		if(j >= 0){
			num_w[i] = temp[j--];
		}
		else{
			num_w[i] = TEXT('_');
		}
	}

	wcscat_s(num_w, NUM_LENGTH, TEXT(" : "));

	draw_text(pDis->hDC, num_w, linecount_strlen + 3, &text_rect, COLOR_LINE_NUM);

	lbitem->Draw(pDis->hDC, &text_rect);

	DrawText(pDis->hDC, lbitem->text, lbitem->line_length, &text_rect, DT_FLAGS | DT_CALCRECT);

	if(output_width < text_rect.right){

		output_width = text_rect.right;
		SendMessage(output_area, LB_SETHORIZONTALEXTENT, output_width, 0);
	}
}

void SearchPlusUI::goto_selected_line()
{
	MatchedLine *lbitem = NULL;
	int index;

	index = SendMessage(output_area, LB_GETCURSEL, 0, 0);
	SendMessage(output_area, LB_GETTEXT, index, WPARAM(&lbitem));

	ShowWindow(main_window, SW_HIDE);

	Ed_GotoMatch(lbitem->line_number, lbitem->match_list->from,
		lbitem->match_list->length);
}

WNDPROC SearchPlusUI::get_wnd_proc(HWND hwnd)
{
	for(int loop = 0; loop < MAX_TAB_CONTROLS; loop++){

		if(tab_controls[loop].hwnd == hwnd){
			return tab_controls[loop].proc;
		}
	}

	return NULL;
}

void SearchPlusUI::handle_tab_order(HWND cur_wnd, HWND *windows, int count, bool forward)
{
	int index;
	HWND focused_window = NULL;

	for(index = 0; (index < count) && (windows[index] != cur_wnd); index++);

	if(index < TAB_COUNT_NORMAL){

		focused_window = forward ? windows[++index % count] : windows[(index + count - 1) % count] ;
		SetFocus(focused_window);
	}
}

int SearchPlusUI::handle_keys_tabbed_control(HWND hwnd, WPARAM wParam, LPARAM lParam)
{
	HWND normal_mode_controls[TAB_COUNT_NORMAL] = {input_field, reset_button, remove_button, add_button, search_button, keyword_listbox};
	HWND edit_mode_controls[TAB_COUNT_EDIT] = {input_field, cancel_button, update_button, keyword_listbox};
	int retval = 0;
	unsigned short shift = GetKeyState(VK_SHIFT);

	if(wParam == VK_ESCAPE){
		handle_escape_key();
		return 0;
	}

	switch(mode){

	case UI_MODE_NORMAL:
		if(wParam == VK_RETURN){

			if(hwnd == input_field || hwnd == add_button){
				handle_add_button();
			}
			else if(hwnd == search_button){
				handle_search_button();
			}
			else if(hwnd == remove_button){
				handle_remove_button();
			}
			else if(hwnd == reset_button){
				handle_reset_button();
			}
		}
		else if(wParam == VK_TAB){
			handle_tab_order(hwnd, normal_mode_controls, TAB_COUNT_NORMAL, !(shift & 0x80));
		}
		else if(wParam == VK_DELETE && hwnd == keyword_listbox){
			ui_data->handle_remove_button();
		}
		else{
			retval = 1;
		}
		break;

	case UI_MODE_EDIT_KW:
		if(wParam == VK_RETURN){

			if(hwnd == input_field || hwnd == update_button){
				ui_data->handle_update_button();
			}
			else if(hwnd == cancel_button){
				ui_data->handle_cancel_button();
			}
		}
		else if(wParam == VK_TAB){
			handle_tab_order(hwnd, edit_mode_controls, TAB_COUNT_EDIT, !(shift & 0x80));
		}
		break;
	}

	return retval;
}

void SearchPlusUI::handle_window_activate(bool focus)
{
	if(focus){
		SetLayeredWindowAttributes(ui_data->main_window, 0, ALPHA_OPAQUE, LWA_ALPHA);
	}
	else{
		SetLayeredWindowAttributes(ui_data->main_window, 0, ALPHA_SEE_THRU, LWA_ALPHA);
	}
}

SearchPattern *SearchPlusUI::get_selected_keyword()
{
	int index = LB_ERR;
	SearchPattern *kw = NULL;

	index = SendMessage(keyword_listbox, LB_GETCURSEL, 0, 0);

	if(index == LB_ERR){
		return NULL;
	}

	SendMessage(keyword_listbox, LB_GETTEXT, index, WPARAM(&kw));

	return kw;
}

void SearchPlusUI::start_keyword_edit()
{
	SearchPattern *kw = NULL;

	if(mode != UI_MODE_NORMAL){
		return;
	}

	kw = get_selected_keyword();

	if(!kw){
		return;
	}

	highlight_keyword(kw);

	mode = UI_MODE_EDIT_KW;

	EnableWindow(case_sensitivity_checkbox, true);
	EnableWindow(whole_word_checkbox, true);
	EnableWindow(plaintext_checkbox, true);

	ShowWindow(reset_button, SW_HIDE);
	ShowWindow(add_button, SW_HIDE);
	ShowWindow(remove_button, SW_HIDE);
	ShowWindow(search_button, SW_HIDE);

	ShowWindow(update_button, SW_SHOW);
	ShowWindow(cancel_button, SW_SHOW);

	SendMessage(list_label, WM_SETTEXT, 0, LPARAM(LIST_LBL_EDIT));
}

void SearchPlusUI::reset_highlights()
{
	SendMessage(case_sensitivity_checkbox, BM_SETCHECK, BST_UNCHECKED, 0);
	SendMessage(whole_word_checkbox, BM_SETCHECK, BST_UNCHECKED, 0);
	SendMessage(plaintext_checkbox, BM_SETCHECK, BST_UNCHECKED, 0);
}

void SearchPlusUI::highlight_keyword(SearchPattern *kw)
{
	int check_status;

	if(!kw){
		return;
	}

	check_status = kw->GetCaseSensitivity() ? BST_CHECKED : BST_UNCHECKED;
	SendMessage(case_sensitivity_checkbox, BM_SETCHECK, check_status, 0);

	check_status = kw->GetWholeWordStatus() ? BST_CHECKED : BST_UNCHECKED;
	SendMessage(whole_word_checkbox, BM_SETCHECK, check_status, 0);

	check_status = kw->GetRegexStatus() ? BST_UNCHECKED : BST_CHECKED;
	SendMessage(plaintext_checkbox, BM_SETCHECK, check_status, 0);

	SendMessage(input_field, WM_SETTEXT, 0, LPARAM(kw->GetText()));
}

void SearchPlusUI::handle_cancel_button()
{
	exit_edit();
}

void SearchPlusUI::handle_update_button()
{
	TCHAR txt[SP_MAX_PATTERN_LENGTH] = {0};
	int index, length;
	SearchPattern *keyword;
	bool case_flag = false;
	bool regex_flag = false;
	bool word_flag = false;

	::GetWindowText(input_field, txt, SP_MAX_PATTERN_LENGTH);

	length = wcslen(txt);

	if(!length){
		return;
	}

	index = SendMessage(keyword_listbox, LB_GETCURSEL, 0, 0);

	if(index != -1){

		case_flag = BST_CHECKED == SendMessage(case_sensitivity_checkbox, BM_GETCHECK, 0, 0);
		regex_flag = BST_CHECKED != SendMessage(plaintext_checkbox, BM_GETCHECK, 0, 0);
		word_flag = BST_CHECKED == SendMessage(whole_word_checkbox, BM_GETCHECK, 0, 0);

		keyword = PAT_Update(index, txt, case_flag, word_flag, regex_flag);

		if(!keyword){
			DBG_MSG("Check the keyword for regex error");
			return;
		}

		SearchPattern *ui_kw = (SearchPattern*)SendMessage(keyword_listbox, LB_GETITEMDATA, index, 0);

		if(ui_kw){
			ui_kw->Update(txt, case_flag, word_flag, regex_flag);
			InvalidateRect(keyword_listbox, NULL, true);
		}
	}

	exit_edit();

	SetWindowText(input_field, EMPTY_STRING);
	SetFocus(input_field);
}

void SearchPlusUI::exit_edit()
{
	if(mode != UI_MODE_EDIT_KW)
		return;

	ShowWindow(reset_button, SW_SHOW);
	ShowWindow(add_button, SW_SHOW);
	ShowWindow(remove_button, SW_SHOW);
	ShowWindow(search_button, SW_SHOW);

	ShowWindow(update_button, SW_HIDE);
	ShowWindow(cancel_button, SW_HIDE);

	reset_highlights();

	SendMessage(input_field, WM_SETTEXT, 0, LPARAM(""));

	SendMessage(list_label, WM_SETTEXT, 0, LPARAM(LIST_LBL_NORMAL));

	mode = UI_MODE_NORMAL;
}

void SearchPlusUI::goto_match(MatchIterType type)
{
	bool found = false;
	int retval, counter = 0;
	MatchedLine *line;
	MatchedSegment *segment;
	SearchPattern *kw = NULL;
	int current_index = -1;

	kw = get_selected_keyword();

	if(!kw)
		return;

	if(kw->count == 0)
		return;

	if(type == FIRST){
		current_index = -1;
	}
	else if(type == LAST){
		current_index = total_matches;
	}
	else{
		current_index = SendMessage(output_area, LB_GETCURSEL, 0, 0);
	}

	while(!found){

		if(counter++ > total_matches)
			break;

		current_index += (type == NEXT || type == FIRST) ? 1 : -1;

		if(current_index == total_matches)
			current_index = 0;

		if(current_index < 0)
			current_index = total_matches - 1;

		retval = SendMessage(output_area, LB_GETITEMDATA, current_index, 0);

		if(retval == LB_ERR)
			break;

		line = (MatchedLine*)retval;

		segment = line->match_list;

		while(segment){

			if(segment->pattern == kw){
				SendMessage(output_area, LB_SETCURSEL, current_index, 0);
				found = true;
				break;
			}

			segment = segment->next;
		}	
	}
}

void SearchPlusUI::handle_keyword_selection()
{
	SearchPattern *kw = NULL;
	bool found = false;
	unsigned short ctrl = GetKeyState(VK_CONTROL);

	if((kw = get_selected_keyword()) == NULL){
		return;
	}

	if(mode == UI_MODE_EDIT_KW){
		exit_edit();
	}

	if(ctrl){
		start_keyword_edit();
		return;
	}

	highlight_keyword(kw);

	EnableWindow(case_sensitivity_checkbox, false);
	EnableWindow(whole_word_checkbox, false);
	EnableWindow(plaintext_checkbox, false);
}

void SearchPlusUI::handle_keywordlb_focuschange()
{
	if(mode == UI_MODE_EDIT_KW)
		return;

	reset_highlights();

	EnableWindow(case_sensitivity_checkbox, true);
	EnableWindow(whole_word_checkbox, true);
	EnableWindow(plaintext_checkbox, true);
}

bool SearchPlusUI::get_minmax_size(MINMAXINFO *minmax)
{
	if(!minmax)
		return false;

	if(minmax->ptMinTrackSize.x < SP_UIPANEL_MIN_W){
		minmax->ptMinTrackSize.x = SP_UIPANEL_MIN_W;
		minmax->ptMinTrackSize.y = (minmax->ptMaxSize.y * 50) / 100;
		return true;
	}

	return false;
}

HWND SearchPlusUI::assign_tooltip(HWND button, HWND parent, TCHAR *text)
{
	TOOLINFO toolinfo = {0};
	HWND tooltip;

	tooltip = CreateWindowEx(WS_EX_TOPMOST, TOOLTIPS_CLASS,NULL,
		WS_POPUP | TTS_ALWAYSTIP | TTS_NOFADE | TTS_NOANIMATE,
		CW_USEDEFAULT, CW_USEDEFAULT,
		CW_USEDEFAULT, CW_USEDEFAULT,
		parent, NULL, NULL, 0);

	if(!tooltip)
		return NULL;

	toolinfo.cbSize = sizeof(toolinfo);
	toolinfo.hwnd = parent;
	toolinfo.uFlags = TTF_IDISHWND | TTF_SUBCLASS;

	toolinfo.uId = (UINT_PTR)button;
	toolinfo.lpszText = text;
	SendMessage(tooltip, TTM_ADDTOOL, 0, LPARAM(&toolinfo));

	return tooltip;
}

void UI_Initialize(HWND parent_handle)
{
	ui_data = new SearchPlusUI(parent_handle);
}

void UI_ShowPluginWindow()
{
	ui_data->LaunchSearchPlusWindow();
}

void UI_ShowAboutWindow()
{
	ui_data->LaunchAboutWindow();
}

void UI_HandleMatchingLine(int line_number, int line_length, SearchPattern *pat, int match_start, int match_length)
{
	ui_data->send_match_msg(line_number, line_length, pat, match_start, match_length);
}

void UI_HandleSearchComplete(int count, SearchPattern *patlist)
{
	ui_data->send_search_over();
}

void UI_Terminate()
{
	ui_data->Terminate();
}

LRESULT CALLBACK WndProc(HWND hWnd, UINT iMessage, WPARAM wParam, LPARAM lParam)
{
	if(!ui_data){
		return DefWindowProc(hWnd, iMessage, wParam, lParam);
	}

	switch (iMessage) {

	case WM_CREATE:
		ui_data->handle_window_create(hWnd);
		break;

	case WM_COMMAND:

		if(HIWORD(wParam) == LBN_DBLCLK && LOWORD(wParam) == SP_InstanceOutputArea){
			ui_data->goto_selected_line();
		}
		else if(HIWORD(wParam) == LBN_DBLCLK && LOWORD(wParam) == SP_InstancePatternList){
			ui_data->start_keyword_edit();
		}
		else if(HIWORD(wParam) == LBN_SELCHANGE && LOWORD(wParam) == SP_InstancePatternList){
			ui_data->handle_keyword_selection();
		}
		else if(HIWORD(wParam) == LBN_KILLFOCUS && LOWORD(wParam) == SP_InstancePatternList){
				ui_data->handle_keywordlb_focuschange();
		}
		else if(HIWORD(wParam) == BN_CLICKED){

			switch(LOWORD(wParam)){

			case SP_InstanceAddButton:
				ui_data->handle_add_button();
				break;

			case SP_InstanceRemoveButton:
				ui_data->handle_remove_button();
				break;

			case SP_InstanceClearButton:
				ui_data->handle_reset_button();
				break;

			case SP_InstanceSearchButton:
				ui_data->handle_search_button();
				break;

			case SP_InstanceClipboardButton:
				ui_data->handle_copy_button();
				break;

			case SP_InstanceHighlightButton:
				ui_data->handle_highlight_button();
				break;

			case SP_InstanceUpdateButton:
				ui_data->handle_update_button();
				break;

			case SP_InstanceCancelButton:
				ui_data->handle_cancel_button();
				break;

			case SP_InstanceNextButton:
				ui_data->goto_match(NEXT);
				break;

			case SP_InstancePrevButton:
				ui_data->goto_match(PREV);
				break;
			}
		}
		break;

	case WM_VKEYTOITEM:
		if(LOWORD(wParam) == VK_ESCAPE){
			ui_data->handle_escape_key();
			return -2;
		}
		break;

	case WM_KEYDOWN:
		if(wParam == VK_ESCAPE){
			ui_data->handle_escape_key();
			return 0;
		}
		break;

	case WM_CLOSE:
		ui_data->handle_window_close();
		break;

	case WM_GETMINMAXINFO:
		if(ui_data->get_minmax_size((MINMAXINFO*)lParam)){
			return 0;
		}
		break;

	case WM_SIZE:
		ui_data->resize_controls();
		break;

	case WM_DRAWITEM:
		if (((LPDRAWITEMSTRUCT)lParam)->CtlID == SP_InstanceOutputArea) {
			ui_data->draw_matched_string((LPDRAWITEMSTRUCT)lParam);
		}
		else if(((LPDRAWITEMSTRUCT)lParam)->CtlID == SP_InstancePatternList){
			ui_data->draw_keyword((LPDRAWITEMSTRUCT)lParam);
		}
		return true;

	case WM_DELETEITEM:
		if(wParam == SP_InstanceOutputArea){
			MatchedLine *item = (MatchedLine *)(((DELETEITEMSTRUCT *)lParam)->itemData);
			delete item;
		}
		else if(wParam == SP_InstancePatternList){
			SearchPattern *uipat = (SearchPattern*)(((DELETEITEMSTRUCT *)lParam)->itemData);
			delete uipat;
		}
		break;

	case WM_ACTIVATE:
		ui_data->handle_window_activate(wParam != WA_INACTIVE);
		break;

	case WM_DESTROY:
		PostQuitMessage(0);
		break;

	case SPM_MATCH_FOUND:
		ui_data->handle_matching_line((match_msg_t*)wParam);
		break;

	case SPM_SEARCH_OVER:
		ui_data->handle_search_complete();
		break;

	case SPM_TERMINATE:
		ui_data->stop_search();
		delete ui_data;
		ui_data = NULL;
		break;

	default:
		break;
	}

	return DefWindowProc(hWnd, iMessage, wParam, lParam);
}

LRESULT CALLBACK TabbedControlProc(HWND hWnd, UINT iMessage, WPARAM wParam, LPARAM lParam)
{
	WNDPROC proc = ui_data->get_wnd_proc(hWnd);
	LRESULT retval = 1;

	if(iMessage == WM_KEYDOWN){
		retval = ui_data->handle_keys_tabbed_control(hWnd, wParam, lParam);
	}

	if(iMessage == WM_CHAR && (wParam == VK_RETURN || wParam == VK_TAB || wParam == VK_ESCAPE)){
		 return 0;
	}

	if(retval){
		return CallWindowProc(proc, hWnd, iMessage, wParam, lParam);
	}

	return 0;
}

DWORD WINAPI UI_Thread(LPVOID param)
{
	SearchPlusUI *ui_data = (SearchPlusUI*)param;
	MSG msg;

	ui_data->CreatePluginWindow();
	ui_data->LoadCurrentWord();

	while(GetMessage(&msg, NULL, 0, 0) > 0){

		TranslateMessage(&msg);
		DispatchMessage(&msg);
	}

	return 0;
}

int SearchPlusUI::LaunchAboutWindow()
{
	create_about_window();
	return 0;
}

LRESULT CALLBACK AboutWndProc(HWND hWnd, UINT iMessage, WPARAM wParam, LPARAM lParam)
{
	if(!ui_data){
		return DefWindowProc(hWnd, iMessage, wParam, lParam);
	}

	switch (iMessage) {

	case WM_NOTIFY:
		if(LOWORD(wParam) == SP_InstanceUpdateLink)	{

			PNMLINK nlink = (PNMLINK)lParam;

			switch(nlink->hdr.code){

			case NM_CLICK:
				ShellExecute(NULL, TEXT("open"), nlink->item.szUrl, NULL, NULL, SW_SHOWDEFAULT);
				break;
			}
		}
	break;

	case WM_KEYDOWN:
		if(wParam == VK_ESCAPE){
			CloseWindow(ui_data->about_window);
			DestroyWindow(ui_data->about_window);
			ui_data->about_window = NULL;
		}
		break;

	case WM_CREATE:
		break;

	case WM_CLOSE:
		ui_data->about_window = NULL;
		break;
	}

	return DefWindowProc(hWnd, iMessage, wParam, lParam);
}

bool SearchPlusUI::create_about_window()
{
	RECT rect = {0, 0, 0, 0};
	RECT clientrect = {0, 0, 0, 0};
	HWND desktop;
	int x, y, link_height = 0;
	HDC linkhdc;
	SIZE size = {0, 0};

	if(about_window)
		return false;

	desktop = GetDesktopWindow();

	GetClientRect(desktop, &rect);

	x = (rect.right - ABOUT_WND_WIDTH) / 2;
	y = (rect.bottom - ABOUT_WND_HEIGHT) / 2;

	about_window = ::CreateWindowEx(
		WS_EX_TOOLWINDOW,
		about_wnd_classname,
		TEXT("About Search+"),
		WS_SYSMENU | WS_SIZEBOX | WS_TABSTOP | WS_GROUP | WS_MAXIMIZEBOX | WS_MINIMIZEBOX | WS_CLIPCHILDREN,
		x, y, ABOUT_WND_WIDTH, ABOUT_WND_HEIGHT,
		this->editor_handle, NULL,
		(HINSTANCE)SP_InstanceAboutWindowClass,
		0);

	update_link = CreateWindowEx(0,
		WC_LINK,
		SP_VERSION_STR TEXT("\n\n")
			TEXT("© <a href=\"http://amarghosh.blogspot.com/2012/08/searchplus.html\">amarghosh</a>\n\n")
			TEXT("Check for the <a href=\"http://code.google.com/p/searchplus/downloads/list\">")
			TEXT("latest version</a>"),
		WS_VISIBLE | WS_CHILD | WS_TABSTOP | SS_NOTIFY,
		CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT,
		this->about_window, (HMENU)SP_InstanceUpdateLink,
		(HINSTANCE)SP_InstanceUpdateLink,
		0);

	SendMessage(update_link, WM_SETFONT, WPARAM(font), 0);

	GetClientRect(about_window, &clientrect);

	linkhdc = GetWindowDC(update_link);

	x = (ABOUT_WND_WIDTH - ABT_LINK_WIDTH) / 2;
	y = ABT_WND_VERT_PADDING;
	link_height = SendMessage(update_link, LM_GETIDEALSIZE, ABT_LINK_WIDTH, LPARAM(&size));

	MoveWindow(update_link, x, y, ABT_LINK_WIDTH, link_height, false);

	ShowWindow(about_window, SW_SHOW);
	SetFocus(about_window);

	return true;
}

