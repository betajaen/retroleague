#include "funk.h"

f32 WHEEL_VERTS[]={
10,0,0,
-10,-30,0,
-10,-25.981f,15,
-10,-15,25.981f,
-10,0,30,
-10,15,25.981f,
-10,25.981f,15,
-10,30,0,
-10,25.981f,-15,
-10,15,-25.981f,
-10,0,-30,
-10,-15,-25.981f,
-10,-25.981f,-15,
-10,0,0,
10,-30,0,
10,-25.981f,15,
10,-15,25.981f,
10,0,30,
10,15,25.981f,
10,25.981f,15,
10,30,0,
10,25.981f,-15,
10,15,-25.981f,
10,0,-30,
10,-15,-25.981f,
10,-25.981f,-15,
};

u32 WHEEL_INDEXES[]={
1,14,15,
2,15,16,
3,16,17,
4,17,18,
5,18,19,
6,19,20,
7,20,21,
8,21,22,
9,22,23,
10,23,24,
11,24,25,
12,25,14,
2,13,1,
3,13,2,
4,13,3,
5,13,4,
6,13,5,
7,13,6,
8,13,7,
9,13,8,
10,13,9,
11,13,10,
12,13,11,
1,13,12,
14,25,0,
14,0,15,
15,0,16,
16,0,17,
17,0,18,
0,19,18,
0,20,19,
0,21,20,
0,22,21,
0,23,22,
0,24,23,
0,25,24,
1,15,2,
2,16,3,
3,17,4,
4,18,5,
5,19,6,
6,20,7,
7,21,8,
8,22,9,
9,23,10,
10,24,11,
11,25,12,
12,14,1,
};

#define NB_VERTICES  26
#define NB_TRIANGLES 48

void Mesh_MakeWheel(Mesh* mesh)
{
  mesh->triangles = (Triangle*) $.Mem.PermaAllocator(NULL, sizeof(Triangle) * NB_TRIANGLES);
  mesh->nbTriangles = NB_TRIANGLES;

  for(u32 ii=0, jj=0;ii < (NB_TRIANGLES * 3);ii+=3,jj++)
  {
    Triangle* triangle = &mesh->triangles[jj];
    for(u32 kk=0;kk < 3;kk++)
    {
      u32 index = WHEEL_INDEXES[ii + kk];
      triangle->v[kk].x = WHEEL_VERTS[(index * 3) + 0] / 100.0f;
      triangle->v[kk].y = WHEEL_VERTS[(index * 3) + 1] / 100.0f;
      triangle->v[kk].z = WHEEL_VERTS[(index * 3) + 2] / 100.0f;
    }
    triangle->colour = DB16_VERY_DARK_VIOLET;
  }

  $.Mesh.Finalise(mesh);
}
