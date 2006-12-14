#include "data.h"
#include "parser.h"

Object::Object(int objId, LispEntity &le) : id(objId), name(le.getString()) {
}
