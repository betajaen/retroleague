#include "funk.h"


bool Object_IsAlive(Object* obj)
{
  return obj->type != OT_NONE;
}
