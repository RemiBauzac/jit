#ifndef __MARSHAL_H__
#define __MARSHAL_H__

#include "lang.h"

void marshal(FILE *out, function *f);
function *unmarshal(char *filename);
#endif
