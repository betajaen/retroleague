#ifndef FUNK_H
#define FUNK_H

#include "data.h"

bool Object_IsAlive(Object* obj);

void Mesh_MakePlayer(Mesh* mesh);
void Mesh_MakeBall(Mesh* mesh);

f32 constrainAngle(f32 x);

#endif

