#include "synthwave.h"

f32 BALL_VERTS[]={
  -122.551f,-42.667f,-62.608f,
  122.551f,-42.667f,-62.608f,
  0,-42.667f,125.216f,
  0,128,0,
  -64,-66.846f,-98.088f,
  64,-66.846f,-98.088f,
  128,-66.846f,0,
  64,-66.846f,98.088f,
  -128,-66.846f,0,
  -64,-66.846f,98.088f,
  -128,22.282f,-65.392f,
  -64,111.41f,-32.696f,
  128,22.282f,-65.392f,
  64,111.41f,-32.696f,
  0,22.282f,130.784f,
  0,111.41f,65.392f,
  0,-128,0,
  0,42.667f,-125.216f,
  122.551f,42.667f,62.608f,
  -122.551f,42.667f,62.608f
};

u32 BALL_INDEXES[]={
0,4,8,
4,5,16,
16,8,4,
8,16,9,
5,1,6,
6,16,5,
16,6,7,
7,9,16,
9,7,2,
0,10,4,
10,11,17,
17,4,10,
4,17,5,
11,3,13,
13,17,11,
17,13,12,
12,5,17,
5,12,1,
1,12,6,
12,13,18,
18,6,12,
6,18,7,
13,3,15,
15,18,13,
18,15,14,
14,7,18,
7,14,2,
2,14,9,
14,15,19,
19,9,14,
9,19,8,
15,3,11,
11,19,15,
19,11,10,
10,8,19,
8,10,0
};

#define NB_VERTICES  20
#define NB_TRIANGLES 36

void Mesh_MakeBall(Mesh* mesh)
{
  mesh->triangles = (Triangle*) $.Mem.PermaAllocator(NULL, sizeof(Triangle) * NB_TRIANGLES);
  mesh->nbTriangles = NB_TRIANGLES;

  for(u32 ii=0, jj=0;ii < (NB_TRIANGLES * 3);ii+=3,jj++)
  {
    Triangle* triangle = &mesh->triangles[jj];
    for(u32 kk=0;kk < 3;kk++)
    {
      u32 index = BALL_INDEXES[ii + kk];
      triangle->v[kk].x = BALL_VERTS[(index * 3) + 0] / 100.0f;
      triangle->v[kk].y = BALL_VERTS[(index * 3) + 1] / 100.0f;
      triangle->v[kk].z = BALL_VERTS[(index * 3) + 2] / 100.0f;
    }
    triangle->colour = 5;
  }

  $.Mesh.Finalise(mesh);
}
