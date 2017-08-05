#include "synthwave.h"

#define TRI(V0_X, V0_Y, V0_Z, V1_X, V1_Y, V1_Z, V2_X, V2_Y, V2_Z, COLOUR) { .v[0] = {V0_X,V0_Y,V0_Z}, .v[1] = {V1_X,V1_Y,V1_Z}, .v[2] = {V2_X,V2_Y,V2_Z}, .shader = 0, .colour = COLOUR, .p0 = 0, .p1 = 0}

#define B 0.25

Triangle box[12] = {
    TRI(
      -B,-B,-B,
      -B,-B, B,
      -B, B, B,
    1),
    
    TRI(
       B, B,-B, 
      -B,-B,-B,
      -B, B,-B, 
    2),

    TRI(
       B,-B, B,
      -B,-B,-B,
       B,-B,-B,
    3),

    TRI(
       B, B,-B,
       B,-B,-B,
      -B,-B,-B,
    4),

    TRI(
      -B,-B,-B,
      -B, B, B,
      -B, B,-B,
    5),

    TRI(
       B,-B, B,
      -B,-B, B,
      -B,-B,-B,
    6),

    TRI(
      -B, B, B,
      -B,-B, B,
       B,-B, B,
    7),
    
    TRI(
       B, B, B,
       B,-B,-B,
       B, B,-B,
    8),
    
    TRI(
       B,-B,-B,
       B, B, B,
       B,-B, B,
    9),
    TRI(
       B, B, B,
       B, B,-B,
      -B, B,-B,
    10),
    TRI(
       B, B, B,
      -B, B,-B,
      -B, B, B,
    11),
    TRI(
       B, B, B,
      -B, B, B,
       B,-B, B,
    12)
};


void Mesh_MakeBall(Mesh* mesh)
{
  mesh->triangles = &box[0];
  mesh->nbTriangles = 12;
  $.Mesh.Finalise(mesh);
}
