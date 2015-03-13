/*
Search+ Plugin 
amarghosh @ gmail dot com
*/


#include <Windows.h>
#include <CommCtrl.h>

#include "searchplus.h"

#define MAX_LINES_SUPPORTED (10000000)

#define EMPTY_STRING TEXT("")

/* strings used in UI */
#define BTN_LBL_ADD TEXT("Add")
#define BTN_LBL_DELETE TEXT("Delete")
#define BTN_LBL_RESET TEXT("Reset")
#define BTN_LBL_SEARCH TEXT("Search")
#define BTN_LBL_COPY TEXT("Ctrl+C")
#define BTN_LBL_HIGHLIGHT TEXT("Highlight")
#define BTN_LBL_UPDATE TEXT("Update")
#define BTN_LBL_CANCEL TEXT("Cancel")

#define CHK_BOX_LABEL_CASE TEXT("Case Sensitive")
#define CHK_BOX_LABEL_WORD TEXT("Whole Words only")
#define CHK_BOX_LABEL_REGEX TEXT("Use plain text")

#define LIST_LBL_NORMAL TEXT("Add keywords to the list and click Search")
#define LIST_LBL_EDIT TEXT("Edit the text and flags and click Update")

/*
This should be big enough to hold itoa(MAX_LINES_SUPPORTED) + " : "
*/
#define NUM_LENGTH (15)

#define COLOR_NORMAL RGB(0, 0, 0)
#define COLOR_LINE_NUM RGB(128, 128, 128)
#define COLOR_SEL_BKG RGB(192, 192, 192)
#define COLOR_WHITE RGB(255, 255, 255)

/* DrawText flags */
#define DT_FLAGS (DT_EXPANDTABS | DT_NOCLIP | DT_SINGLELINE)

/* Window transparency */
#define ALPHA_OPAQUE (255)
#define ALPHA_SEE_THRU (192)

/* macros for positioning various controls */

#define SP_WINDOW_HEIGHT (500)

#define SP_UIPANEL_MARGIN_H (20)
#define SP_PANEL_WIDTH (((PATTERN_LISTBOX_WIDTH + SP_UIPANEL_MARGIN_H + SP_PADDING_HORI) * 2) + SP_CHECKBOX_WIDTH)

#define RESULT_LBL_TEXT_LENGTH (30)
#define SP_MARGIN_HORIZONTAL (5)
#define SP_PADDING_HORI (3)
#define SP_Y_OFFSET (5)

#define SP_INPUTLABEL_WIDTH 150
#define SP_LISTLABEL_WIDTH 260
#define SP_RESLABEL_WIDTH 200

#define SP_CHECKBOX_WIDTH (150)
#define SP_CHECKBOX_HEIGHT (20)

#define SP_ROW_HEIGHT (22)
#define SP_LBL_HEIGHT (15)

#define SP_LIST_HEIGHT (100)

#define SP_LIST_Y SP_Y_OFFSET

#define SP_TA_X (0)
#define SP_TA_Y (SP_LIST_HEIGHT + (2 * SP_Y_OFFSET))

#define SP_INPUT_Y (SP_Y_OFFSET + SP_LBL_HEIGHT)

#define SP_BTN_Y (SP_ROW_HEIGHT + (SP_Y_OFFSET) + SP_INPUT_Y)
#define SP_BTN_WIDTH (70)
#define SP_ADD_BTN_WIDTH (100)

#define SP_COPY_BTN_WIDTH 80

#define COPY_BTN_X(width) width / 2 + SP_PADDING_HORI * 2 + PATTERN_LISTBOX_WIDTH

#define HIGHLIGHT_BTN_WIDTH 80
#define HIGHLIGHT_BTN_X(width) COPY_BTN_X(width) - HIGHLIGHT_BTN_WIDTH - (SP_PADDING_HORI / 2)

#define PATTERN_LISTBOX_WIDTH (300)

static const TCHAR className[] = TEXT("SP_Window");

static LRESULT CALLBACK WndProc(HWND hWnd, UINT iMessage, WPARAM wParam, LPARAM lParam);

/* 
window procedures to handle tabbed navigation, simulate button click on enter key 
and to hide window on hitting escape.
*/
static LRESULT CALLBACK TabbedControlProc(HWND hWnd, UINT iMessage, WPARAM wParam, LPARAM lParam);

struct keyword_t{

	TCHAR *string;
	int count;
	COLORREF color;
	bool case_flag;
	bool word_flag;
	bool regex_flag;
	
	keyword_t(SearchPattern *keyword);

	~keyword_t(){
		if(string)
			delete string;
	}

	void update(SearchPattern *keyword);
};

/* Don't give me NULL */
keyword_t::keyword_t(SearchPattern *keyword)
{
	TCHAR *text = keyword->getText();
	int length = wcslen(text);

	string = new TCHAR[length + 1];
	wcscpy_s(string, length + 1, text);

	this->color = Ed_GetColorFromStyle(keyword->getStyle());
	this->case_flag = keyword->getCaseSensitivity();
	this->word_flag = keyword->getWholeWordStatus();
	this->regex_flag = keyword->getRegexStatus();

	count = 0;
}

void keyword_t::update(SearchPattern *keyword)
{
	TCHAR *text = keyword->getText();
	int length = wcslen(text);

	if(string){
		delete string;
	}

	string = new TCHAR[length + 1];
	wcscpy_s(string, length + 1, text);

	this->color = Ed_GetColorFromStyle(keyword->getStyle());
	this->case_flag = keyword->getCaseSensitivity();
	this->word_flag = keyword->getWholeWordStatus();
	this->regex_flag = keyword->getRegexStatus();

	count = 0;
}


struct matched_segment_t{
	int from;
	int length;
	int style;
	matched_segment_t *next;

	matched_segment_t(int match_start, int match_length, int style);
	~matched_segment_t();
};

/* data item for the output listbox */
struct matched_line_t{
	TCHAR *text;
	int line_length;
	int line_number;
	matched_segment_t *match_list;

	matched_line_t(TCHAR *matched_line, int total_length, int line_number, int match_from, int match_length, int style);
	~matched_line_t();

	void add_match(int match_from, int match_length, int style);
	void draw_item(HDC hdc, LPRECT full_rect);
};

matched_segment_t::matched_segment_t(int match_start, int match_length, int style)
{
	from = match_start;
	length = match_length;
	this->style = style;
	next = NULL;
}

matched_segment_t::~matched_segment_t()
{
	if(next){
		delete next;
	}
}

matched_line_t::matched_line_t(TCHAR *matched_line, int total_length, int line_number, int match_from, int match_length, int style)
{
	this->line_number = line_number;
	this->line_length = total_length;

	this->text = new TCHAR[total_length + 1];
	wcscpy_s(text, total_length + 1, matched_line);

	this->match_list = new matched_segment_t(match_from, match_length, style);
}

matched_line_t::~matched_line_t()
{
	if(text)
		delete text;

	delete match_list;
}

void matched_line_t::add_match(int match_from, int match_length, int style)
{
	matched_segment_t *last_match = this->match_list;

	if(!last_match){
		this->match_list = new matched_segment_t(match_from, match_length, style);
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

		last_match->next = new matched_segment_t(match_from, match_length, style);
	}
}

static void draw_text(HDC hdc, TCHAR *text, int length, LPRECT rect, COLORREF textcolor)
{
	int rightend = rect->right;

	SetTextColor(hdc, textcolor);
	DrawText(hdc, text, length, rect, DT_FLAGS | DT_CALCRECT);
	DrawText(hdc, text, length, rect, DT_FLAGS);
	rect->left = rect->right;
	rect->right = rightend;
}

void matched_line_t::draw_item(HDC hdc, LPRECT full_rect)
{
	int cur_pos = 0;
	int segment_length = 0;
	RECT segment_rect = *full_rect;
	matched_segment_t *matchlist = this->match_list;
	TCHAR *text_segment;
	
	text_segment = new TCHAR[this->line_length + 1];

	while(matchlist){

		if(cur_pos < matchlist->from){
			
			segment_length = matchlist->from - cur_pos;
			wcsncpy_s(text_segment, line_length + 1, text + cur_pos, segment_length);
			text_segment[segment_length] = 0;

			draw_text(hdc, text_segment, segment_length, &segment_rect, COLOR_NORMAL);

			cur_pos += segment_length;
		}

		wcsncpy_s(text_segment, line_length + 1, text + cur_pos, matchlist->length);
		text_segment[matchlist->length] = 0;

		draw_text(hdc, text_segment, matchlist->length, &segment_rect, Ed_GetColorFromStyle(matchlist->style));

		cur_pos += matchlist->length;

		matchlist = matchlist->next;
	}

	segment_length = this->line_length - cur_pos;
	wcsncpy_s(text_segment, line_length + 1, text + cur_pos, segment_length);
	draw_text(hdc, text_segment, segment_length, &segment_rect, COLOR_NORMAL);

	delete text_segment;
}

enum{
	SP_InstanceNone,
	SP_InstanceWndClass,
	SP_InstanceMainWindow,
	SP_InstanceInputField,
	SP_InstanceAddButton,
	SP_InstancePatternList,
	SP_InstanceOutputArea,
	SP_InstanceRemoveButton,
	SP_InstanceClearButton,
	SP_InstanceSearchButton,
	SP_InstanceClipboardButton,
	SP_InstanceHighlightButton,
	SP_InstanceUpdateButton,
	SP_InstanceCancelButton,
	SP_InstanceCaseSensitivityCB,
	SP_InstanceMatchWordCB,
	SP_InstanceRegexCB
};

typedef enum {
	UI_MODE_NORMAL,
	UI_MODE_SEARCH,
	UI_MODE_EDIT_KW
}SPUI_mode_t;

class SearchPlusUI{
	bool init_flag;
	bool close_flag;

	HWND editor_handle;

	HWND main_window;

	HWND list_label;
	HWND result_label;
	HWND input_field;
	HWND add_button;
	HWND keyword_listbox;
	HWND output_area;
	HWND remove_button;
	HWND reset_button;
	HWND search_button;
	HWND copy_button;
	HWND highlight_button;
	HWND update_button;
	HWND cancel_button;

	HWND case_sensitivity_checkbox;
	HWND whole_word_checkbox;
	HWND plaintext_checkbox;

	HWND clipboard_tooltip;
	HWND highlight_tooltip;
	HWND add_tooltip;
	HWND regex_cb_tooltip;
	HWND whole_word_cb_tooltip;
	HWND keyword_tooltip;

	HFONT font;

	int linecount;
	int linecount_strlen;

	/* sum of strlen of all matched strings : used for copy to clipboard allocation */
	int total_strlen;

	/* 
	** EDIT and BUTTON controls are subclassed to handle key events (tab navigation etc).
	** Save default window procedures for EDIT and BUTTON controls in these vars as these 
	** need to be invoked from the overriden procedures.
	*/
	WNDPROC edit_proc;
	WNDPROC search_btn_proc;
	WNDPROC list_proc;
	WNDPROC add_proc;

	SPUI_mode_t mode;

	HWND assign_tooltip(HWND button, HWND parent, TCHAR *text);
	void create_controls(HWND parent);
	bool create_window();
	keyword_t *get_selected_keyword();

	void hide_window();

	/* 
	Updates checkbox and input field values to reflect the passed keyword.
	*/
	void highlight_keyword(keyword_t *kw);
	void reset_highlights();
	void exit_edit();

public:

	SearchPlusUI(HWND npp);

	~SearchPlusUI(){
		if(!close_flag){
			close_window();
		}
	}

	int launch();

	void handle_window_create(HWND hwnd);
	void handle_window_close();
	void close_window();
	WNDPROC get_wnd_proc(HWND hwnd);

	void handle_add_button();
	void handle_remove_button();
	void handle_reset_button();
	void handle_search_button();
	void handle_copy_button();
	void handle_highlight_button();
	void handle_cancel_button();
	void handle_update_button();

	void handle_keyword_selection();
	void start_keyword_edit();

	HBRUSH draw_checkbox(HWND checkbox, HDC hdc);

	void handle_escape_key();

	void resize_controls();

	void draw_matched_string(LPDRAWITEMSTRUCT pDis);

	void draw_keyword(LPDRAWITEMSTRUCT pDis);

	void load_keywords(SearchPattern *pat_list);

	void goto_selected_line();

	void handle_matching_line(int line_number, int line_length, CHAR *text, SearchPattern *pat, int match_start, int match_length);

	void handle_window_activate(bool focus);

	int handle_keys_tabbed_control(HWND hwnd, WPARAM wParam, LPARAM lParam);

	void update_result_count(int count);

	void handle_search_complete(int count, SearchPattern *patlist);

	void handle_keywordlb_focuschange();

	bool get_minmax_size(MINMAXINFO *minmax);
};


static SearchPlusUI *ui_data;

void SearchPlusUI::handle_matching_line(int line_number, int line_length, CHAR *text, SearchPattern *pat, int match_start, int match_length)
{
	TCHAR *temp = NULL;
	matched_line_t *list_item = NULL;
	int item_count;
	RECT item_rect = {0, 0, 0, 0};

	item_count = SendMessage(output_area, LB_GETCOUNT, 0, 0);

	while(--item_count >= 0){

		list_item = (matched_line_t*)SendMessage(output_area, LB_GETITEMDATA, item_count, 0);
		
		if(list_item->line_number == line_number){
		
			list_item->add_match(match_start, match_length, pat->getStyle());
			SendMessage(output_area, LB_GETITEMRECT, item_count, LPARAM(&item_rect));
			InvalidateRect(output_area, &item_rect, true);
			return;
		}

		if(list_item->line_number < line_number)
			break;
	}

	temp = new TCHAR[line_length + 1];

	mbstowcs(temp, text, line_length);
	temp[line_length] = 0;

	total_strlen += line_length;
	list_item = new matched_line_t(temp, line_length, line_number, match_start, match_length, pat->getStyle());
	
	SendMessage(output_area, LB_ADDSTRING, 0, (LPARAM)list_item);

	delete temp;
}

void SearchPlusUI::update_result_count(int count)
{
	TCHAR text[RESULT_LBL_TEXT_LENGTH] = {0};

	wsprintf(text, TEXT("%d matches.."), count);

	SendMessage(result_label, WM_SETTEXT, 0, LPARAM(text));
}

int SearchPlusUI::launch()
{
	TCHAR current_word[SP_MAX_PATTERN_LENGTH] = {0};
	int sel_length = SP_MAX_PATTERN_LENGTH;

	if(ui_data->create_window()){
		ui_data->load_keywords(PAT_GetList());
	}

	Ed_GetCurrentWord(current_word, sel_length);

	SendMessage(input_field, WM_SETTEXT, 0, WPARAM(current_word));

	/* display the window, and set caret at the end of the input text */
	ShowWindow(main_window, SW_SHOW);
	SendMessage(input_field, EM_SETSEL, sel_length, sel_length);
	SetFocus(input_field);

	return 0;
}

SearchPlusUI::SearchPlusUI(HWND parent)
{
	init_flag = false;
	close_flag = false;
	editor_handle = parent;
	input_field = NULL;
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

void SearchPlusUI::create_controls(HWND parent)
{
	TOOLINFO toolinfo = {0};

	input_field = ::CreateWindowEx(WS_EX_CLIENTEDGE,
		TEXT("EDIT"), EMPTY_STRING,
		WS_VISIBLE | WS_CHILD | WS_TABSTOP | WS_GROUP | ES_AUTOHSCROLL,
		CW_USEDEFAULT, CW_USEDEFAULT,
		CW_USEDEFAULT, CW_USEDEFAULT,
		parent, HMENU(SP_InstanceInputField),
		HINSTANCE(SP_InstanceInputField), 0);

	add_button = ::CreateWindowEx(0,
		TEXT("BUTTON"), BTN_LBL_ADD,
		WS_VISIBLE | WS_CHILD | WS_TABSTOP,
		CW_USEDEFAULT, CW_USEDEFAULT,
		CW_USEDEFAULT, CW_USEDEFAULT,
		parent, HMENU(SP_InstanceAddButton),
		HINSTANCE(SP_InstanceAddButton), 0);

	keyword_listbox = ::CreateWindowEx(WS_EX_CLIENTEDGE,
		TEXT("LISTBOX"), EMPTY_STRING,
		WS_VISIBLE | WS_CHILD | WS_TABSTOP | WS_VSCROLL | LBS_WANTKEYBOARDINPUT |
			LBS_OWNERDRAWFIXED | LBS_NOTIFY,
		CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT,
		parent,	HMENU(SP_InstancePatternList), 
		HINSTANCE(SP_InstancePatternList), 0);

	output_area  = ::CreateWindowEx(WS_EX_CLIENTEDGE,
		TEXT("LISTBOX"), EMPTY_STRING,
		WS_VISIBLE | WS_CHILD | WS_TABSTOP |  WS_HSCROLL | WS_VSCROLL | 
			LBS_OWNERDRAWFIXED | LBS_NOTIFY | LBS_WANTKEYBOARDINPUT, 
		CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT,
		parent, HMENU(SP_InstanceOutputArea),
		HINSTANCE(SP_InstanceOutputArea), 0);

	remove_button = ::CreateWindowEx(0, TEXT("BUTTON"),
		BTN_LBL_DELETE,
		WS_VISIBLE | WS_CHILD | WS_TABSTOP,
		CW_USEDEFAULT, CW_USEDEFAULT,
		CW_USEDEFAULT, CW_USEDEFAULT,
		parent, HMENU(SP_InstanceRemoveButton),
		HINSTANCE(SP_InstanceRemoveButton), 0);

	reset_button = ::CreateWindowEx(0, TEXT("BUTTON"),
		BTN_LBL_RESET,
		WS_VISIBLE | WS_CHILD | WS_TABSTOP,
		CW_USEDEFAULT, CW_USEDEFAULT,
		CW_USEDEFAULT, CW_USEDEFAULT,
		parent, HMENU(SP_InstanceClearButton),
		HINSTANCE(SP_InstanceClearButton), 0);

	search_button = CreateWindowEx(0, TEXT("BUTTON"),
		BTN_LBL_SEARCH,
		WS_VISIBLE | WS_CHILD | WS_TABSTOP,
		CW_USEDEFAULT, CW_USEDEFAULT,
		CW_USEDEFAULT, CW_USEDEFAULT,
		parent, HMENU(SP_InstanceSearchButton),
		HINSTANCE(SP_InstanceSearchButton), 0);

	list_label = CreateWindowEx(0, TEXT("STATIC"), LIST_LBL_NORMAL,
		WS_VISIBLE | WS_CHILD | SS_EDITCONTROL,
		CW_USEDEFAULT, CW_USEDEFAULT, 
		CW_USEDEFAULT, CW_USEDEFAULT, 
		parent, NULL, NULL, 0);

	result_label = CreateWindowEx(0, TEXT("STATIC"), EMPTY_STRING,
		WS_VISIBLE | WS_CHILD | SS_EDITCONTROL,
		CW_USEDEFAULT, CW_USEDEFAULT, 
		CW_USEDEFAULT, CW_USEDEFAULT, 
		parent, NULL, NULL, 0);

	copy_button = CreateWindowEx(0, TEXT("BUTTON"),
		BTN_LBL_COPY,
		WS_VISIBLE | WS_CHILD | WS_TABSTOP | WS_DISABLED,
		CW_USEDEFAULT, CW_USEDEFAULT,
		CW_USEDEFAULT, CW_USEDEFAULT,
		parent, HMENU(SP_InstanceClipboardButton),
		HINSTANCE(SP_InstanceClipboardButton), 0);

	highlight_button = CreateWindowEx(0, TEXT("BUTTON"),
		BTN_LBL_HIGHLIGHT,
		WS_VISIBLE | WS_CHILD | WS_TABSTOP | WS_DISABLED,
		CW_USEDEFAULT, CW_USEDEFAULT,
		CW_USEDEFAULT, CW_USEDEFAULT,
		parent, HMENU(SP_InstanceHighlightButton),
		HINSTANCE(SP_InstanceHighlightButton), 0);
	
	update_button = CreateWindowEx(0, TEXT("BUTTON"),
		BTN_LBL_UPDATE,
		WS_CHILD | WS_TABSTOP,
		CW_USEDEFAULT, CW_USEDEFAULT,
		CW_USEDEFAULT, CW_USEDEFAULT,
		parent, HMENU(SP_InstanceUpdateButton),
		HINSTANCE(SP_InstanceUpdateButton), 0);

	cancel_button = CreateWindowEx(0, TEXT("BUTTON"),
		BTN_LBL_CANCEL,
		WS_CHILD | WS_TABSTOP,
		CW_USEDEFAULT, CW_USEDEFAULT,
		CW_USEDEFAULT, CW_USEDEFAULT,
		parent, HMENU(SP_InstanceCancelButton),
		HINSTANCE(SP_InstanceCancelButton), 0);

	case_sensitivity_checkbox = ::CreateWindowEx(0,
		TEXT("BUTTON"), CHK_BOX_LABEL_CASE,
		WS_VISIBLE | WS_CHILD | WS_TABSTOP | BS_AUTOCHECKBOX, 
		CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT,
		parent, HMENU(SP_InstanceCaseSensitivityCB),
		HINSTANCE(SP_InstanceCaseSensitivityCB), 0);

	whole_word_checkbox = ::CreateWindowEx(0,
		TEXT("BUTTON"), CHK_BOX_LABEL_WORD,
		WS_VISIBLE | WS_CHILD | WS_TABSTOP | BS_AUTOCHECKBOX, 
		CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT,
		parent, HMENU(SP_InstanceMatchWordCB),
		HINSTANCE(SP_InstanceMatchWordCB), 0);

	plaintext_checkbox = ::CreateWindowEx(0,
		TEXT("BUTTON"), CHK_BOX_LABEL_REGEX,
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

	font = CreateFont(14, 0, 0, 0, FW_DONTCARE, FALSE, FALSE, FALSE, DEFAULT_CHARSET,
		OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS, DEFAULT_QUALITY, DEFAULT_PITCH | FF_SWISS,
		TEXT("Verdana"));
	
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
	}
}

void SearchPlusUI::close_window()
{
	SetWindowLongPtr(input_field, GWLP_WNDPROC, (LONG_PTR)edit_proc);
	SetWindowLongPtr(search_button, GWLP_WNDPROC, (LONG_PTR)search_btn_proc);
	SetWindowLongPtr(keyword_listbox, GWLP_WNDPROC, (LONG_PTR)list_proc);
	SetWindowLongPtr(add_button, GWLP_WNDPROC, (LONG_PTR)add_proc);

	CloseWindow(main_window);
	DeleteObject(font);
	close_flag = true;
}

bool SearchPlusUI::create_window()
{
	RECT rect = {0, 0, 0, 0};
	HWND desktop;
	int x, y;

	if(init_flag)
		return false;

	desktop = GetDesktopWindow();

	GetClientRect(desktop, &rect);

	x = (rect.right - SP_PANEL_WIDTH) / 2;
	y = (rect.bottom - SP_WINDOW_HEIGHT) / 2;



	main_window = ::CreateWindowEx(
		WS_EX_TOOLWINDOW | WS_EX_LAYERED,
		className,
		TEXT("Search+"),
		WS_SYSMENU | WS_SIZEBOX | WS_TABSTOP | WS_GROUP | WS_MAXIMIZEBOX | WS_MINIMIZEBOX | WS_CLIPCHILDREN,
		x, y, SP_PANEL_WIDTH, SP_WINDOW_HEIGHT,
		this->editor_handle, NULL,
		(HINSTANCE)SP_InstanceWndClass,
		0);

	init_flag = true;

	mode = UI_MODE_NORMAL;

	return true;
}

void SearchPlusUI::handle_window_close()
{
	init_flag = false;
	DestroyWindow(main_window);

	if(font)
		DeleteObject(font);

	Ed_SetFocusOnEditor();
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

	ui_panel_width = SP_PANEL_WIDTH;

	x_offset = SP_UIPANEL_MARGIN_H + PATTERN_LISTBOX_WIDTH + SP_PADDING_HORI;

	if(width > SP_PANEL_WIDTH){
		x_offset += (width - SP_PANEL_WIDTH) / 2;
	}

	MoveWindow(output_area, 
		SP_TA_X, SP_TA_Y, 
		parent_rect.right,
		parent_rect.bottom - SP_TA_Y, true);

	MoveWindow(input_field, x_offset - PATTERN_LISTBOX_WIDTH - SP_PADDING_HORI, 
		SP_INPUT_Y, PATTERN_LISTBOX_WIDTH, 
		SP_ROW_HEIGHT, true);

	MoveWindow(search_button, x_offset - SP_PADDING_HORI - SP_BTN_WIDTH,
		SP_BTN_Y, SP_BTN_WIDTH, SP_ROW_HEIGHT, true);

	MoveWindow(add_button, x_offset - 2 * (SP_PADDING_HORI + SP_BTN_WIDTH),
		SP_BTN_Y, SP_BTN_WIDTH, SP_ROW_HEIGHT, true);

	MoveWindow(remove_button, x_offset - 3 * (SP_PADDING_HORI + SP_BTN_WIDTH),
		SP_BTN_Y, SP_BTN_WIDTH, SP_ROW_HEIGHT, true);

	MoveWindow(reset_button, x_offset - 4 * (SP_PADDING_HORI + SP_BTN_WIDTH),
		SP_BTN_Y, SP_BTN_WIDTH, SP_ROW_HEIGHT, true);

	MoveWindow(update_button, x_offset - SP_PADDING_HORI - SP_BTN_WIDTH,
		SP_BTN_Y, SP_BTN_WIDTH, SP_ROW_HEIGHT, true);

	MoveWindow(cancel_button, x_offset - 2 * (SP_PADDING_HORI + SP_BTN_WIDTH),
		SP_BTN_Y, SP_BTN_WIDTH, SP_ROW_HEIGHT, true);

	MoveWindow(keyword_listbox, x_offset + SP_PADDING_HORI, 
		SP_Y_OFFSET, PATTERN_LISTBOX_WIDTH,
		SP_LIST_HEIGHT, true);

	MoveWindow(list_label, x_offset - SP_LISTLABEL_WIDTH, 
		SP_Y_OFFSET, SP_LISTLABEL_WIDTH, SP_LBL_HEIGHT, true);

	MoveWindow(result_label, SP_MARGIN_HORIZONTAL, SP_TA_Y - SP_LBL_HEIGHT, 
		SP_RESLABEL_WIDTH, SP_LBL_HEIGHT, true);

	MoveWindow(copy_button, x_offset - SP_PADDING_HORI - SP_COPY_BTN_WIDTH,
		SP_BTN_Y + SP_Y_OFFSET + SP_ROW_HEIGHT, SP_COPY_BTN_WIDTH, SP_ROW_HEIGHT, true);

	MoveWindow(highlight_button, x_offset - 2 * SP_PADDING_HORI - SP_COPY_BTN_WIDTH - HIGHLIGHT_BTN_WIDTH,
		SP_BTN_Y + SP_Y_OFFSET + SP_ROW_HEIGHT, 
		HIGHLIGHT_BTN_WIDTH, SP_ROW_HEIGHT, true);

	MoveWindow(case_sensitivity_checkbox, x_offset + 2 * SP_PADDING_HORI + PATTERN_LISTBOX_WIDTH, 
		SP_Y_OFFSET, SP_CHECKBOX_WIDTH, SP_CHECKBOX_HEIGHT, true);

	MoveWindow(whole_word_checkbox, x_offset + 2 * SP_PADDING_HORI + PATTERN_LISTBOX_WIDTH, 
		SP_Y_OFFSET + SP_CHECKBOX_HEIGHT, SP_CHECKBOX_WIDTH, SP_CHECKBOX_HEIGHT, true);

	MoveWindow(plaintext_checkbox, x_offset + 2 * SP_PADDING_HORI + PATTERN_LISTBOX_WIDTH, 
		SP_Y_OFFSET + 2 * SP_CHECKBOX_HEIGHT, SP_CHECKBOX_WIDTH, SP_CHECKBOX_HEIGHT, true);

	InvalidateRect(main_window, NULL, true);
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
	edit_proc = (WNDPROC)SetWindowLongPtr(input_field, GWLP_WNDPROC, (LONG_PTR)TabbedControlProc);
	search_btn_proc = (WNDPROC)SetWindowLongPtr(search_button, GWLP_WNDPROC, (LONG_PTR)TabbedControlProc);
	list_proc = (WNDPROC)SetWindowLongPtr(keyword_listbox, GWLP_WNDPROC, (LONG_PTR)TabbedControlProc);
	add_proc = (WNDPROC)SetWindowLongPtr(add_button, GWLP_WNDPROC, (LONG_PTR)TabbedControlProc);
}

void SearchPlusUI::handle_add_button()
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

	index = PAT_GetIndex(txt);

	if(index == -1){

		case_flag = BST_CHECKED == SendMessage(case_sensitivity_checkbox, BM_GETCHECK, 0, 0);
		regex_flag = BST_CHECKED != SendMessage(plaintext_checkbox, BM_GETCHECK, 0, 0);
		word_flag = BST_CHECKED == SendMessage(whole_word_checkbox, BM_GETCHECK, 0, 0);

		keyword = PAT_Add(txt, case_flag, word_flag, regex_flag);

		if(!keyword){
			DBG_MSG("Check the keyword for regex error");
			return;
		}

		keyword_t *ui_kw = new keyword_t(keyword);
		index = SendMessage(keyword_listbox, LB_ADDSTRING, 0, LPARAM(ui_kw));
	}


	SendMessage(keyword_listbox, LB_SETCARETINDEX, index, FALSE);
	SetWindowText(input_field, EMPTY_STRING);
	SetFocus(input_field);
}

void SearchPlusUI::handle_remove_button()
{
	int index = SendMessage(keyword_listbox, LB_GETCURSEL, 0, 0);

	if(index == LB_ERR){
		return;
	}

	PAT_Delete(index);

	SendMessage(keyword_listbox, LB_DELETESTRING, index, 0);
}

void SearchPlusUI::handle_reset_button()
{
	PAT_DeleteAll();
	SendMessage(keyword_listbox, LB_RESETCONTENT, 0, 0);
	SendMessage(output_area, LB_RESETCONTENT, 0, 0);
	SendMessage(result_label, WM_SETTEXT, 0, (LPARAM)EMPTY_STRING);
	EnableWindow(copy_button, false);
	EnableWindow(highlight_button, false);
}

void SearchPlusUI::handle_copy_button()
{
	int count = 0;
	matched_line_t *item;
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

		item = (matched_line_t *)SendMessage(output_area, LB_GETITEMDATA, index, 0);
#ifdef COPY_STRINGS_THE_STUPID_WAY
		--> This was a nice ego unbooster. //break the build if stupid macro is enabled.
		wcscat_s(clipboard_string, clipboard_len, item->text);
#else
		memcpy(buf_pos, item->text, sizeof(TCHAR) * item->line_length);
		buf_pos += item->line_length;
#endif
	}

	*buf_pos = 0;

	ret = OpenClipboard(main_window);
	ret = EmptyClipboard();
	hnd = SetClipboardData(CF_UNICODETEXT, clipboard_string);
	ret = CloseClipboard();
}

void SearchPlusUI::handle_highlight_button()
{
	matched_line_t *lbitem = NULL;
	matched_segment_t *segment;
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

			Ed_HighlightWord(lbitem->line_number, segment->from, segment->length, segment->style);
			segment = segment->next;
		}
	}

}

void SearchPlusUI::handle_search_button()
{
	TCHAR temp[NUM_LENGTH] = {0};
	int result;

	if(mode != UI_MODE_NORMAL){
		return;
	}

	mode = UI_MODE_SEARCH;

	linecount = Ed_GetLineCount();

	if(linecount > MAX_LINES_SUPPORTED){
		DBG_MSG("File is too big: split it and try again");
		return;
	}

	_itow_s(linecount, temp, 10, 10);
	linecount_strlen = wcslen(temp);

	total_strlen = 0;

	SendMessage(output_area, LB_RESETCONTENT, 0, 0);

	result = Ed_Search();

	if(result == 0){
		SendMessage(result_label, WM_SETTEXT, 0, (LPARAM)TEXT("Searching.."));
		EnableWindow(reset_button, false);
		EnableWindow(add_button, false);
		EnableWindow(remove_button, false);
		EnableWindow(search_button, false);
	}
	else{
		SendMessage(result_label, WM_SETTEXT, 0, (LPARAM)EMPTY_STRING);
	}
}

void SearchPlusUI::hide_window()
{
	ShowWindow(main_window, SW_HIDE);
	Ed_SetFocusOnEditor();
}

void SearchPlusUI::handle_escape_key()
{
	if(mode == UI_MODE_EDIT_KW){
		exit_edit();
	}
	else{
		hide_window();
	}
}

void SearchPlusUI::load_keywords(SearchPattern *pat_list)
{
	SearchPattern *temp_pat = pat_list;
	keyword_t *ui_kw = NULL;

	while(temp_pat){
		ui_kw = new keyword_t(temp_pat);
		::SendMessage(keyword_listbox, LB_ADDSTRING, 0, LPARAM(ui_kw));
		temp_pat = temp_pat->next;
	}
}

void SearchPlusUI::draw_keyword(LPDRAWITEMSTRUCT pDis)
{
	keyword_t *uipat = (keyword_t*)(pDis->itemData);
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

	length = wcslen(uipat->string);

	if(uipat->count){
		wsprintf(temp, TEXT("%s (%d)"), uipat->string, uipat->count);
	}
	else{
		wcscpy_s(temp, SP_MAX_PATTERN_LENGTH + NUM_LENGTH, uipat->string);
	}

	length = wcslen(temp);

	draw_text(pDis->hDC, temp, length, &text_rect, uipat->color);

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
	matched_line_t *lbitem = (matched_line_t*)(pDis->itemData);
	RECT text_rect = pDis->rcItem;
	int i = 0, j = 0;
	matched_segment_t *matchlist = NULL;

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

	lbitem->draw_item(pDis->hDC, &text_rect);
}

void SearchPlusUI::goto_selected_line()
{
	matched_line_t *lbitem = NULL;
	int index;
	
	index = SendMessage(output_area, LB_GETCURSEL, 0, 0);
	SendMessage(output_area, LB_GETTEXT, index, WPARAM(&lbitem));

	ShowWindow(main_window, SW_HIDE);

	Ed_GotoMatch(lbitem->line_number, lbitem->match_list->from,
		lbitem->match_list->length);
}

WNDPROC SearchPlusUI::get_wnd_proc(HWND hwnd)
{
	if(hwnd == input_field){
		return edit_proc;
	}
	else if(hwnd == keyword_listbox){
		return list_proc;
	}
	else if(hwnd == add_button){
		return add_proc;
	}
	else if(hwnd == search_button){
		return search_btn_proc;
	}

	return NULL;
}
#define TAB_CTRL_COUNT (4)
int SearchPlusUI::handle_keys_tabbed_control(HWND hwnd, WPARAM wParam, LPARAM lParam)
{
	HWND windows[TAB_CTRL_COUNT] = {input_field, add_button, search_button, keyword_listbox};
	HWND focused_window = NULL;
	int index;
	int retval = 0;
	unsigned short shift = GetKeyState(VK_SHIFT);

	if(wParam == VK_ESCAPE){
		ui_data->handle_escape_key();
		return 0;
	}

	/* Don't do anything unless you are normal */
	if(mode != UI_MODE_NORMAL){
		return 1;
	}

	if(wParam == VK_RETURN){

		if(hwnd == input_field || hwnd == add_button){
			ui_data->handle_add_button();
		}
		else if(hwnd == search_button){
			ui_data->handle_search_button();
		}
	}
	else if(wParam == VK_TAB){
		
		for(index = 0; (index < TAB_CTRL_COUNT) && (windows[index] != hwnd); index++);

		if(index < TAB_CTRL_COUNT){

			focused_window = (shift & 0x80) ? windows[(index + TAB_CTRL_COUNT - 1) % TAB_CTRL_COUNT] :
				windows[++index % TAB_CTRL_COUNT];

			SetFocus(focused_window);
		}
	}
	else if(wParam == VK_DELETE && hwnd == keyword_listbox){
		ui_data->handle_remove_button();
	}
	else{
		retval = 1;
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

keyword_t *SearchPlusUI::get_selected_keyword()
{
	int index = LB_ERR;
	keyword_t *kw = NULL;

	index = SendMessage(keyword_listbox, LB_GETCURSEL, 0, 0);

	if(index == LB_ERR){
		return NULL;
	}

	SendMessage(keyword_listbox, LB_GETTEXT, index, WPARAM(&kw));

	return kw;
}

void SearchPlusUI::start_keyword_edit()
{
	keyword_t *kw = NULL;

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

void SearchPlusUI::highlight_keyword(keyword_t *kw)
{
	int check_status;

	if(!kw){
		return;
	}

	check_status = kw->case_flag ? BST_CHECKED : BST_UNCHECKED;
	SendMessage(case_sensitivity_checkbox, BM_SETCHECK, check_status, 0);

	check_status = kw->word_flag ? BST_CHECKED : BST_UNCHECKED;
	SendMessage(whole_word_checkbox, BM_SETCHECK, check_status, 0);

	check_status = kw->regex_flag ? BST_UNCHECKED : BST_CHECKED;
	SendMessage(plaintext_checkbox, BM_SETCHECK, check_status, 0);

	SendMessage(input_field, WM_SETTEXT, 0, LPARAM(kw->string));
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

		keyword_t *ui_kw = (keyword_t*)SendMessage(keyword_listbox, LB_GETITEMDATA, index, 0);

		if(ui_kw){
			ui_kw->update(keyword);
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

void SearchPlusUI::handle_keyword_selection()
{
	keyword_t *kw = NULL;

	if((kw = get_selected_keyword()) == NULL){
		return;
	}

	exit_edit();
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

	if(minmax->ptMinTrackSize.x < SP_PANEL_WIDTH){
		minmax->ptMinTrackSize.x = SP_PANEL_WIDTH;
		minmax->ptMinTrackSize.y = (minmax->ptMaxSize.y * 50) / 100;
		return true;
	}

	return false;
}

LRESULT CALLBACK TabbedControlProc(HWND hWnd, UINT iMessage, WPARAM wParam, LPARAM lParam) 
{
	WNDPROC proc = ui_data->get_wnd_proc(hWnd);
	LRESULT retval = 1;

	if(iMessage == WM_KEYDOWN){
		retval = ui_data->handle_keys_tabbed_control(hWnd, wParam, lParam);
	}

	if(retval){
		retval = CallWindowProc(proc, hWnd, iMessage, wParam, lParam);
	}

	return retval;
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
		PAT_ResetMatchCount();
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
			matched_line_t *item = (matched_line_t *)(((DELETEITEMSTRUCT *)lParam)->itemData);
			delete item;
		}
		else if(wParam == SP_InstancePatternList){
			keyword_t *uipat = (keyword_t*)(((DELETEITEMSTRUCT *)lParam)->itemData);
			delete uipat;
		}
		break;

	case WM_ACTIVATE:
		ui_data->handle_window_activate(wParam != WA_INACTIVE);
		break;

	default:
		break;
	}

	return DefWindowProc(hWnd, iMessage, wParam, lParam);
}

void UI_Terminate()
{
	ui_data->close_window();
	delete ui_data;
	ui_data = NULL;
}

void UI_Initialize(HWND parent_handle)
{
	WNDCLASS wndclass;

	ui_data = new SearchPlusUI(parent_handle);

	wndclass.style = CS_HREDRAW | CS_VREDRAW;
	wndclass.lpfnWndProc = WndProc;
	wndclass.cbClsExtra = 0;
	wndclass.cbWndExtra = 0;
	wndclass.hInstance = HINSTANCE(SP_InstanceWndClass);
	wndclass.hIcon = 0;
	wndclass.hCursor = NULL;
	wndclass.hbrBackground = (HBRUSH) GetStockObject(WHITE_BRUSH);
	wndclass.lpszMenuName = NULL;
	wndclass.lpszClassName = className;

	if (!::RegisterClass(&wndclass)){
		::MessageBox(NULL, TEXT("register class failed"), TEXT("Err: Search+ plugin"), MB_OK);
	}
}

void UI_ShowSettingsWindow()
{
	ui_data->launch();
}

void UI_HandleMatchingLine(int line_number, int line_length, CHAR *text, SearchPattern *pat, int match_start, int match_length)
{
	ui_data->handle_matching_line(line_number, line_length, text, pat, match_start, match_length);
}

void UI_UpdateResultCount(int count)
{
	ui_data->update_result_count(count);
}

void SearchPlusUI::handle_search_complete(int count, SearchPattern *patlist)
{
	TCHAR text[RESULT_LBL_TEXT_LENGTH] = {0};
	int matched_line_count, pos;
	keyword_t *uipat;

	matched_line_count = SendMessage(output_area, LB_GETCOUNT, 0, 0);

	wsprintf(text, TEXT("%d matches in %d lines"), count, matched_line_count);

	SendMessage(result_label, WM_SETTEXT, 0, LPARAM(text));

	pos = 0;

	while(patlist){

		uipat = (keyword_t*)SendMessage(keyword_listbox, LB_GETITEMDATA, pos, 0);

		if(uipat){
			uipat->count = patlist->count;
			SendMessage(keyword_listbox, LB_SETITEMDATA, pos, LPARAM(uipat));
		}

		pos++;
		patlist = patlist->next;
	}
	
	InvalidateRect(keyword_listbox, NULL, true);
	UpdateWindow(keyword_listbox);

	EnableWindow(reset_button, true);
	EnableWindow(add_button, true);
	EnableWindow(remove_button, true);
	EnableWindow(search_button, true);
	EnableWindow(copy_button, true);
	EnableWindow(highlight_button, true);

	mode = UI_MODE_NORMAL;
}

void UI_HandleSearchComplete(int count, SearchPattern *patlist)
{
	ui_data->handle_search_complete(count, patlist);
}