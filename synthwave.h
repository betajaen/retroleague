#ifndef SYNTHWAVE_H
#define SYNTHWAVE_H

#include <stdint.h>
#include <stdbool.h>
#include <assert.h>

typedef uint8_t  u8;
typedef uint16_t u16;
typedef uint32_t u32;
typedef uint64_t u64;
typedef int8_t   i8;
typedef int16_t  i16;
typedef int32_t  i32;
typedef int64_t  i64;
typedef float    f32;

typedef struct { i32 x, y;                    } Vec2;

typedef union  {
  struct { f32 x, y, z; };
  struct { f32 RIGHT, UP, FORWARD; };
 } Vec3f;

typedef struct { f32 x, y, z, w;              } Vec4f;
typedef struct { i16 roll, pitch, yaw;        } Rot3i;
typedef struct { u8  r, g, b;                 } Colour;
typedef struct { u64 opaque;                  } Bitmap;
typedef struct { u64 opaque;                  } Surface;
typedef struct { u64 opaque;                  } Canvas;
typedef struct { u64 opaque;                  } Scene;
typedef struct { u64 opaque;                  } Font;
typedef struct { u64 opaque;                  } Sound;
typedef struct { u32 size, capacity;          } ArrayHeader;
typedef struct { u32 start, paused; u8 state; } Timer;
typedef union  { f32 m[4][4]; f32 M[16]; Vec4f row[4];   } Mat44;

typedef struct
{
  Vec3f v[3];
  u8    shader;
  u8    colour;
  u8    p0;
  u8    p1;
} Triangle;

typedef struct
{ 
  Triangle* triangles;
  u16       nbTriangles;
  Vec3f     min, max, centre, halfSize;
  f32       squaredRadius;
} Mesh;

typedef struct Palette_t
{
  u8      transparent;
  u8      numColours;
  Colour  colours[256];
} Palette;

typedef struct
{

  i32         screenX, screenY;
  u32         width, height, displayScale;
  u32         updateMs, drawMs;
  f32         time, fixedDeltaTime, deltaTime;
  u32         frameCount;
  const char* title;
  bool        quit;

  struct
  {
    u32 nbTriangles, nbDrawCalls;
    f32 fps;
  } Stats;

  struct
  {
    void (*Bind)(Palette* palette);
    void (*New)(Palette* palette);
    void (*Delete)(Palette* palette);
    void (*Append)(Palette* palette, Colour* colour);
    void (*AppendRgb)(Palette* palette, u8 r, u8 g, u8 b);
    void (*AppendU32)(Palette* palette, u32);
  } Palette;

  struct
  {
    bool (*Load)(Bitmap* bitmap, const char* name);
    bool (*GetSize)(Bitmap* bitmap, Vec2* outSize);
  } Bitmap;

  struct
  {
    void (*New)(Canvas* canvas);
    void (*Delete)(Canvas* canvas);
    void (*Render)(Canvas* canvas, Surface* surface);
    void (*Clear)(Canvas* canvas, u8 colour);
    void (*DrawBox)(Canvas* canvas, u8 borderColour, Vec2 position, Vec2 size);
    void (*DrawBoxXywh)(Canvas* canvas, u8 borderColour, i16 x, i16 y, i16 w, i16 h);
    void (*DrawFilledBox)(Canvas* canvas, u8 fillColour, Vec2 position, Vec2 size);
    void (*DrawFilledBoxXywh)(Canvas* canvas, u8 fillColour, i16 x, i16 y, i16 w, i16 h);
    void (*DrawLine)(Canvas* canvas, u8 colour, i16 x0, i16 y0, i16 x1, i16 y1);
    void (*DrawPoint)(Canvas* canvas, u8 colour, i16 x, i16 y);
    void (*DrawBitmap)(Canvas* canvas, Bitmap* bitmap, i16 x, i16 y);
    void (*DrawSprite)(Canvas* canvas, Bitmap* bitmap, Vec2 spritePosition, Vec2 spriteSize, Vec2 position);
    void (*DrawSpriteXywh)(Canvas* canvas, Bitmap* bitmap, i16 srcX, i16 srcY, i16 srcW, i16 srcH, i16 dstX, i16 dstY);
    void (*DrawText)(Canvas* canvas, Font* font, u8 c, i16 x, i16 y, const char* text);
    void (*DrawTextF)(Canvas* canvas, Font* font, u8 c, i16 x, i16 y, const char* text, ...);
  } Canvas;

  struct
  {
    void (*New)(Font* font, const char* name, Colour marker, Colour transparent);
  } Font;
  
  struct
  {
    void (*Finalise)(Mesh* mesh);
  } Mesh;

  struct
  {
    void (*New)(Scene* scene);
    void (*Delete)(Scene* scene);
    void (*Render)(Scene* scene, Surface* surface);
    void (*Clear)(Scene* scene, u8 colour);
    void (*SetPovLookAt)(Scene* scene, Vec3f position, Vec3f target);
    void (*SetPovLookAtXyz)(Scene* scene, f32 px, f32 py, f32 pz, f32 tx, f32 ty, f32 tz);
    void (*DrawSkybox)(Scene* scene, u8 sky, u8 ground);
    void (*DrawMesh)(Scene* scene, Mesh* mesh, Vec3f position, Rot3i rotation);
    void (*DrawMeshXyz)(Scene* scene, Mesh* mesh, f32 x, f32 y, f32 z, i16 pitch, i16 yaw, i16 roll);
    void (*DrawCustomShaderMesh)(Scene* scene, Mesh* mesh, u8 shader, Vec3f position, Rot3i rotation);
    void (*DrawCustomShaderMeshXyz)(Scene* scene, Mesh* mesh, u8 shader, f32 x, f32 y, f32 z, i16 pitch, i16 yaw, i16 roll);
  } Scene;

  struct
  {
    void (*New)(Sound* sound, const char* name);
    void (*Play)(Sound* sound);
    void (*MuteAll)();
  } Sound;

  struct
  {
    void (*Play)(const char* name);
  } Music;

  struct
  {
    void (*New)(Timer* timer);
    void (*Delete)(Timer* timer);
    void (*Start)(Timer* timer);
    void (*Stop)(Timer* timer);
    void (*Pause)(Timer* timer);
    void (*Unpause)(Timer* timer);
    u32  (*Ticks)(Timer* timer);
    bool (*IsRunning)(Timer* timer);
    bool (*IsPaused)(Timer* timer);
  } Timer;
  
  struct
  {
    void (*New)(Surface* surface);
    void (*Delete)(Surface* surface);
    void (*Render)(Surface* surface);
  } Surface;

  struct {
    void (*BindControl)(u32 control, i32 key);
    bool (*ControlDown)(u32 control);
    bool (*ControlPressed)(u32 control);
    bool (*ControlReleased)(u32 control);
  } Input;

  struct {
    void* (*PermaAllocator)(void* ptr, u32 nbBytes);
    void* (*TempAllocator)(u32 nbytes);
  } Mem;

  struct {
    bool (*Connect)(const char* address, u16 port);
    bool (*PeekMessage)();
    bool (*RecvMessage)(u32* dataLength, u32 dataCapacity, void* data); 
    void (*SendMessage)(u32 dataLength, void* data);
  } Net;

} Synthwave;

extern Synthwave $;

extern void $Setup();
extern void $Start();
extern void $Update();
extern void $Draw();

#if _DEBUG
#define $Assert(X, REASON) assert(X && REASON)
#define $Ensure(X) assert(X)
#define $DEBUG  1
#else
#define $Assert(X, REASON)
#define $Ensure(X)
#define $DEBUG  0
#endif

#if defined(_MSC_VER)
#define $IsWindows 1
#define $IsBrowser 0
#elif defined(__EMSCRIPTEN__)
#define $IsWindows 0
#define $IsBrowser 1
#endif

#define $Unused(X)              (void)X
#define $Scope(...)             do { __VA_ARGS__ } while(0)
#define $VFn(NAME, ...)         inline void NAME(__VA_ARGS__)
#define $Cast(T)                (T)
#define $Swap(TYPE, X, Y)       $Scope(TYPE TEMP = (X); (X) = (Y); (Y) = (TEMP);)

#define $Min(A, B)              (((A) < (B)) ? (A) : (B))
#define $Max(A, B)              (((A) > (B)) ? (A) : (B))
#define $Min3(A, B, C)          ($Min(A, $Min(B, C)))
#define $Max3(A, B, C)          ($Max(A, $Max(B, C)))
#define $Min4(A, B, C, D)       ($Min(A, $Min(B, $Min(C, D))))
#define $Max4(A, B, C, D)       ($Max(A, $Max(B, $Max(C, D))))
#define $Clamp(X, MIN, MAX)     (X > MAX ? MAX : X < MIN ? MIN : X)
#define $Sign(X)                ((0 < (X)) - ((X) < 0))
#define $SignF(X)               (X < 0.0f ? -1.0f : 1.0f)

#define $PermaNew(TYPE)         ($Cast(TYPE*) $.Mem.PermaAllocator(NULL, sizeof(TYPE)))
#define $PermaDelete(OBJ)       $Scope(($.Mem.PermaAllocator(OBJ, 0)); OBJ = NULL;)

#define $TempNew(TYPE)          ($Cast(TYPE*) $.Mem.TempAllocator(sizeof(TYPE)))
#define $TempNewA(TYPE, N)      ($Cast(TYPE*) $.Mem.TempAllocator(sizeof(TYPE) * N))


#define $PI                     3.14159265358979323846264338327950288f
#define $Rad2Deg(V)             ((V) * 180.0f / $PI)
#define $Deg2Rad(V)             ((V) * $PI   / 180.0f)

inline Colour  $Rgb(u8 r, u8 g, u8 b)  { Colour c; c.r = r; c.g = g; c.b = b; return c;}

#define $CenterInParent(INNER, OUTER)  (((OUTER) / 4) - ((INNER) / 2))

#define $Vec2_Set(V, X,Y)       $Scope({ (V)->x  = (X); (V)->y  = (Y); })
#define $Vec2_SetX(V, X)        $Scope({ (V)->x  = (X); })
#define $Vec2_SetY(V, Y)        $Scope({ (V)->y  = (Y); })
#define $Vec2_AddXyz(V, X,Y)    $Scope({ (V)->x += (X); (V)->y += (Y); })
#define $Vec2_SubXyz(V, X,Y)    $Scope({ (V)->x -= (X); (V)->y -= (Y); })
#define $Vec2_MulXyz(V, X,Y)    $Scope({ (V)->x *= (X); (V)->y *= (Y); })
#define $Vec2_DivXyz(V, X,Y)    $Scope({ (V)->x /= (X); (V)->y /= (Y); })
#define $Vec2_AddS(V, S)        $Scope({ (V)->x += (S); (V)->y += (S); })
#define $Vec2_SubS(V, S)        $Scope({ (V)->x -= (S); (V)->y -= (S); })
#define $Vec2_MulS(V, S)        $Scope({ (V)->x *= (S); (V)->y *= (S); })
#define $Vec2_DivS(V, S)        $Scope({ (V)->x /= (S); (V)->y /= (S); })
#define $Vec2_Add(V, A,B)       $Scope({ (V)->x = (A)->x + (B)->x; (V)->y = (A)->y + (B)->y; })
#define $Vec2_Sub(V, A,B)       $Scope({ (V)->x = (A)->x - (B)->x; (V)->y = (A)->y + (B)->y; })
#define $Vec2_Mul(V, A,B)       $Scope({ (V)->x = (A)->x * (B)->x; (V)->y = (A)->y * (B)->y; })
#define $Vec2_Div(V, A,B)       $Scope({ (V)->x = (A)->x / (B)->x; (V)->y = (A)->y / (B)->y; })

$VFn($Vec3_Set,  Vec3f* v, f32 x, f32 y, f32 z) { v->x = x; v->y = y; v->z = z; }
$VFn($Vec3_SetX, Vec3f* v, f32 x)               { v->x = x; }
$VFn($Vec3_SetY, Vec3f* v, f32 y)               { v->y = y; }
$VFn($Vec3_SetZ, Vec3f* v, f32 z)               { v->z = z; }

inline Vec3f $Vec3_Xyz(f32 x, f32 y, f32 z)
{
  Vec3f m; m.x = x; m.y = y; m.z = z; return m;
}

inline Vec3f $Vec3_Add(Vec3f a, Vec3f b) { return $Vec3_Xyz(a.x + b.x, a.y + b.y, a.z + b.z); }
inline Vec3f $Vec3_Sub(Vec3f a, Vec3f b) { return $Vec3_Xyz(a.x - b.x, a.y - b.y, a.z - b.z); }
inline Vec3f $Vec3_Mul(Vec3f a, Vec3f b) { return $Vec3_Xyz(a.x * b.x, a.y * b.y, a.z * b.z); }
inline Vec3f $Vec3_Div(Vec3f a, Vec3f b) { return $Vec3_Xyz(a.x / b.x, a.y / b.y, a.z / b.z); }
inline Vec3f $Vec3_MulS(Vec3f a, f32 s)  { return $Vec3_Xyz(a.x * s, a.y * s, a.z * s); }
inline Vec3f $Vec3_DivS(Vec3f a, f32 s)  { return $Vec3_Xyz(a.x / s, a.y / s, a.z / s); }

Vec3f $Vec3_Normalise(Vec3f v);
inline f32   $Vec3_Dot(Vec3f a, Vec3f b)  { return a.x * b.x + a.y * b.y + a.z * b.z;          }
Vec3f $Vec3_Cross(Vec3f a, Vec3f b);
f32   $Vec3_Length(Vec3f v);
f32   $Vec3_Length2(Vec3f v);

#define $Vec4_Set(V, X,Y,Z,W)   $Scope({ (V)->x  = (X); (V)->y  = (Y); (V)->z  = (Z);  (V)->w  = (W);})

void $Vec3_Transform(Vec3f* v, Rot3i* b);
void $Vec3_TransformPitch(Vec3f* v, i16 pitch);
void $Vec3_TransformYaw(Vec3f* v, i16 yaw);
void $Vec3_TransformRoll(Vec3f* v, i16 roll);
#define $Vec3_TransformRotX $Vec3_TransformPitch
#define $Vec3_TransformRotY $Vec3_TransformYaw
#define $Vec3_TransformRotZ $Vec3_TransformRoll

void $Vec3_InvTransform(Vec3f* v, Rot3i* b);
void $Vec3_InvTransformPitch(Vec3f* v, i16 pitch);
void $Vec3_InvTransformYaw(Vec3f* v, i16 yaw);
void $Vec3_InvTransformRoll(Vec3f* v,  i16 roll);
#define $Vec3_InvTransformRotX $Vec3_InvTransformPitch
#define $Vec3_InvTransformRotY $Vec3_InvTransformYaw
#define $Vec3_InvTransformRotZ $Vec3_InvTransformRoll

void $Mat44_Identity(Mat44* self);
void $Mat44_Multiply(Mat44* m, Mat44* a, Mat44* b);
void $Mat44_LookAt(Mat44* m, Vec3f pos, Vec3f target);
void $Mat44_MultiplyVec4(Vec4f* out, Mat44* m, Vec4f* v);
void $Mat44_Inverse(Mat44* a, Mat44* m);
void $Mat44_RotMatrixX(Mat44* m, f32 xDeg);
void $Mat44_RotMatrixY(Mat44* m, f32 yDeg);
void $Mat44_RotMatrixZ(Mat44* m, f32 zDeg);
void $Mat44_RotMatrixZXY(Mat44* m, Rot3i rot);
void $Mat44_MultiplyTransform(Mat44* m, Vec3f t);

#define $Array(TYPE) TYPE*
#define $Array_Header(A)                ($Cast(ArrayHeader*)(A) - 1)
#define $Array_Size(A)                  ($Array_Header(A)->size)
#define $Array_Capacity(A)              ($Array_Header(A)->capacity)

#define $Array_New(A, CAPACITY)\
    $Scope({                                                                            \
      void** AP = $Cast(void**)(&(A));                                                  \
      ArrayHeader* AH = $Cast(ArrayHeader*) $.Mem.PermaAllocator(NULL, sizeof(*(A)) * CAPACITY);  \
      AH->size = 0; AH->capacity = CAPACITY; (*AP) = $Cast(void*)(AH + 1);              \
    })

#define $Array_Delete(A)\
    $Scope( $.Mem.PermaAllocator($Array_Header(A), 0); )

#define $Array_SetCapacity(A, NEW_CAPACITY)\
    $Scope({                                                         \
      void** AP = $Cast(void**)&(A);                                 \
      ArrayHeader* AH = $.Mem.PermaAllocator($Array_Header(A), sizeof(*(A)) * NEW_CAPACITY); \
      AH->capacity = NEW_CAPACITY;                                   \
      AH->size = $Min(AH->size, AH->capacity);                       \
      (*AP) = $Cast(void*)(AH + 1);                                  \
    })

#define $Array_Grow(A, MIN_CAPACITY)\
    $Scope({                                                      \
      u32 _newCapacity = (8 + ($Array_Capacity(A) * 2));          \
      if (_newCapacity < MIN_CAPACITY)\
        _newCapacity = MIN_CAPACITY;                            \
      $Array_SetCapacity(A, _newCapacity);                 \
    })

#define $Array_Clear(A)\
    $Scope($Array_Header(A)->size = 0;)

#define $Array_Push(A, V)\
    $Scope({                                          \
      if ($Array_Capacity(A) < ($Array_Size(A) + 1))  \
        $Array_Grow(A, 0);                      \
      (A)[$Array_Size(A)] = (ITEM);                   \
      $Array_Header(A)->size++;                       \
    });

#define $Array_Pop(A)\
    $Scope({                                          \
      $Assert($Array_Size(A) > 0)                     \
      $Array_Header(A)->size--;                       \
    });
    
#define $Array_PushAndFillOut(A, OUT_PTR)\
    $Scope({                                          \
      if ($Array_Capacity(A) < ($Array_Size(A) + 1))  \
      {\
        $Array_Grow(A, 0);                      \
      }\
      OUT_PTR = &((A)[$Array_Size(A)]);               \
      $Array_Header(A)->size++;                       \
    });

#define $Array_RemoveAt(A, INDEX) \
    $Scope({                                          \
      ((A)[INDEX]) = ((A)[$Array_Size(A)] - 1);       \
      $Array_Header(A)->size--;                       \
    });

typedef enum
{
    $KEY_UNKNOWN = 0,

    $KEY_A = 4,
    $KEY_B = 5,
    $KEY_C = 6,
    $KEY_D = 7,
    $KEY_E = 8,
    $KEY_F = 9,
    $KEY_G = 10,
    $KEY_H = 11,
    $KEY_I = 12,
    $KEY_J = 13,
    $KEY_K = 14,
    $KEY_L = 15,
    $KEY_M = 16,
    $KEY_N = 17,
    $KEY_O = 18,
    $KEY_P = 19,
    $KEY_Q = 20,
    $KEY_R = 21,
    $KEY_S = 22,
    $KEY_T = 23,
    $KEY_U = 24,
    $KEY_V = 25,
    $KEY_W = 26,
    $KEY_X = 27,
    $KEY_Y = 28,
    $KEY_Z = 29,

    $KEY_1 = 30,
    $KEY_2 = 31,
    $KEY_3 = 32,
    $KEY_4 = 33,
    $KEY_5 = 34,
    $KEY_6 = 35,
    $KEY_7 = 36,
    $KEY_8 = 37,
    $KEY_9 = 38,
    $KEY_0 = 39,

    $KEY_RETURN = 40,
    $KEY_ESCAPE = 41,
    $KEY_BACKSPACE = 42,
    $KEY_TAB = 43,
    $KEY_SPACE = 44,

    $KEY_MINUS = 45,
    $KEY_EQUALS = 46,
    $KEY_LEFTBRACKET = 47,
    $KEY_RIGHTBRACKET = 48,
    $KEY_BACKSLASH = 49, /**< Located at the lower left of the return
                                  *   key on ISO keyboards and at the right end
                                  *   of the QWERTY row on ANSI keyboards.
                                  *   Produces REVERSE SOLIDUS (backslash) and
                                  *   VERTICAL LINE in a US layout, REVERSE
                                  *   SOLIDUS and VERTICAL LINE in a UK Mac
                                  *   layout, NUMBER SIGN and TILDE in a UK
                                  *   Windows layout, DOLLAR SIGN and POUND SIGN
                                  *   in a Swiss German layout, NUMBER SIGN and
                                  *   APOSTROPHE in a German layout, GRAVE
                                  *   ACCENT and POUND SIGN in a French Mac
                                  *   layout, and ASTERISK and MICRO SIGN in a
                                  *   French Windows layout.
                                  */
    $KEY_NONUSHASH = 50, /**< ISO USB keyboards actually use this code
                                  *   instead of 49 for the same key, but all
                                  *   OSes I've seen treat the two codes
                                  *   identically. So, as an implementor, unless
                                  *   your keyboard generates both of those
                                  *   codes and your OS treats them differently,
                                  *   you should generate $KEY_BACKSLASH
                                  *   instead of this code. As a user, you
                                  *   should not rely on this code because SDL
                                  *   will never generate it with most (all?)
                                  *   keyboards.
                                  */
    $KEY_SEMICOLON = 51,
    $KEY_APOSTROPHE = 52,
    $KEY_GRAVE = 53, /**< Located in the top left corner (on both ANSI
                              *   and ISO keyboards). Produces GRAVE ACCENT and
                              *   TILDE in a US Windows layout and in US and UK
                              *   Mac layouts on ANSI keyboards, GRAVE ACCENT
                              *   and NOT SIGN in a UK Windows layout, SECTION
                              *   SIGN and PLUS-MINUS SIGN in US and UK Mac
                              *   layouts on ISO keyboards, SECTION SIGN and
                              *   DEGREE SIGN in a Swiss German layout (Mac:
                              *   only on ISO keyboards), CIRCUMFLEX ACCENT and
                              *   DEGREE SIGN in a German layout (Mac: only on
                              *   ISO keyboards), SUPERSCRIPT TWO and TILDE in a
                              *   French Windows layout, COMMERCIAL AT and
                              *   NUMBER SIGN in a French Mac layout on ISO
                              *   keyboards, and LESS-THAN SIGN and GREATER-THAN
                              *   SIGN in a Swiss German, German, or French Mac
                              *   layout on ANSI keyboards.
                              */
    $KEY_COMMA = 54,
    $KEY_PERIOD = 55,
    $KEY_SLASH = 56,

    $KEY_CAPSLOCK = 57,

    $KEY_F1 = 58,
    $KEY_F2 = 59,
    $KEY_F3 = 60,
    $KEY_F4 = 61,
    $KEY_F5 = 62,
    $KEY_F6 = 63,
    $KEY_F7 = 64,
    $KEY_F8 = 65,
    $KEY_F9 = 66,
    $KEY_F10 = 67,
    $KEY_F11 = 68,
    $KEY_F12 = 69,

    $KEY_PRINTSCREEN = 70,
    $KEY_SCROLLLOCK = 71,
    $KEY_PAUSE = 72,
    $KEY_INSERT = 73, /**< insert on PC, help on some Mac keyboards (but
                                   does send code 73, not 117) */
    $KEY_HOME = 74,
    $KEY_PAGEUP = 75,
    $KEY_DELETE = 76,
    $KEY_END = 77,
    $KEY_PAGEDOWN = 78,
    $KEY_RIGHT = 79,
    $KEY_LEFT = 80,
    $KEY_DOWN = 81,
    $KEY_UP = 82,

    $KEY_NUMLOCKCLEAR = 83, /**< num lock on PC, clear on Mac keyboards
                                     */
    $KEY_KP_DIVIDE = 84,
    $KEY_KP_MULTIPLY = 85,
    $KEY_KP_MINUS = 86,
    $KEY_KP_PLUS = 87,
    $KEY_KP_ENTER = 88,
    $KEY_KP_1 = 89,
    $KEY_KP_2 = 90,
    $KEY_KP_3 = 91,
    $KEY_KP_4 = 92,
    $KEY_KP_5 = 93,
    $KEY_KP_6 = 94,
    $KEY_KP_7 = 95,
    $KEY_KP_8 = 96,
    $KEY_KP_9 = 97,
    $KEY_KP_0 = 98,
    $KEY_KP_PERIOD = 99,

    $KEY_NONUSBACKSLASH = 100, /**< This is the additional key that ISO
                                        *   keyboards have over ANSI ones,
                                        *   located between left shift and Y.
                                        *   Produces GRAVE ACCENT and TILDE in a
                                        *   US or UK Mac layout, REVERSE SOLIDUS
                                        *   (backslash) and VERTICAL LINE in a
                                        *   US or UK Windows layout, and
                                        *   LESS-THAN SIGN and GREATER-THAN SIGN
                                        *   in a Swiss German, German, or French
                                        *   layout. */
    $KEY_APPLICATION = 101, /**< windows contextual menu, compose */
    $KEY_POWER = 102, /**< The USB document says this is a status flag,
                               *   not a physical key - but some Mac keyboards
                               *   do have a power key. */
    $KEY_KP_EQUALS = 103,
    $KEY_F13 = 104,
    $KEY_F14 = 105,
    $KEY_F15 = 106,
    $KEY_F16 = 107,
    $KEY_F17 = 108,
    $KEY_F18 = 109,
    $KEY_F19 = 110,
    $KEY_F20 = 111,
    $KEY_F21 = 112,
    $KEY_F22 = 113,
    $KEY_F23 = 114,
    $KEY_F24 = 115,
    $KEY_EXECUTE = 116,
    $KEY_HELP = 117,
    $KEY_MENU = 118,
    $KEY_SELECT = 119,
    $KEY_STOP = 120,
    $KEY_AGAIN = 121,   /**< redo */
    $KEY_UNDO = 122,
    $KEY_CUT = 123,
    $KEY_COPY = 124,
    $KEY_PASTE = 125,
    $KEY_FIND = 126,
    $KEY_MUTE = 127,
    $KEY_VOLUMEUP = 128,
    $KEY_VOLUMEDOWN = 129,
/* not sure whether there's a reason to enable these */
/*     $KEY_LOCKINGCAPSLOCK = 130,  */
/*     $KEY_LOCKINGNUMLOCK = 131, */
/*     $KEY_LOCKINGSCROLLLOCK = 132, */
    $KEY_KP_COMMA = 133,
    $KEY_KP_EQUALSAS400 = 134,

    $KEY_INTERNATIONAL1 = 135, /**< used on Asian keyboards, see
                                            footnotes in USB doc */
    $KEY_INTERNATIONAL2 = 136,
    $KEY_INTERNATIONAL3 = 137, /**< Yen */
    $KEY_INTERNATIONAL4 = 138,
    $KEY_INTERNATIONAL5 = 139,
    $KEY_INTERNATIONAL6 = 140,
    $KEY_INTERNATIONAL7 = 141,
    $KEY_INTERNATIONAL8 = 142,
    $KEY_INTERNATIONAL9 = 143,
    $KEY_LANG1 = 144,
    $KEY_LANG2 = 145,
    $KEY_LANG3 = 146,
    $KEY_LANG4 = 147,
    $KEY_LANG5 = 148,
    $KEY_LANG6 = 149,
    $KEY_LANG7 = 150,
    $KEY_LANG8 = 151,
    $KEY_LANG9 = 152,

    $KEY_ALTERASE = 153,
    $KEY_SYSREQ = 154,
    $KEY_CANCEL = 155,
    $KEY_CLEAR = 156,
    $KEY_PRIOR = 157,
    $KEY_RETURN2 = 158,
    $KEY_SEPARATOR = 159,
    $KEY_OUT = 160,
    $KEY_OPER = 161,
    $KEY_CLEARAGAIN = 162,
    $KEY_CRSEL = 163,
    $KEY_EXSEL = 164,

    $KEY_KP_00 = 176,
    $KEY_KP_000 = 177,
    $KEY_THOUSANDSSEPARATOR = 178,
    $KEY_DECIMALSEPARATOR = 179,
    $KEY_CURRENCYUNIT = 180,
    $KEY_CURRENCYSUBUNIT = 181,
    $KEY_KP_LEFTPAREN = 182,
    $KEY_KP_RIGHTPAREN = 183,
    $KEY_KP_LEFTBRACE = 184,
    $KEY_KP_RIGHTBRACE = 185,
    $KEY_KP_TAB = 186,
    $KEY_KP_BACKSPACE = 187,
    $KEY_KP_A = 188,
    $KEY_KP_B = 189,
    $KEY_KP_C = 190,
    $KEY_KP_D = 191,
    $KEY_KP_E = 192,
    $KEY_KP_F = 193,
    $KEY_KP_XOR = 194,
    $KEY_KP_POWER = 195,
    $KEY_KP_PERCENT = 196,
    $KEY_KP_LESS = 197,
    $KEY_KP_GREATER = 198,
    $KEY_KP_AMPERSAND = 199,
    $KEY_KP_DBLAMPERSAND = 200,
    $KEY_KP_VERTICALBAR = 201,
    $KEY_KP_DBLVERTICALBAR = 202,
    $KEY_KP_COLON = 203,
    $KEY_KP_HASH = 204,
    $KEY_KP_SPACE = 205,
    $KEY_KP_AT = 206,
    $KEY_KP_EXCLAM = 207,
    $KEY_KP_MEMSTORE = 208,
    $KEY_KP_MEMRECALL = 209,
    $KEY_KP_MEMCLEAR = 210,
    $KEY_KP_MEMADD = 211,
    $KEY_KP_MEMSUBTRACT = 212,
    $KEY_KP_MEMMULTIPLY = 213,
    $KEY_KP_MEMDIVIDE = 214,
    $KEY_KP_PLUSMINUS = 215,
    $KEY_KP_CLEAR = 216,
    $KEY_KP_CLEARENTRY = 217,
    $KEY_KP_BINARY = 218,
    $KEY_KP_OCTAL = 219,
    $KEY_KP_DECIMAL = 220,
    $KEY_KP_HEXADECIMAL = 221,

    $KEY_LCTRL = 224,
    $KEY_LSHIFT = 225,
    $KEY_LALT = 226,
    $KEY_LGUI = 227,
    $KEY_RCTRL = 228,
    $KEY_RSHIFT = 229,
    $KEY_RALT = 230,
    $KEY_RGUI = 231,

    $KEY_MODE = 257,

    $KEY_AUDIONEXT = 258,
    $KEY_AUDIOPREV = 259,
    $KEY_AUDIOSTOP = 260,
    $KEY_AUDIOPLAY = 261,
    $KEY_AUDIOMUTE = 262,
    $KEY_MEDIASELECT = 263,
    $KEY_WWW = 264,
    $KEY_MAIL = 265,
    $KEY_CALCULATOR = 266,
    $KEY_COMPUTER = 267,
    $KEY_AC_SEARCH = 268,
    $KEY_AC_HOME = 269,
    $KEY_AC_BACK = 270,
    $KEY_AC_FORWARD = 271,
    $KEY_AC_STOP = 272,
    $KEY_AC_REFRESH = 273,
    $KEY_AC_BOOKMARKS = 274,

    $KEY_BRIGHTNESSDOWN = 275,
    $KEY_BRIGHTNESSUP = 276,
    $KEY_DISPLAYSWITCH = 277,

    $KEY_KBDILLUMTOGGLE = 278,
    $KEY_KBDILLUMDOWN = 279,
    $KEY_KBDILLUMUP = 280,
    $KEY_EJECT = 281,
    $KEY_SLEEP = 282,

    $KEY_APP1 = 283,
    $KEY_APP2 = 284,

    $KEY_NUM_SCANCODES = 512 /**< not a key, just marks the number of scancodes
                                 for array bounds */
} $Key;

#endif
