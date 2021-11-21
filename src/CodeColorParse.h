#ifndef CODE_COLOR_PARSE_H
#define CODE_COLOR_PARSE_H

#include "SyntaxColor.h"

class Funcdata;

R_API RAnnotatedCode *ParseCodeXML(Funcdata *func, const char *xml);

#endif //CODE_COLOR_PARSE_H