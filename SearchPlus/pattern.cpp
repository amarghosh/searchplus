
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
	length = 0;
	is_regex = 0;
	next = NULL;
	count = 0;
	style_id = 0;
}

SearchPattern::SearchPattern(TCHAR *str, bool is_regex)
{
	int length;
	/* accomodate possible escape characters in case of non-regex patterns */
	char temp[SP_MAX_PATTERN_LENGTH * 2] = {0}; 

	length = wcslen(str);

	this->length = length;
	this->is_regex = is_regex;
	this->next = NULL;

	this->text = new TCHAR[length + 1];
	memcpy(this->text, str, length * sizeof(TCHAR));

	this->text[length] = 0;

	wcstombs(temp, str, length);

	this->reg = new regex(temp, std::tr1::regex_constants::icase);

	this->style_id = pat_count++ % LP_MAX_STYLE_ID;

	this->count = 0;
}


SearchPattern::~SearchPattern()
{
	if(text){
		delete text;
	}

	if(reg){
		delete reg;
	}
}

TCHAR *SearchPattern::getText()
{
	return this->text;
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

SearchPattern * PAT_Add(TCHAR *str)
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
		pat = new SearchPattern(str, true);
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