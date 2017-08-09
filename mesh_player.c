#include "funk.h"

f32 CAMARO_VERTS[]={
-90,15,-200,
-90,-57,-200,
-90,-57,200,
-90,12,170,
-90,65,-22,
-90,-25,-132,
-90,65,-100,
-90,-35,-164,
-90,25,42,
-90,25,-165,
-90,-25,200,
-90,-45,190,
-90,-35,132,
-90,-65,68,
-90,-65,140,
-90,18,115,
-90,-25,100,
-90,-35,68,
-90,25,-100,
-90,-35,-100,
-90,-65,-164,
-90,-65,-100,
0,-57,200,
0,-25,200,
0,-45,190,
0,-35,132,
0,-65,68,
0,-65,140,
0,-35,68,
0,-25,200,
0,-65,-100,
0,18,120,
0,-35,-164,
0,-35,-100,
0,-65,-164,
0,15,-200,
0,12,170,
0,65,-22,
0,-57,-200,
0,65,-110,
0,25,70,
0,25,-175,
90,15,-200,
90,-57,-200,
90,-57,200,
90,12,170,
90,65,-22,
90,-25,-132,
90,65,-100,
90,-35,-164,
90,25,42,
90,25,-165,
90,-25,200,
90,-45,190,
90,-35,132,
90,-65,68,
90,-65,140,
90,18,115,
90,-25,100,
90,-35,68,
90,25,-100,
90,-35,-100,
90,-65,-164,
90,-65,-100,
};

u32 CAMARO_INDEXES[]={
46,60,48,
50,60,46,
60,51,48,
37,4,40,
4,8,40,
37,40,46,
46,40,50,
4,6,18,
8,4,18,
18,6,9,
9,6,39,
41,9,39,


51,39,48,
41,39,51,
55,59,26,
25,56,27,
52,45,29,
61,55,63,
26,59,28,
51,42,41,
59,61,50,
56,53,44,
24,29,11,
12,14,11,
39,6,37,
20,34,7,
50,61,60,
38,35,43,
12,25,14,
33,63,30,
41,42,35,
31,15,36,
16,12,15,
20,7,1,
0,1,7,
7,5,9,
11,10,3,
24,23,29,
19,18,5,
40,8,31,
1,0,35,
15,3,36,
8,15,31,
6,4,37,
5,18,9,
19,33,21,
29,45,36,
17,16,8,
2,22,11,
11,3,12,
22,24,11,
3,15,12,
29,10,11,
10,29,3,
14,2,11,
17,19,13,
9,0,7,
13,26,17,
25,27,14,
29,36,3,
19,21,13,
26,28,17,
9,41,0,
17,8,19,
33,30,21,
41,35,0,
8,18,19,
38,1,35,
16,15,8,
34,32,7,
59,55,61,
51,49,42,
24,53,29,
54,53,56,
39,37,48,
62,49,34,
58,50,57,
34,49,32,
54,56,25,
22,53,24,
45,54,57,
31,36,57,
58,57,54,
62,43,49,
42,49,43,
49,51,47,
53,45,52,
24,29,23,
61,47,60,
40,31,50,
43,35,42,
57,36,45,
50,31,57,
48,37,46,
47,51,60,
61,63,33,
29,53,52,
59,50,58,
44,53,22,
53,54,45,

};


#define NB_VERTICES  64
#define NB_TRIANGLES 100

void Mesh_MakeRedPlayer(Mesh* mesh)
{
  mesh->triangles = (Triangle*) $.Mem.PermaAllocator(NULL, sizeof(Triangle) * NB_TRIANGLES);
  mesh->nbTriangles = NB_TRIANGLES;
  
  for(u32 ii=0, jj=0;ii < (NB_TRIANGLES * 3);ii+=3,jj++)
  {
    Triangle* triangle = &mesh->triangles[jj];
    for(u32 kk=0;kk < 3;kk++)
    {
      u32 index = CAMARO_INDEXES[ii + kk];
      triangle->v[kk].x = CAMARO_VERTS[(index * 3) + 0] / 100.0f;
      triangle->v[kk].y = CAMARO_VERTS[(index * 3) + 1] / 100.0f;
      triangle->v[kk].z = CAMARO_VERTS[(index * 3) + 2] / 100.0f;
    }
    triangle->colour = ii < (14*3) ? DB16_REGENT_GREY : DB16_FADED_RED;
  }
  
  $.Mesh.Finalise(mesh);
}

void Mesh_MakeBluePlayer(Mesh* mesh)
{
  
  mesh->triangles = (Triangle*) $.Mem.PermaAllocator(NULL, sizeof(Triangle) * NB_TRIANGLES);
  mesh->nbTriangles = NB_TRIANGLES;
  
  for(u32 ii=0, jj=0;ii < (NB_TRIANGLES * 3);ii+=3,jj++)
  {
    Triangle* triangle = &mesh->triangles[jj];
    for(u32 kk=0;kk < 3;kk++)
    {
      u32 index = CAMARO_INDEXES[ii + kk];
      triangle->v[kk].x = CAMARO_VERTS[(index * 3) + 0] / 100.0f;
      triangle->v[kk].y = CAMARO_VERTS[(index * 3) + 1] / 100.0f;
      triangle->v[kk].z = CAMARO_VERTS[(index * 3) + 2] / 100.0f;
    }
    triangle->colour = ii < (14*3) ? DB16_REGENT_GREY : DB16_CADET_BLUE;
  }
  
  $.Mesh.Finalise(mesh);
}
