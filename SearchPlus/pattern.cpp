
/*
Search+ Plugin
amarghosh @ gmail dot com
*/


#include <regex>

#include "searchplus.h"

static int pat_count = 0;


SearchPattern::SearchPattern()
{
	text = NULL;
	case_sensitive = 0;
	next = NULL;
	count = 0;
	style_id = 0;
}

void escape_regex_chars(TCHAR *dst, TCHAR *src)
{
	while(*src){
			
		switch(*src){

		case TEXT('('):
		case TEXT(')'):
		case TEXT('['):
		case TEXT(']'):
		case TEXT('{'):
		case TEXT('}'):
		case TEXT('*'):
		case TEXT('.'):
		case TEXT('?'):
		case TEXT('^'):
		case TEXT('$'):
			*dst++ = TEXT('\\');
			break;
		}

		*dst++ = *src++;
		*dst = 0;
	}
}

void SearchPattern::generate_pattern(TCHAR *str, bool case_sensitive, bool whole_words_only, bool use_regex)
{
	int length, reg_count = 0;
	size_t convert_length = 0;
	regex_constants::syntax_option_type regex_flags = (regex_constants::syntax_option_type)0;
	/* 
	Accomodate possible escape characters in case of non-regex patterns.
	Add 4 bytes for \b in case of whole word matching
	*/
	char temp[SP_MAX_PATTERN_LENGTH * 2 + 4 + 1] = {0}; 

	length = wcslen(str);

	if(whole_words_only){
		/* Add \b at the start and end of the keyword */
		length += 4;
	}

	if(!use_regex){

		for(int loop = 0; loop < length; loop++){
			
			switch(str[loop]){

			case TEXT('('):
			case TEXT(')'):
			case TEXT('['):
			case TEXT(']'):
			case TEXT('{'):
			case TEXT('}'):
			case TEXT('*'):
			case TEXT('.'):
			case TEXT('?'):
			case TEXT('^'):
			case TEXT('$'):
				reg_count++;
				break;
			}
		}

		length += reg_count;
	}

	this->case_sensitive = case_sensitive;
	this->whole_words_flag = whole_words_only;
	this->is_regex = use_regex;

	if(!case_sensitive){
		regex_flags |= regex_constants::icase;
	}

	this->text = new TCHAR[length + 1];

	this->text[0] = 0;

	/* 
	Since input string is wchar, it would be easier to generate the required regex string 
	using the input and then convert it to ascii for regex, as npp text is ascii.
	Ultimately, store the input string in its original form as that's what we show it to user
	*/

	if(whole_words_only){
		/* this is just for constructing regex */
		wcscpy_s(this->text, length + 1, TEXT("\\b"));

		if(use_regex){
			wcscat_s(this->text, length + 1, str);
		}
		else{
			escape_regex_chars(this->text + 2, str);
		}

		wcscat_s(this->text, length + 1, TEXT("\\b"));
	}
	else if(use_regex){
		wcscpy_s(this->text, length + 1, str);
	}
	else{
		escape_regex_chars(this->text, str);
	}

	this->text[length] = 0;

	wcstombs_s(&convert_length, temp, sizeof(temp), this->text, length * sizeof(TCHAR));

	this->reg = new regex(temp, regex_flags);

	if(whole_words_only || !use_regex){
		/* keep the text same as original */
		wcscpy_s(this->text, length + 1, str);
	}

	this->count = 0;
}

void SearchPattern::destroy()
{
	if(text){
		delete text;
		text = NULL;
	}

	if(reg){
		delete reg;
		reg = NULL;
	}
}

SearchPattern::SearchPattern(TCHAR *str, bool case_sensitive, bool whole_words_only, bool use_regex)
{
	generate_pattern(str, case_sensitive, whole_words_only, use_regex);
	this->style_id = pat_count++ % LP_MAX_STYLE_ID;
	this->next = NULL;
}

void SearchPattern::update(TCHAR *str, bool case_sensitive, bool whole_words_only, bool use_regex)
{
	destroy();
	generate_pattern(str, case_sensitive, whole_words_only, use_regex);
}


SearchPattern::~SearchPattern()
{
	destroy();
}

TCHAR *SearchPattern::getText()
{
	return this->text;
}

bool SearchPattern::getCaseSensitivity()
{
	return this->case_sensitive;
}

bool SearchPattern::getWholeWordStatus()
{
	return this->whole_words_flag;
}

bool SearchPattern::getRegexStatus()
{
	return this->is_regex;
}

int SearchPattern::getStyle()
{
	return this->style_id;
}

bool SearchPattern::search(CHAR *text, int &from, int &length)
{
	cmatch res;

	if(!regex_search(text, res, *(this->reg))){
		return false;
	}

	from = res.position(0);
	length = res.length(0);
	count++;
	return true;
}

SearchPattern *g_pat_list;

int PAT_GetIndex(TCHAR *str)
{
	SearchPattern *pat = g_pat_list;
	int index = 0;

	if(!str){
		return -1;
	}

	while(pat){

		if(!wcscmp(str, pat->getText())){
			return index;
		}

		index++;
		pat = pat->next;
	}

	return -1;
}

SearchPattern * PAT_Add(TCHAR *str, bool case_sensitive, bool whole_words_only, bool regex_flag)
{
	SearchPattern *pat;
	SearchPattern *iter = g_pat_list;

	if(!str){
		return NULL;
	}

	if(PAT_GetIndex(str) >= 0){
		return NULL;
	}

	try{
		pat = new SearchPattern(str, case_sensitive, whole_words_only, regex_flag);
	}
	catch(regex_error er){
		return NULL;
	}

	if(!iter){
		g_pat_list = pat;
	}
	else{
		while(iter->next){
			iter = iter->next;
		}
		iter->next = pat;
	}

	return pat;
}

SearchPattern * PAT_Update(int index, TCHAR *str, bool case_sensitive, bool whole_words_only, bool regex_flag)
{
	SearchPattern *iter = g_pat_list;

	if(!str){
		return NULL;
	}

	for(int loop = 0; loop < index && iter != NULL; loop++){
		iter = iter->next;
	}

	if(!iter){
		return NULL;
	}

	iter->update(str, case_sensitive, whole_words_only, regex_flag);
	
	return iter;
}

pat_err_t PAT_Delete(int index)
{
	SearchPattern *pat = g_pat_list;
	SearchPattern *parent = NULL;
	int loop = 0;

	while(pat != NULL && loop++ < index){
		parent = pat;
		pat = pat->next;
	}

	if(loop == index || pat == NULL){
		return LP_FAILURE;
	}

	if(parent == NULL){
		g_pat_list = pat->next;
		delete pat;
	}
	else{
		parent->next = pat->next;
		delete pat;
	}

	return LP_SUCCESS;
}

pat_err_t PAT_ResetMatchCount()
{
	SearchPattern *pat = g_pat_list;

	while(pat){
		pat->count = 0;
		pat = pat->next;
	}

	return LP_SUCCESS;
}

pat_err_t PAT_DeleteAll()
{
	SearchPattern *pat = g_pat_list;
	SearchPattern *temp = NULL;
	int loop = 0;

	while(pat){
		temp = pat->next;
		delete pat;
		pat = temp;
	}

	g_pat_list = NULL;

	pat_count = 0;

	return LP_SUCCESS;
}


int LP_get_pattern_count()
{
	SearchPattern *pat = g_pat_list;
	int count = 0;

	while(pat){
		count++;
		pat = pat->next;
	}

	return count;
}

SearchPattern *PAT_GetList()
{
	return g_pat_list;
}