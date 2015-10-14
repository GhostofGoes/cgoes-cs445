#ifndef _TOKER_H_
#define _TOKER_H_

#include "cminus.h"
#include "types.h"
#include "cminus.tab.h"

int toker(int bisons_value, Kind tok_kind, Type tok_type, char * tok_text, int line_number, ...);

int toker(int num, int ival );
int toker(int num, char * val);
int toker(int num, char ch);
int error(const char * msg);

// Handles string constants
int strconst();

// Handles character constants
int cconst();

#endif
