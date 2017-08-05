#ifndef FUNK_H
#define FUNK_H

#include "data.h"

bool Object_IsAlive(Object* obj);

void Mesh_MakePlayer(Mesh* mesh);
void Mesh_MakeBall(Mesh* mesh);


Vec3f RotatePointXZ(Vec3f p, f32 yaw);
f32 ConstrainAngle(f32 x);


#endif

