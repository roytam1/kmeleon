#ifndef PARSER_H
#define PARSER_H

#define ENABLE_LOG
#include "Log.h"

class CParser {
protected:
   DEFINE_LOG

	virtual int Parse(char *p) = 0;

   char *nextoken;
   char *strtok (char * string, const char * control);

public:
	int Load(CString &filename);

};

#endif