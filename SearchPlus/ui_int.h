
/*
This file contains macros and other definitions used for UI - not to be used elsewhere.
The UI interfaces are defined in searchplus.h
*/

#ifndef SP_UI_INT_H
#define SP_UI_INT_H

/* DrawText flags */
#define DT_FLAGS (DT_EXPANDTABS | DT_NOCLIP | DT_SINGLELINE)

/* Window transparency */
#define ALPHA_OPAQUE (255)
#define ALPHA_SEE_THRU (192)

/* Colors used in list box etc  */
#define COLOR_NORMAL RGB(0, 0, 0)
#define COLOR_LINE_NUM RGB(128, 128, 128)
#define COLOR_SEL_BKG RGB(192, 192, 192)
#define COLOR_WHITE RGB(255, 255, 255)

#define MAX_TAB_CONTROLS (8)
#define TAB_COUNT_NORMAL (6)
#define TAB_COUNT_EDIT (4)


/* strings used as labels for various control */
#define BTN_LBL_ADD TEXT("Add")
#define BTN_LBL_DELETE TEXT("Delete")
#define BTN_LBL_RESET TEXT("Reset")
#define BTN_LBL_SEARCH TEXT("Search")
#define BTN_LBL_STOP TEXT("Stop")
#define BTN_LBL_COPY TEXT("Ctrl+C")
#define BTN_LBL_HIGHLIGHT TEXT("Highlight")
#define BTN_LBL_UPDATE TEXT("Update")
#define BTN_LBL_CANCEL TEXT("Cancel")
#define BTN_LBL_NEXT TEXT(">")
#define BTN_LBL_PREV TEXT("<")

#define CHK_BOX_LABEL_CASE TEXT("Case Sensitive")
#define CHK_BOX_LABEL_WORD TEXT("Whole Words only")
#define CHK_BOX_LABEL_REGEX TEXT("Use plain text")

#define LIST_LBL_NORMAL TEXT("Add keywords to the list and click Search")
#define LIST_LBL_EDIT TEXT("Edit the text and flags and click Update")
#define SP_VERSION_STR TEXT("Search+ Version ") SP_CUR_VER


/* max number of chars in result label : "%d matches in %d lines" */
#define RESULT_LABEL_TEXT_LENGTH (30)


#define MAX_LINES_SUPPORTED (10000000)
/*
This should be big enough to hold itoa(MAX_LINES_SUPPORTED) + " : "
*/
#define NUM_LENGTH (15)


/* size of various controls */
#define SP_WINDOW_HEIGHT (500)

#define SP_MARGIN_H (5)
#define SP_PADDING_H (3)

#define SP_PADDING_V (5)
#define SP_MARGIN_V (5)

#define SP_ROW_HEIGHT (22)
#define SP_LBL_HEIGHT (15)

#define SP_UIPANEL_MIN_W ((KW_LISTBOX_W + KW_INPUT_W + (SP_UIPANEL_MARGIN_H + SP_PADDING_H) * 2) + SP_CHECKBOX_W)
#define SP_UIPANEL_MARGIN_H (20)

#define KW_LABEL_X(center) ((center) - KW_LABEL_W)
#define KW_LABEL_Y SP_MARGIN_V
#define KW_LABEL_W (260)
#define KW_LABEL_H SP_LBL_HEIGHT

#define KW_INPUT_X(center) ((center) - KW_INPUT_W - SP_PADDING_H)
#define KW_INPUT_Y (SP_MARGIN_V + KW_LABEL_H)
#define KW_INPUT_W (300)
#define KW_INPUT_H SP_ROW_HEIGHT

#define KW_LISTBOX_X(center) ((center) + SP_PADDING_H)
#define KW_LISTBOX_Y SP_MARGIN_V
#define KW_LISTBOX_W (300)
#define KW_LISTBOX_H (100)

#define SP_BTN_FIRST_ROW_X(center, index) ((center) - ((index + 1) *  (SP_PADDING_H + SP_BTN_FIRST_ROW_W)))
#define SP_BTN_FIRST_ROW_Y (KW_INPUT_Y + KW_INPUT_H + SP_PADDING_V)
#define SP_BTN_FIRST_ROW_W (70)
#define SP_BTN_SECOND_ROW_Y (SP_BTN_FIRST_ROW_Y + SP_ROW_HEIGHT + SP_PADDING_H)

#define BTN_SEARCH_X(center) SP_BTN_FIRST_ROW_X(center, 0)
#define BTN_SEARCH_Y SP_BTN_FIRST_ROW_Y
#define BTN_SEARCH_W SP_BTN_FIRST_ROW_W
#define BTN_SEARCH_H SP_ROW_HEIGHT

#define BTN_ADD_X(center) SP_BTN_FIRST_ROW_X(center, 1)
#define BTN_ADD_Y SP_BTN_FIRST_ROW_Y
#define BTN_ADD_W SP_BTN_FIRST_ROW_W
#define BTN_ADD_H SP_ROW_HEIGHT

#define BTN_REMOVE_X(center) SP_BTN_FIRST_ROW_X(center, 2)
#define BTN_REMOVE_Y SP_BTN_FIRST_ROW_Y
#define BTN_REMOVE_W SP_BTN_FIRST_ROW_W
#define BTN_REMOVE_H SP_ROW_HEIGHT

#define BTN_RESET_X(center) SP_BTN_FIRST_ROW_X(center, 3)
#define BTN_RESET_Y SP_BTN_FIRST_ROW_Y
#define BTN_RESET_W SP_BTN_FIRST_ROW_W
#define BTN_RESET_H SP_ROW_HEIGHT

#define BTN_UPDATE_X(center) SP_BTN_FIRST_ROW_X(center, 0)
#define BTN_UPDATE_Y SP_BTN_FIRST_ROW_Y
#define BTN_UPDATE_W SP_BTN_FIRST_ROW_W
#define BTN_UPDATE_H SP_ROW_HEIGHT

#define BTN_CANCEL_X(center) SP_BTN_FIRST_ROW_X(center, 1)
#define BTN_CANCEL_Y SP_BTN_FIRST_ROW_Y
#define BTN_CANCEL_W SP_BTN_FIRST_ROW_W
#define BTN_CANCEL_H SP_ROW_HEIGHT


#define BTN_COPY_X(center) ((center) - SP_PADDING_H - BTN_COPY_W)
#define BTN_COPY_Y SP_BTN_SECOND_ROW_Y
#define BTN_COPY_W (80)
#define BTN_COPY_H SP_ROW_HEIGHT

#define BTN_HIGHLIGHT_X(center) ((center) - 2 * SP_PADDING_H - BTN_COPY_W - BTN_HIGHLIGHT_W)
#define BTN_HIGHLIGHT_Y SP_BTN_SECOND_ROW_Y
#define BTN_HIGHLIGHT_W (80)
#define BTN_HIGHLIGHT_H SP_ROW_HEIGHT

#define RESULT_LABEL_X SP_MARGIN_H
#define RESULT_LABEL_Y OUTPUT_LIST_Y - RESULT_LABEL_H
#define RESULT_LABEL_W (200)
#define RESULT_LABEL_H SP_LBL_HEIGHT

#define SP_CHECKBOX_X(center) (KW_LISTBOX_X(center) + KW_LISTBOX_W + SP_PADDING_H)
#define SP_CHECKBOX_Y(position) (SP_MARGIN_V + (position) * SP_CHECKBOX_H)
#define SP_CHECKBOX_W (150)
#define SP_CHECKBOX_H (20)


#define CHKBOX_CASE_X(center) SP_CHECKBOX_X(center)
#define CHKBOX_CASE_Y SP_CHECKBOX_Y(0)
#define CHKBOX_CASE_W SP_CHECKBOX_W
#define CHKBOX_CASE_H SP_CHECKBOX_H

#define CHKBOX_WHOLEWORD_X(center) SP_CHECKBOX_X(center)
#define CHKBOX_WHOLEWORD_Y SP_CHECKBOX_Y(1)
#define CHKBOX_WHOLEWORD_W SP_CHECKBOX_W
#define CHKBOX_WHOLEWORD_H SP_CHECKBOX_H

#define CHKBOX_REGEX_X(center) SP_CHECKBOX_X(center)
#define CHKBOX_REGEX_Y SP_CHECKBOX_Y(2)
#define CHKBOX_REGEX_W SP_CHECKBOX_W
#define CHKBOX_REGEX_H SP_CHECKBOX_H

#define PREV_BTN_X(center) SP_CHECKBOX_X(center)
#define PREV_BTN_Y (OUTPUT_LIST_Y - PREV_BTN_H - SP_MARGIN_V)
#define PREV_BTN_W (20)
#define PREV_BTN_H (20)

#define NEXT_BTN_X(center) (PREV_BTN_X(center) + PREV_BTN_W)
#define NEXT_BTN_Y PREV_BTN_Y
#define NEXT_BTN_W PREV_BTN_W
#define NEXT_BTN_H PREV_BTN_H

#define OUTPUT_LIST_X (0)
#define OUTPUT_LIST_Y (KW_LISTBOX_H + SP_MARGIN_V + SP_PADDING_V)

#define ABOUT_WND_WIDTH (400)
#define ABOUT_WND_HEIGHT (160)

#define ABT_LINK_WIDTH ((80 * ABOUT_WND_WIDTH) / 100)
#define ABT_WND_VERT_PADDING (10)


enum{
	SP_InstanceNone,
	SP_InstanceSPWndClass,
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
	SP_InstanceNextButton,
	SP_InstancePrevButton,
	SP_InstanceCaseSensitivityCB,
	SP_InstanceMatchWordCB,
	SP_InstanceRegexCB,
	SP_InstanceAboutWindowClass,
	SP_InstanceAboutWindow,
	SP_InstanceUpdateLink
};

typedef enum {
	UI_MODE_NORMAL,
	UI_MODE_SEARCH,
	UI_MODE_EDIT_KW
}SPUI_mode_t;

typedef enum{
	SPM_MATCH_FOUND = WM_APP + 1,
	SPM_SEARCH_OVER,
	SPM_TERMINATE
}sp_custom_messages_t;

typedef struct{
	SearchPattern *pattern;
	int line_number;
	int line_length;
	int match_start;
	int match_length;
}match_msg_t;

/*
List of matched segments for a given keyword in a given line.
Each matched line will have a member of this struct.
*/
struct MatchedSegment{
	int from;
	int length;
	SearchPattern *pattern;
	MatchedSegment *next;

	MatchedSegment(int match_start, int match_length, SearchPattern *pattern);
	~MatchedSegment();
};

/* data item for the output listbox */
struct MatchedLine{
	TCHAR *text;
	int line_length;
	int line_number;
	MatchedSegment *match_list;

	MatchedLine(int line_number, int total_length, int match_from, int match_length, SearchPattern *pattern);
	~MatchedLine();

	void FetchLine();
	void AddMatch(int match_from, int match_length, SearchPattern *pattern);
	void Draw(HDC hdc, LPRECT full_rect);
};

typedef struct{
	HWND hwnd;
	WNDPROC proc;
}TabControl;

typedef enum{
	FIRST,
	NEXT,
	LAST,
	PREV
}MatchIterType;

class SearchPlusUI{
	bool close_flag;
	bool stop_flag;

	DWORD thread_id;
	HANDLE thread_handle;

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

	HWND next_button;
	HWND prev_button;

	HWND case_sensitivity_checkbox;
	HWND whole_word_checkbox;
	HWND plaintext_checkbox;

	HWND clipboard_tooltip;
	HWND highlight_tooltip;
	HWND add_tooltip;
	HWND regex_cb_tooltip;
	HWND whole_word_cb_tooltip;
	HWND keyword_tooltip;
	HWND next_btn_tooltip;
	HWND prev_btn_tooltip;

	HFONT font;

	int total_matches;

	int linecount;
	int linecount_strlen;

	/* sum of strlen of all matched strings : used for copy to clipboard allocation */
	int total_strlen;

	/*
	** EDIT and BUTTON controls are subclassed to handle key events (tab navigation etc).
	** Save default window procedures for EDIT and BUTTON controls in these vars as these
	** need to be invoked from the overriden procedures.
	*/
	TabControl tab_controls[MAX_TAB_CONTROLS];

	SPUI_mode_t mode;

	int output_width;

	HWND assign_tooltip(HWND button, HWND parent, TCHAR *text);
	void create_controls(HWND parent);
	bool create_sp_window();
	bool create_about_window();
	SearchPattern *get_selected_keyword();

	/*
	Updates checkbox and input field values to reflect the passed keyword.
	*/
	void highlight_keyword(SearchPattern *kw);
	void reset_highlights();
	void exit_edit();
	void exit_search();

	void register_window_classes();

	/* this might as well be static */
	static void handle_tab_order(HWND cur_wnd, HWND *windows, int count, bool forward);

public:

	HWND about_window;
	HWND update_link;
	HWND credit_text;

	SearchPlusUI(HWND npp);

	~SearchPlusUI();

	int LaunchSearchPlusWindow();
	int LaunchAboutWindow();
	void CreatePluginWindow();
	void LoadCurrentWord();

	void Terminate();

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

	void send_match_msg(int line_number, int line_length, SearchPattern *pat, int match_start, int match_length);

	void send_search_over();

	void handle_matching_line(match_msg_t *msg);

	void handle_window_activate(bool focus);

	int handle_keys_tabbed_control(HWND hwnd, WPARAM wParam, LPARAM lParam);

	void handle_search_complete();

	void handle_keywordlb_focuschange();

	bool get_minmax_size(MINMAXINFO *minmax);

	void stop_search();

	void goto_match(MatchIterType type);
};


#endif

