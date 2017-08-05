#define LODEPNG_NO_COMPILE_ENCODER
#define $EnsureOpaque(OPAQUE) $Ensure(OPAQUE); $Ensure(OPAQUE->opaque);
#define $$CastOpaque(TYPE, N) ($Cast(TYPE*)(N->opaque))
#define $$AUDIO_INSTANCES 16
#define $$AUDIO_FREQUENCY 48000
#define $$AUDIO_CHANNELS 2
#define $$AUDIO_SAMPLES 16384
#define $NET_MAX_PACKET_COUNT 16
#define $NET_MAX_PACKET_SIZE 512


#define $AUDIO_ENABLED 1
#define $MUSIC_ENABLED 1
#define $NETWORK_ENABLED 1

#include "synthwave.h"
#include <math.h>
#include <malloc.h>
#include <string.h>
#include <stdio.h>

#if $IsWindows == 1
  #include <windows.h>
  #undef DrawText
  #undef SendMessage
  #undef PeekMessage
  #undef far
  #undef near
#elif $IsBrowser == 1
  #include <emscripten.h>
#endif

#include <SDL.h>
#include <SDL_net.h>

#include "ref/lodepng.c"

#if $MUSIC_ENABLED == 1
#include "ref/micromod.c"
#include "ref/micromod_sdl.c"
#endif

Synthwave $;

#define SDC_None 0
#define SDC_DrawModel 1
#define SDC_DrawSkybox 2

typedef struct
{
  u32 type;
  union
  {
    struct { Mesh* mesh; Vec3f position; Rot3i rotation; u8 shader; } drawModel;
    struct { u8 sky, ground; } drawSkyBox;
  };
} $$SceneDrawCmd;

typedef struct
{
  i32  width, height;
  f32  halfWidth, halfHeight;
  u8*  colour;
  f32* depth;

  u8*  colourMem;
  f32* depthMem;
  SDL_Texture* texture;
} $$FrameBuffer;

typedef struct
{
  $$SceneDrawCmd* drawCmds;
  u8              background;
  Mat44           vpsMatrix, screenMatrix, projectionMatrix, viewMatrix;
  f32             skyboxY;
  Vec3f           cameraPosition, cameraTarget;
  bool            cameraOutOfDate;
  Vec4f           frustrum[6];
  u8              lightAmbientColour;
  u8              lightDirectionalColour;
  Rot3i           lightDirectionalDirection;
  $$FrameBuffer*  frameBuffer;
} $$Scene;

typedef struct
{
  SDL_Texture* texture;
} $$Surface;

typedef struct
{
  u32 w, h;
  SDL_Texture* texture;
} $$Bitmap;

typedef struct
{
  SDL_Texture* texture;
  u8  height;
  u16 x[256];
  u8  width[256];
} $$Font;

#if $AUDIO_ENABLED == 1
typedef struct
{
  i32 length;
  u8* buffer;
  SDL_AudioSpec spec;
} $$Sound;

typedef struct
{
  $$Sound* sound;
  i32      position;
  u8       volume;
} $$SoundInstance;

#endif

typedef struct
{
  SDL_Keycode key;
  i32         control;
  i32         lastState, state;
} $$Control;

#define CDC_None     0
#define CDC_DrawQuad 1
#define CDC_DrawQuadNullDst 2
#define CDC_DrawBox  3
#define CDC_DrawLine 4
#define CDC_DrawPoint 5
#define CDC_DrawText 6

typedef struct
{
  u32 type;
  union
  {
    struct { SDL_Texture* texture; SDL_Rect src, dst; } drawQuad;
    struct { SDL_Texture* texture; SDL_Rect dst;      } drawQuadNullSrc;
    struct { SDL_Rect dst; u8 b, f;                   } drawBox;
    struct { i16 x0, y0, x1, y1; u8 c;                } drawLine;
    struct { i16 x, y; u8 c;                          } drawPoint;
    struct { $$Font* font; const char* text; i16 x, y; u8 c;  } drawText;
  };
} $$CanvasDrawCmd;

typedef struct
{
  $$CanvasDrawCmd* drawCmds;
  u8               background;
} $$Canvas;

typedef struct
{
  SDL_Window*           window;
  SDL_Renderer*         renderer;
  u32                   deltaTimeMs, accumulator;
  $$Scene*              scenes;
  $$Canvas*             canvases;
  $$Bitmap*             bitmaps;
  $$Surface*            surfaces;
  $$Control*            controls;
#if $AUDIO_ENABLED == 1
  $$Sound*              sounds;
  $$SoundInstance       soundInstances[$$AUDIO_INSTANCES];
#endif
  SDL_AudioSpec         audioSpec;
#if $MUSIC_ENABLED == 1
  micromod_sdl_context* musicContext;
#endif
#if $NETWORK_ENABLED == 1
  u8                    netId;
  TCPsocket             netSocket;
  SDLNet_SocketSet      netSocketSet;
#endif
  Palette*              palette;
  Timer                 fpsTimer, frameLimitTimer, deltaTimer;
} SynthwaveInternal;

static SynthwaveInternal $$;

void Palette_Bind(Palette* palette)
{
  $Ensure(palette);
  memcpy(&$$.palette, palette, sizeof(Palette));
}

void Palette_New(Palette* palette)
{
  $Ensure(palette);
  memset(palette->colours, 0, sizeof(palette));
}

void Palette_Delete(Palette* palette)
{
  $Ensure(palette);
  memset(palette->colours, 0, sizeof(palette));
}

void Palette_Append(Palette* palette, Colour* colour)
{
  $Ensure(palette);
  $Ensure(colour);
  $Assert(palette->numColours <= 255, "Maximum number of colours exceeded");
  palette->colours[palette->numColours++] = *colour;
}

void Palette_AppendRgb(Palette* palette, u8 r, u8 g, u8 b)
{
  Colour col = {.r = r, .g = g, .b = b};
  Palette_Append(palette, &col);
}

void Palette_AppendU32(Palette* palette, u32 k)
{
  Colour col;
  col.b = k & 0xFF;
  k >>= 8;
  col.g = k & 0xFF;
  k >>= 8;
  col.r = k & 0xFF;
  k >>= 8;
  
  Palette_Append(palette, &col);
}
static void* Resource_Load(const char* name, u32* outSize)
{
#if $IsWindows == 1
  assert(outSize);

  HRSRC handle = FindResource(0, name, "RESOURCE");
  assert(handle);

  HGLOBAL data = LoadResource(0, handle);
  assert(data);

  void* ptr = LockResource(data);
  assert(ptr);

  DWORD dataSize = SizeofResource(0, handle);
  assert(dataSize);

  (*outSize) = dataSize;

  return ptr;
#elif $IsBrowser == 1
  $Unused(name);
  $Unused(outSize);
  return NULL;
#endif
}

bool Bitmap_Load(Bitmap* bitmap, const char* name)
{
  $Ensure(bitmap);
  $$Bitmap* b = $PermaNew($$Bitmap);
  bitmap->opaque = $Cast(u64) b;

  u32 width, height;
  u8* imageData = NULL;

  
#if $IsWindows == 1
  u32 resourceSize = 0;
  void* resourceData = Resource_Load(name, &resourceSize);
  lodepng_decode_memory(&imageData, &width, &height, resourceData, resourceSize, LCT_RGB, 8);
#elif $IsBrowser == 1
  char n[256];
  n[0] = 0;
  strcat(&n[0], "assets/");
  strcat(&n[0], name);
  printf("Loading Bitmap for Bitmap: %s\n", n);
  lodepng_decode_file(&imageData, &width, &height, n, LCT_RGB, 8);
#endif

  assert(imageData);

  SDL_Texture* texture = SDL_CreateTexture($$.renderer, SDL_PIXELFORMAT_ABGR8888, SDL_TEXTUREACCESS_STREAMING, width, height);

  SDL_SetTextureBlendMode(texture, SDL_BLENDMODE_BLEND);
  void* pixelsVoid;
  int pitch;
  SDL_LockTexture(texture, NULL, &pixelsVoid, &pitch);
  u8* pixels = (u8*) pixelsVoid;
  
  Palette* palette = $$.palette;
  
  for(u32 i=0, j=0;i < (width * height * 3);i+=3, j+=4)
  {
    Colour col;
    col.r = imageData[i+0];
    col.g = imageData[i+1];
    col.b = imageData[i+2];

    int bestIndex = 0x100;
    int bestDistance = 10000000;
    
    // Match nearest colour by using a treating the two colours as vectors, and matching against the closest distance between the two.
    for (u32 k=0;k < palette->numColours;k++)
    {
      Colour pal = palette->colours[k];

      int distance = ((col.r - pal.r) * (col.r - pal.r)) + 
                     ((col.g - pal.g) * (col.g - pal.g)) + 
                     ((col.b - pal.b) * (col.b - pal.b));
      
      if (distance < bestDistance)
      {
        bestDistance = distance;
        bestIndex = k;
      }
    }

    if (bestIndex == 0x100)
      bestIndex = 1;
    
    Colour bestColour = palette->colours[bestIndex];
    
    pixels[j+0] = bestColour.r;
    pixels[j+1] = bestColour.g;
    pixels[j+2] = bestColour.b;
    pixels[j+3] = bestIndex == 0 ? 0 : 255; 
  }

  SDL_UnlockTexture(texture);

  b->w = width;
  b->h = height;
  b->texture = texture;
  return true;
}

bool Bitmap_GetSize(Bitmap* bitmap, Vec2* outSize)
{
  $Ensure(bitmap);
  $Ensure(outSize);

  $$Bitmap* b = $$CastOpaque($$Bitmap, bitmap);

  outSize->x = b->w;
  outSize->y = b->h;

  return true;
}

void Surface_New(Surface* surface)
{
  $Ensure(surface);

  $$Surface* s = $PermaNew($$Surface);
  surface->opaque = $Cast(u64) s;
  
  s->texture = SDL_CreateTexture(
    $$.renderer,
    SDL_PIXELFORMAT_ABGR8888,
    SDL_TEXTUREACCESS_TARGET,
    $.width, $.height
  );
  
  SDL_SetTextureBlendMode(s->texture, SDL_BLENDMODE_BLEND);
}

void Surface_Delete(Surface* surface)
{
  $Ensure(surface);

  $$Surface* s = $$CastOpaque($$Surface, surface);
  SDL_DestroyTexture(s->texture);

  $PermaDelete(s);
  surface->opaque = 0;
}

void Surface_Render(Surface* surface)
{
  $Ensure(surface);

  $$Surface* s = $$CastOpaque($$Surface, surface);

  SDL_RenderCopy($$.renderer, s->texture, NULL, NULL);
}

void Canvas_New(Canvas* canvas)
{
  $Ensure(canvas);
  
  $$Canvas* c = $PermaNew($$Canvas);
  canvas->opaque = $Cast(u64) c;

  $Array_New(c->drawCmds, 64);
  c->background = 0;
}

void Canvas_Delete(Canvas* canvas)
{
  $Ensure(canvas);

  $$Canvas* c = $$CastOpaque($$Canvas, canvas);
  
  $Array_Delete(c->drawCmds);
  $PermaDelete(c);
  canvas->opaque = 0;
}

void Canvas_Render(Canvas* canvas, Surface* surface)
{
  $EnsureOpaque(canvas);
  $EnsureOpaque(surface);

  $$Canvas*  c = $$CastOpaque($$Canvas, canvas);
  $$Surface* s = $$CastOpaque($$Surface, surface);

  SDL_SetRenderTarget($$.renderer, s->texture);
  
  if (c->background != 0)
  {
    Colour col = $$.palette->colours[c->background];
    SDL_SetRenderDrawColor($$.renderer, col.r, col.g, col.b, 0x00);
    SDL_RenderClear($$.renderer);
    SDL_SetRenderDrawColor($$.renderer, 0xFF, 0xFF, 0xFF, 0x00);
  }
  
  SDL_SetRenderDrawBlendMode($$.renderer, SDL_BLENDMODE_BLEND);
  
  // foreach drawcmds and render.
  u32 numDrawCmds = $Array_Size(c->drawCmds);
  for(u32 ii=0;ii < numDrawCmds;ii++)
  {
    $$CanvasDrawCmd* cmd = &c->drawCmds[ii];
    switch(cmd->type)
    {
      case CDC_DrawQuad:
      {
        SDL_SetRenderDrawColor($$.renderer, 0xFF, 0xFF, 0xFF, 0xFF);
        SDL_RenderCopy($$.renderer, cmd->drawQuad.texture, &cmd->drawQuad.src, &cmd->drawQuad.dst);
      }
      break;
      case CDC_DrawQuadNullDst:
      {
        SDL_SetRenderDrawColor($$.renderer, 0xFF, 0xFF, 0xFF, 0xFF);
        SDL_RenderCopy($$.renderer, cmd->drawQuadNullSrc.texture, NULL, &cmd->drawQuadNullSrc.dst);
      }
      break;
      case CDC_DrawBox:
      {
        SDL_Rect outer = cmd->drawBox.dst;
        Colour* k;

        if (cmd->drawBox.b != 0 && cmd->drawBox.f == 0)
        {
          // Bordered box
          k = &$$.palette->colours[cmd->drawBox.b];
          SDL_SetRenderDrawColor($$.renderer, k->r, k->g, k->b, 0xFF);
          SDL_RenderDrawRect($$.renderer, &outer);
        }
        else if (cmd->drawBox.b != 0 && cmd->drawBox.f != 0)
        {
          // Filled and Bordered Box
          SDL_Rect inner;
          inner.x = outer.x + 1;
          inner.y = outer.y + 1;
          inner.w = outer.w - 2;
          inner.h = outer.h - 2;
          
          k = &$$.palette->colours[cmd->drawBox.b];
          SDL_SetRenderDrawColor($$.renderer, k->r, k->g, k->b, 0xFF);
          SDL_RenderDrawRect($$.renderer, &outer);

          k = &$$.palette->colours[cmd->drawBox.f];
          SDL_SetRenderDrawColor($$.renderer, k->r, k->g, k->b, 0xFF);
          SDL_RenderFillRect($$.renderer, &inner);
        }
        else if (cmd->drawBox.b == 0 && cmd->drawBox.f != 0)
        {
          // Filled Box
          k = &$$.palette->colours[cmd->drawBox.f];
          SDL_SetRenderDrawColor($$.renderer, k->r, k->g, k->b, 0xFF);
          SDL_RenderFillRect($$.renderer, &outer);
        }
      }
      break;
      case CDC_DrawLine:
      {
        Colour* k = &$$.palette->colours[cmd->drawLine.c];
        SDL_SetRenderDrawColor($$.renderer, k->r, k->g, k->b, 0xFF);
        SDL_RenderDrawLine($$.renderer, cmd->drawLine.x0, cmd->drawLine.y0, cmd->drawLine.x1, cmd->drawLine.y1);
      }
      break;
      case CDC_DrawPoint:
      {
        Colour* k = &$$.palette->colours[cmd->drawPoint.c];
        SDL_SetRenderDrawColor($$.renderer, k->r, k->g, k->b, 0xFF);
        SDL_RenderDrawPoint($$.renderer, cmd->drawPoint.x, cmd->drawPoint.y);
      }
      break;
      case CDC_DrawText:
      {
        Colour* k = &$$.palette->colours[cmd->drawText.c];
        SDL_SetRenderDrawColor($$.renderer, k->r, k->g, k->b, 0xFF);
        
        SDL_Rect dst, src;
        src.x = 0;
        src.y = 0;
        src.w = 0;
        src.h = cmd->drawText.font->height;
        dst.x = cmd->drawText.x;
        dst.y = cmd->drawText.y;
        dst.w = 0;
        dst.h = src.h;

        while(true)
        {
          u8 ch = *cmd->drawText.text++;

          if (ch == 0x0)
            break;
    
          if (ch == ' ')
          {
            dst.x += cmd->drawText.font->width[' '];
            continue;
          }

          src.x = cmd->drawText.font->x[ch];
          src.w = cmd->drawText.font->width[ch];
          dst.w = src.w;

          SDL_RenderCopy($$.renderer, (SDL_Texture*) cmd->drawText.font->texture, &src, &dst);

          dst.x += dst.w;
        }
      }
      break;
    }
  }
  SDL_SetRenderTarget($$.renderer, NULL);
  
  $Array_Clear(c->drawCmds);
  
  i32 r = SDL_RenderCopy($$.renderer, s->texture, NULL, NULL);
}

void Canvas_Clear(Canvas* canvas, u8 colour)
{
  $EnsureOpaque(canvas);
  
  $$Canvas* c = $$CastOpaque($$Canvas, canvas);
  c->background = colour;
}

#define $$_PushCanvasCmd(C, CMD, TYPE) \
  $EnsureOpaque(canvas);\
  $$Canvas* c = $$CastOpaque($$Canvas, canvas);\
  $$CanvasDrawCmd* CMD;\
  $Array_PushAndFillOut(c->drawCmds, CMD);\
  CMD->type = TYPE;\
  $.Stats.nbDrawCalls++;

void Canvas_DrawBox(Canvas* canvas, u8 borderColour, Vec2 position, Vec2 size)
{
  $$_PushCanvasCmd(canvas, cmd, CDC_DrawBox);
  cmd->drawBox.dst.x = position.x;
  cmd->drawBox.dst.y = position.y;
  cmd->drawBox.dst.w = size.x;
  cmd->drawBox.dst.h = size.y;
  cmd->drawBox.b = borderColour;
  cmd->drawBox.f = 0;
}

void Canvas_DrawBoxXywh(Canvas* canvas, u8 borderColour, i16 x, i16 y, i16 w, i16 h)
{
  $$_PushCanvasCmd(canvas, cmd, CDC_DrawBox);
  cmd->drawBox.dst.x = x;
  cmd->drawBox.dst.y = y;
  cmd->drawBox.dst.w = w;
  cmd->drawBox.dst.h = h;
  cmd->drawBox.b = borderColour;
  cmd->drawBox.f = 0;
}

void Canvas_DrawFilledBox(Canvas* canvas, u8 fillColour, Vec2 position, Vec2 size)
{
  $$_PushCanvasCmd(canvas, cmd, CDC_DrawBox);
  cmd->drawBox.dst.x = position.x;
  cmd->drawBox.dst.y = position.y;
  cmd->drawBox.dst.w = size.x;
  cmd->drawBox.dst.h = size.y;
  cmd->drawBox.b = 0;
  cmd->drawBox.f = fillColour;
}

void Canvas_DrawFilledBoxXywh(Canvas* canvas, u8 fillColour, i16 x, i16 y, i16 w, i16 h)
{
  $$_PushCanvasCmd(canvas, cmd, CDC_DrawBox);
  cmd->drawBox.dst.x = x;
  cmd->drawBox.dst.y = y;
  cmd->drawBox.dst.w = w;
  cmd->drawBox.dst.h = h;
  cmd->drawBox.b = 0;
  cmd->drawBox.f = fillColour;
}

void Canvas_DrawBitmap(Canvas* canvas, Bitmap* bitmap, i16 x, i16 y)
{
  $Ensure(canvas);
  $Ensure(bitmap);

  $$Bitmap* b = $$CastOpaque($$Bitmap, bitmap);

  $$_PushCanvasCmd(canvas, cmd, CDC_DrawQuadNullDst);
  
  cmd->drawQuadNullSrc.texture = b->texture;
  cmd->drawQuadNullSrc.dst.x = x;
  cmd->drawQuadNullSrc.dst.y = y;
  cmd->drawQuadNullSrc.dst.w = b->w;
  cmd->drawQuadNullSrc.dst.h = b->h;
}

void Canvas_DrawLine(Canvas* canvas, u8 colour, i16 x0, i16 y0, i16 x1, i16 y1)
{
  $Ensure(canvas);

  $$_PushCanvasCmd(canvas, cmd, CDC_DrawLine);
  
  cmd->drawLine.x0 = x0;
  cmd->drawLine.y0 = y0;
  cmd->drawLine.x1 = x1;
  cmd->drawLine.y1 = y1;
  cmd->drawLine.c  = colour;
}

void Canvas_DrawPoint(Canvas* canvas, u8 colour, i16 x, i16 y)
{
  $Ensure(canvas);

  $$_PushCanvasCmd(canvas, cmd, CDC_DrawPoint);
  
  cmd->drawPoint.x = x;
  cmd->drawPoint.y = y;
  cmd->drawPoint.c = colour;
}

void Canvas_DrawSprite(Canvas* canvas, Bitmap* bitmap, Vec2 spritePosition, Vec2 spriteSize, Vec2 position)
{
  $$_PushCanvasCmd(canvas, cmd, CDC_DrawQuad);
  
  $$Bitmap* b = $$CastOpaque($$Bitmap, bitmap);

  cmd->drawQuad.src.x = spritePosition.x;
  cmd->drawQuad.src.y = spritePosition.y;
  cmd->drawQuad.src.w = spriteSize.x;
  cmd->drawQuad.src.h = spriteSize.y;
  cmd->drawQuad.dst.x = position.x;
  cmd->drawQuad.dst.y = position.y;
  cmd->drawQuad.dst.w = spriteSize.x;
  cmd->drawQuad.dst.h = spriteSize.y;
  cmd->drawQuad.texture = b->texture;
}

void Canvas_DrawSpriteXywh(Canvas* canvas, Bitmap* bitmap, i16 srcX, i16 srcY, i16 srcW, i16 srcH, i16 dstX, i16 dstY)
{
  $$_PushCanvasCmd(canvas, cmd, CDC_DrawQuad);
  
  $$Bitmap* b = $$CastOpaque($$Bitmap, bitmap);

  cmd->drawQuad.src.x = srcX;
  cmd->drawQuad.src.y = srcY;
  cmd->drawQuad.src.w = srcW;
  cmd->drawQuad.src.h = srcH;
  cmd->drawQuad.dst.x = dstX;
  cmd->drawQuad.dst.y = dstY;
  cmd->drawQuad.dst.w = srcW;
  cmd->drawQuad.dst.h = srcH;
  cmd->drawQuad.texture = b->texture;
}

void Canvas_DrawText(Canvas* canvas, Font* font, u8 colour, i16 x, i16 y, const char* text)
{
  $Ensure(text);

  $$_PushCanvasCmd(canvas, cmd, CDC_DrawText);

  $$Font* f = $$CastOpaque($$Font, font);
  u32 len = strlen(text);
  char* textCopy = $.Mem.TempAllocator(len + 1);
  memcpy(textCopy, text, len + 1);

  cmd->drawText.x    = x;
  cmd->drawText.y    = y;
  cmd->drawText.font = f;
  cmd->drawText.text = textCopy;
  cmd->drawText.c    = colour;
}

void Canvas_DrawTextF(Canvas* canvas, Font* font, u8 colour, i16 x, i16 y, const char* text, ...)
{
  $Ensure(text);

  $$_PushCanvasCmd(canvas, cmd, CDC_DrawText);

  $$Font* f = $$CastOpaque($$Font, font);

  va_list args;
  va_start(args, text);
  u32 len = vsnprintf(NULL, 0, text, args);
  va_end(args);
  char* textCopy = $.Mem.TempAllocator(len + 1);
  va_start(args, text);
  vsprintf(textCopy, text, args);
  va_end(args);

  cmd->drawText.x    = x;
  cmd->drawText.y    = y;
  cmd->drawText.font = f;
  cmd->drawText.text = textCopy;
  cmd->drawText.c    = colour;
}

void Font_New(Font* font, const char* name, Colour marker, Colour transparent)
{
  $Ensure(font);
  $$Font* f = $PermaNew($$Font);
  font->opaque = $Cast(u64) f;

  u32 width = 0, height = 0;
  u8* imageData = NULL;

#if $IsWindows == 1
  u32 resourceSize = 0;
  void* resourceData = Resource_Load(name, &resourceSize);
  lodepng_decode_memory(&imageData, &width, &height, resourceData, resourceSize, LCT_RGB, 8);
#elif $IsBrowser == 1
  char n[256];
  n[0] = 0;
  strcat(&n[0], "assets/");
  strcat(&n[0], name);
  printf("Loading Bitmap for Font: %s\n", n);
  lodepng_decode_file(&imageData, &width, &height, n, LCT_RGB, 8);
  printf("Loaded Bitmap for Font: %s %ix%i at %p\n", n, width, height, imageData);
#endif

  $Assert(imageData, "Missing image data");

  SDL_Texture* texture = SDL_CreateTexture($$.renderer, SDL_PIXELFORMAT_ABGR8888, SDL_TEXTUREACCESS_STREAMING, width, height);
  SDL_SetTextureBlendMode(texture, SDL_BLENDMODE_BLEND);

  void* pixelsVoid;
  int pitch;
  Palette* palette = $$.palette;
  
  u32 i, j;

  u32 lx = 0xCAFEBEEF;
  u8  ch = '!';

  // Scan the first line for markers.
  for(i=0;i < width * 3;i+=3)
  {
    u8 r, g, b;
    r = imageData[i + 0];
    g = imageData[i + 1];
    b = imageData[i + 2];

    if (r == marker.r && g == marker.g && b == marker.b)
    {
      int x = i / 3;

      if (lx == 0xCAFEBEEF)
      {
        lx = 0;
      }
      else
      {
        f->x[ch] = lx;
        f->width[ch] = x - lx;
        ch++;
        lx = x;
      }
    }
  }

  f->width[' '] = f->width['e'];
  
  SDL_LockTexture(texture, NULL, &pixelsVoid, &pitch);
  u8* pixels = (u8*) pixelsVoid;
  
  // Copy rest of image into the texture.
  for(i=0, j=width * 3;i < width * (height - 1) * 4;i+=4, j+=3)
  {
    u8 r, g, b;
    r = imageData[j + 0];
    g = imageData[j + 1];
    b = imageData[j + 2];

    pixels[i+0] = 0xFF;
    pixels[i+1] = 0xFF;
    pixels[i+2] = 0xFF;

    if (r == transparent.r && g == transparent.g && b == transparent.b)
    {
      pixels[i+3] = 0x00;
    }
    else
    {
      pixels[i+3] = 0xFF;
    }
  }

  SDL_UnlockTexture(texture);

  f->height = height;
  f->texture = texture;
  
}

void Mesh_Finalise(Mesh* mesh)
{
  // Calculate bounds
  Vec3f min, max, extents;
  min.x = 1000.0f;  min.y = 1000.0f;  min.z = 1000.0f;
  max.x = -1000.0f; max.y = -1000.0f; max.z = -1000.0f;
  
  for(u32 ii=0;ii < mesh->nbTriangles;ii++)
  {
    Triangle* triangle = &mesh->triangles[ii];
    for(u32 jj=0;jj < 3;jj++)
    {
      Vec3f v = triangle->v[jj];
      min.x = $Min(min.x, v.x);
      min.y = $Min(min.y, v.y);
      min.z = $Min(min.z, v.z);
      max.x = $Max(max.x, v.x);
      max.y = $Max(max.y, v.y);
      max.z = $Max(max.z, v.z);
    }
  }
  
  mesh->min = min;
  mesh->max = max;
  extents = $Vec3_Sub(mesh->max, mesh->min);
  mesh->centre = extents;
  mesh->halfSize = $Vec3_MulS($Vec3_Sub(mesh->max, mesh->min), 0.5f);

  // Radius
  mesh->squaredRadius = $Vec3_Length2(extents);
}

#define $$FrameBuffer_Size(FB)                       ((FB)->width * (FB)->height)
#define $$FrameBuffer_StoreColour(FB, X, Y, VALUE)   FB->colour[X + (Y * FB->width)] = VALUE
#define $$FrameBuffer_FetchColour(FB, X, Y)          (FB->colour[X + (Y * FB->width)])
#define $$FrameBuffer_StoreDepth(FB, X, Y, VALUE)    FB->depth[X + (Y * FB->width)] = VALUE
#define $$FrameBuffer_FetchDepth(FB, X, Y)           (FB->depth[X + (Y * FB->width)])

void $$Mat44_ProjectionMatrix(Mat44* m, u32 w, u32 h, f32 fovDeg, f32 far, f32 near);
void $$Mat44_SceneMatrix(Mat44* m, u32 w, u32 h);

void Scene_New(Scene* scene)
{
  $Ensure(scene);

  $$Scene* s = $PermaNew($$Scene);
  scene->opaque = $Cast(u64) s;

  $Array_New(s->drawCmds, 64);
  $$FrameBuffer* fb = $PermaNew($$FrameBuffer);
  s->frameBuffer = fb;

  fb->width = $.width;
  fb->height = $.height;
  fb->halfWidth = $.width * 0.5f;
  fb->halfHeight = $.height * 0.5f;
  fb->colourMem = $.Mem.PermaAllocator(NULL, sizeof(u8) *  ((fb->width * 2) + $$FrameBuffer_Size(fb)));
  fb->depthMem  = $.Mem.PermaAllocator(NULL, sizeof(f32) * ((fb->width * 2) + $$FrameBuffer_Size(fb)));

  s->frameBuffer->colour = s->frameBuffer->colourMem + fb->width;
  s->frameBuffer->depth  = s->frameBuffer->depthMem + fb->width;
  s->frameBuffer->texture = SDL_CreateTexture(
    $$.renderer,
    SDL_PIXELFORMAT_BGR888,
    SDL_TEXTUREACCESS_STREAMING,
    $.width, $.height
  );
  
  $$Mat44_ProjectionMatrix(&s->projectionMatrix, fb->width, fb->height, 80.0f, 1.0f, 100.0f);
  $$Mat44_SceneMatrix(&s->screenMatrix, fb->width, fb->height);
 
  s->cameraOutOfDate = true;
}

void Scene_Delete(Scene* scene)
{
  $EnsureOpaque(scene);
  $$Scene* s = $$CastOpaque($$Scene, scene);

  $Array_Delete(s->drawCmds);
  $PermaDelete(s->frameBuffer->colourMem);
  $PermaDelete(s->frameBuffer->depthMem);
  $PermaDelete(s->frameBuffer);
  $PermaDelete(s);
  scene->opaque = 0;
}

#define FOVY 80.0f

void $$Mat44_ProjectionMatrix(Mat44* m, u32 w, u32 h, f32 fovy_deg, f32 far, f32 near)
{
  f32 aspect    = (f32) w / (f32) h;
  f32 farPlane  = 100.0f;
  f32 nearPlane = 0.5f;

  f32 width  = tanf($Deg2Rad(fovy_deg) * 0.5f);
  f32 height = width / aspect;
  f32 diff   = farPlane - nearPlane;
  f32 div    = farPlane / diff;

  m->m[0][0] = 1.0f / width;
  m->m[0][1] = 0.0f;
  m->m[0][2] = 0.0f;
  m->m[0][3] = 0.0f;

  m->m[1][0] = 0.0f;
  m->m[1][1] = 1.0f / height;
  m->m[1][2] = 0.0f;
  m->m[1][3] = 0.0f;

  m->m[2][0] = 0.0f;
  m->m[2][1] = 0.0f;
  m->m[2][2] = div;
  m->m[2][3] = 1.0f;

  m->m[3][0] = 0.0f;
  m->m[3][1] = 0.0f;
  m->m[3][2] = -nearPlane * div;
  m->m[3][3] = 0.0f;
}

void $$Mat44_SceneMatrix(Mat44* m, u32 w, u32 h)
{
  f32 halfW = ((f32) w) * 0.5f;
  f32 halfH = ((f32) h) * 0.5f;

  m->m[0][0] = halfW;
  m->m[0][1] = 0.0f;
  m->m[0][2] = 0.0f;
  m->m[0][3] = 0.0f;

  m->m[1][0] = 0.0f;
  m->m[1][1] = -halfH;
  m->m[1][2] = 0.0f;
  m->m[1][3] = 0.0f;

  m->m[2][0] = 0.0f;
  m->m[2][1] = 0.0f;
  m->m[2][2] = 1.0f;
  m->m[2][3] = 0.0f;

  m->m[3][0] = halfW;
  m->m[3][1] = halfH;
  m->m[3][2] = 0.0f;
  m->m[3][3] = 1.0f;
}

typedef struct
{
  i32 x, y;
  f32 z, w;
} RVec;

inline void $$Vec4_XForm(RVec* dst, Vec4f* m)
{
  dst->x = (i32) (m->x + 0.5f);
  dst->y = (i32) (m->y + 0.5f);
  dst->z = m->z;
  dst->w = m->w;
}

#define $$Fwd_FragmentShader(NAME)\
bool $$Fragment_Shader_##NAME($$FrameBuffer* fb,  Vec4f* v0, Vec4f* v1, Vec4f* v2, u8 c)

#define $$Make_FragmentShader(NAME, ...)\
bool $$Fragment_Shader_##NAME($$FrameBuffer* fb,  Vec4f* v0, Vec4f* v1, Vec4f* v2, u8 c) \
{\
  RVec r[3]; $$Vec4_XForm(&r[0], v0); $$Vec4_XForm(&r[1], v1); $$Vec4_XForm(&r[2], v2); \
  i32 A0 = r[1].y - r[2].y; /* 0 sized triangle rejection*/ \
  i32 B0 = r[2].x - r[1].x;\
  i32 C0 = r[1].x * r[2].y - r[2].x * r[1].y;\
  i32 triArea = A0 * r[0].x + B0 * r[0].y + C0;\
  if (triArea <= 0) return false;\
  i32 minX = $Max($Min3(r[0].x, r[1].x, r[2].x), 0) & (i32) 0xFFFFFFFE,\
      maxX = $Min($Max3(r[0].x, r[1].x, r[2].x), (fb->width - 1)),\
      minY = $Max($Min3(r[0].y, r[1].y, r[2].y), 0) & (i32) 0xFFFFFFFE,\
      maxY = $Min($Max3(r[0].y, r[1].y, r[2].y), (fb->height - 1));\
  if(maxX < minX || maxY < minY) return false;\
  i32 A1 = r[2].y - r[0].y, A2 = r[0].y - r[1].y,\
      B1 = r[0].x - r[2].x, B2 = r[1].x - r[0].x,\
      C1 = r[2].x * r[0].y - r[0].x * r[2].y, C2 = r[0].x * r[1].y - r[1].x * r[0].y;\
  f32 oneOverTriArea = (1.0f/ $Cast(f32)(triArea));\
  r[1].z = (r[1].z - r[0].z) * oneOverTriArea;\
  r[2].z = (r[2].z - r[0].z) * oneOverTriArea;\
  f32 zx = A1 * r[1].z + A2 * r[2].z;\
  Vec2 p = { .x = minX, .y = minY };\
  i32 w0Row = (A0 * p.x) + (B0 * p.y) + C0,\
      w1Row = (A1 * p.x) + (B1 * p.y) + C1,\
      w2Row = (A2 * p.x) + (B2 * p.y) + C2;\
  for(p.y = minY; p.y <= maxY; p.y++) {\
    i32 w0 = w0Row, w1 = w1Row, w2 = w2Row;\
    f32 depth = r[0].z + r[1].z * w1 + r[2].z * w2;\
    for(p.x = minX; p.x <= maxX;p.x++) {\
        if ((w0 | w1 | w2) >= 0) {\
          f32 prevDepth = $$FrameBuffer_FetchDepth(fb, p.x, p.y);\
          if (depth < prevDepth) {\
            __VA_ARGS__;\
            $$FrameBuffer_StoreDepth(fb, p.x, p.y, depth);\
          }\
      }\
      w0 += A0; w1 += A1; w2 += A2; depth += zx; }\
    w0Row += B0; w1Row += B1;  w2Row += B2;\
  }\
  return true;\
}

#define $$FRAGMENT_SET_COLOUR(C)  $$FrameBuffer_StoreColour(fb, p.x, p.y, C);



bool $$Fragment_Shader_InternalMetric($$FrameBuffer* fb,  Vec4f* v0, Vec4f* v1, Vec4f* v2, u8 c)
{
  RVec r[3];
  $$Vec4_XForm(&r[0], v0);
  $$Vec4_XForm(&r[1], v1);
  $$Vec4_XForm(&r[2], v2);

  // Reject 0 sized triangles
  i32 A0 = r[1].y - r[2].y;
  i32 B0 = r[2].x - r[1].x;
  i32 C0 = r[1].x * r[2].y - r[2].x * r[1].y;
  i32 triArea = A0 * r[0].x + B0 * r[0].y + C0;

  if (triArea <= 0)
    return false;
  
  // Reject triangle if not on screen
  i32 minX = $Max($Min3(r[0].x, r[1].x, r[2].x), 0) & (i32) 0xFFFFFFFE;
  i32 maxX = $Min($Max3(r[0].x, r[1].x, r[2].x), (fb->width - 1));
  i32 minY = $Max($Min3(r[0].y, r[1].y, r[2].y), 0) & (i32) 0xFFFFFFFE;
  i32 maxY = $Min($Max3(r[0].y, r[1].y, r[2].y), (fb->height - 1));
  if(maxX < minX || maxY < minY)
    return false;
  
  i32 A1 = r[2].y - r[0].y;
  i32 A2 = r[0].y - r[1].y;
  i32 B1 = r[0].x - r[2].x;
  i32 B2 = r[1].x - r[0].x;
  i32 C1 = r[2].x * r[0].y - r[0].x * r[2].y;
  i32 C2 = r[0].x * r[1].y - r[1].x * r[0].y;
  
  f32 oneOverTriArea = (1.0f/ $Cast(f32)(triArea));
  
  r[1].z = (r[1].z - r[0].z) * oneOverTriArea;
  r[2].z = (r[2].z - r[0].z) * oneOverTriArea;
  
  f32 zx = A1 * r[1].z + A2 * r[2].z;
  
  Vec2 p = { .x = minX, .y = minY };

  i32 w0Row = (A0 * p.x) + (B0 * p.y) + C0;
  i32 w1Row = (A1 * p.x) + (B1 * p.y) + C1;
  i32 w2Row = (A2 * p.x) + (B2 * p.y) + C2;

  for(p.y = minY; p.y <= maxY; p.y++)
  {
    i32 w0 = w0Row;
    i32 w1 = w1Row;
    i32 w2 = w2Row;

    f32 depth = r[0].z + r[1].z * w1 + r[2].z * w2;

    for(p.x = minX; p.x <= maxX;p.x++)
    {
      if ((w0 | w1 | w2) >= 0)
      {
        f32 prevDepth = $$FrameBuffer_FetchDepth(fb, p.x, p.y);
        if (depth < prevDepth)
        {
          $$FrameBuffer_StoreColour(fb, p.x, p.y, c);
          $$FrameBuffer_StoreDepth(fb, p.x, p.y, depth);
        }
      }

      w0 += A0;
      w1 += A1;
      w2 += A2;
      depth += zx;
    }
    
    w0Row += B0;
    w1Row += B1;
    w2Row += B2;
  }

  return true;
}

typedef bool (*$$FragmentShader)($$FrameBuffer* fb,  Vec4f* v0, Vec4f* v1, Vec4f* v2, u8 c);

$$Fwd_FragmentShader(Standard);
$$Fwd_FragmentShader(Shadow);

static $$FragmentShader $$FRAGMENT_SHADERS[2] = {
  $$Fragment_Shader_Standard,
  $$Fragment_Shader_Shadow,
};


void $$Scene_DrawModel($$Scene* scene, $$FrameBuffer* fb, Mat44* vps, Mesh* mesh, Vec3f position, Rot3i rotation, u8 shader)
{
  Vec3f u[3]; Vec4f v[3];
  Mat44 m;

  // Model
  $Mat44_Identity(&m);
  $Mat44_RotMatrixZXY(&m, rotation);
  $Mat44_MultiplyTransform(&m, position);

  // VPS
  $Mat44_Multiply(&m, vps, &m);
  
  for(u32 ii=0;ii < mesh->nbTriangles;ii++)
  {
    Triangle* triangle = &mesh->triangles[ii];
    bool render = true;
    for(u32 jj=0;jj < 3;jj++)
    {
      u[jj] = triangle->v[jj];

      Vec4f p;
      p.x = u[jj].x;
      p.y = u[jj].y;
      p.z = u[jj].z;
      p.w = 1.0f;

      $Mat44_MultiplyVec4(&v[jj], &m, &p);
      
      if (v[jj].z <= v[jj].w)
      {
        v[jj].x = v[jj].x / v[jj].w;
        v[jj].y = v[jj].y / v[jj].w;

        v[jj].z = v[jj].z / v[jj].w;
        v[jj].w = v[jj].w;
      }
      else
      {
        render = false;
        break;
      }
    }
    
    if(render && (v[0].w > 0.0f && v[1].w > 0.0f && v[2].w > 0.0f))
    {
      if ($$FRAGMENT_SHADERS[shader](fb, &v[0], &v[1], &v[2], triangle->colour))
      {
        $.Stats.nbTriangles++;
      }
    }
  }
}

void $$Scene_DrawSkybox($$Scene* scene, $$FrameBuffer* fb, f32 y, u8 sky, u8 ground)
{
  i32 yPx = (i32) (y + 0.5f);
  
  if (yPx > fb->height - 1)
  {
    memset(fb->colour, sky, fb->height * fb->width);
  }
  else if (yPx < 0)
  {
    memset(fb->colour, ground, fb->height * fb->width);
  }
  else
  {
    i32 horizon = yPx;
    for(i32 yy=0;yy < horizon;yy++)
    {
      for(i32 xx=0;xx < fb->width;xx++)
      {
       $$FrameBuffer_StoreColour(fb, xx, yy, sky);
      }
    }
    for(i32 yy=horizon;yy < fb->height;yy++)
    {
      for(i32 xx=0;xx < fb->width;xx++)
      {
       $$FrameBuffer_StoreColour(fb, xx, yy, ground);
      }
    }
  }
}

#define $$RENDER_ONLY_DEPTH 0

void Scene_Render(Scene* scene, Surface* surface)
{
  $EnsureOpaque(scene);
  $$Scene* s = $$CastOpaque($$Scene, scene);
  $$Surface* g = $$CastOpaque($$Surface, surface);
  $$FrameBuffer* fb = s->frameBuffer;

  // Clear colour and depth bfufers
  memset(fb->colour, 0, sizeof(u8)  * $$FrameBuffer_Size(fb));
  //memset(fb->depth,  0, sizeof(f32) * $$FrameBuffer_Size(fb));
  i32 wh = $$FrameBuffer_Size(fb);
  for(i32 i=0;i < wh;i++)
    fb->depth[i] = 1.0f;

  $.Stats.nbTriangles = 0;
  
  if (s->cameraOutOfDate)
  {
    $Mat44_LookAt(&s->viewMatrix, s->cameraPosition, s->cameraTarget);
    $Mat44_Inverse(&s->viewMatrix, &s->viewMatrix);

    Vec4f farclip;
    farclip.x = 0;
    farclip.y = 0;
    farclip.z = 100;
    farclip.w = 1;

    Vec4f worldFarClip;

    $Mat44_MultiplyVec4(&worldFarClip, &s->viewMatrix, &farclip);

    worldFarClip.y = 0;

    worldFarClip.x *= 100.0f;
    worldFarClip.z *= 100.0f;

    $Mat44_Identity(&s->vpsMatrix);
    $Mat44_Multiply(&s->vpsMatrix, &s->viewMatrix, &s->vpsMatrix);
    $Mat44_Multiply(&s->vpsMatrix, &s->projectionMatrix, &s->vpsMatrix);
    $Mat44_Multiply(&s->vpsMatrix, &s->screenMatrix, &s->vpsMatrix);

    Vec4f screenFarPlane;

    $Mat44_MultiplyVec4(&screenFarPlane, &s->vpsMatrix, &worldFarClip);

    s->skyboxY = screenFarPlane.y / screenFarPlane.w;
    s->cameraOutOfDate = false;
  }

  // Foreach command, transform coordinates, trianglate and send to rasterizer
  u32 numDrawCmds = $Array_Size(s->drawCmds);
  for(u32 ii=0;ii < numDrawCmds;ii++)
  {
    $$SceneDrawCmd* cmd = &s->drawCmds[ii];
    switch(cmd->type)
    {
      case SDC_DrawModel:
      {
        $$Scene_DrawModel(s, fb, &s->vpsMatrix, cmd->drawModel.mesh, cmd->drawModel.position, cmd->drawModel.rotation, cmd->drawModel.shader);
      }
      break;
      case SDC_DrawSkybox:
      {
        $$Scene_DrawSkybox(s, fb, s->skyboxY, cmd->drawSkyBox.sky, cmd->drawSkyBox.ground);
      }
      break;
    }
  }
  
  // Turn paletted colour buffer, into an actual colour into the texture
  u8* pixels = NULL;
  int pixelPitch = 0;

  u32 limit = $$FrameBuffer_Size(fb);
  Palette* palette = $$.palette;
  
  SDL_LockTexture(fb->texture, NULL, (void*) &pixels, &pixelPitch);

  for(u32 i=0, j=0;i < limit;i++,j+=4 /* RGBA for some reason */)
  {
#if $$RENDER_ONLY_DEPTH == 1
    u8 index = (u8) ((1.0f - fb->depth[i]) * 255.0f);
    pixels[j + 0] = index; // R
    pixels[j + 1] = index; // G
    pixels[j + 2] = index; // B
#else
    u8 index = fb->colour[i];
    Colour* colour = &palette->colours[index];
    pixels[j + 0] = colour->r; // R
    pixels[j + 1] = colour->g; // G
    pixels[j + 2] = colour->b; // B
#endif
  }
  
  SDL_UnlockTexture(fb->texture);
  
  SDL_SetRenderTarget($$.renderer, g->texture);
  SDL_RenderCopy($$.renderer, fb->texture, NULL, NULL);
  SDL_SetRenderTarget($$.renderer, NULL);

  $Array_Clear(s->drawCmds);
}

void Scene_Clear(Scene* scene, u8 colour)
{
  $EnsureOpaque(scene);
  $$Scene* s = $$CastOpaque($$Scene, scene);
  
  $Array_Clear(s->drawCmds);
}

void Scene_SetPovLookAt(Scene* scene, Vec3f position, Vec3f target)
{
  $EnsureOpaque(scene);
  $$Scene* s = $$CastOpaque($$Scene, scene);
  
  s->cameraPosition  = position;
  s->cameraTarget    = target;
  s->cameraOutOfDate = true;
}

void Scene_SetPovLookAtXyz(Scene* scene, f32 px, f32 py, f32 pz, f32 tx, f32 ty, f32 tz)
{
  $EnsureOpaque(scene);
  $$Scene* s = $$CastOpaque($$Scene, scene);
  
  s->cameraPosition.x  = px;
  s->cameraPosition.y  = py;
  s->cameraPosition.z  = pz;
  s->cameraTarget.x    = tx;
  s->cameraTarget.y    = ty;
  s->cameraTarget.z    = tz;
  s->cameraOutOfDate   = true;
}

void Scene_DrawSkybox(Scene* scene, u8 sky, u8 ground)
{
  $EnsureOpaque(scene);
  $$Scene* s = $$CastOpaque($$Scene, scene);
  
  $$SceneDrawCmd* cmd;
  $Array_PushAndFillOut(s->drawCmds, cmd);
  cmd->type               = SDC_DrawSkybox;
  cmd->drawSkyBox.sky     = sky;
  cmd->drawSkyBox.ground  = ground;
}

void Scene_DrawMesh(Scene* scene, Mesh* mesh, Vec3f position, Rot3i rotation)
{
  $EnsureOpaque(scene);
  $$Scene* s = $$CastOpaque($$Scene, scene);
  
  $$SceneDrawCmd* cmd;
  $Array_PushAndFillOut(s->drawCmds, cmd);
  cmd->type               = SDC_DrawModel;
  cmd->drawModel.mesh     = mesh;
  cmd->drawModel.position = position;
  cmd->drawModel.rotation = rotation;
  cmd->drawModel.shader   = 0;
}

void Scene_DrawMeshXyz(Scene* scene, Mesh* mesh, f32 x, f32 y, f32 z, i16 pitch, i16 yaw, i16 roll)
{
  $EnsureOpaque(scene);
  $$Scene* s = $$CastOpaque($$Scene, scene);
  
  $$SceneDrawCmd* cmd;
  $Array_PushAndFillOut(s->drawCmds, cmd);
  cmd->type           = SDC_DrawModel;
  cmd->drawModel.mesh     = mesh;
  cmd->drawModel.position.x = x;
  cmd->drawModel.position.y = y;
  cmd->drawModel.position.z = z;
  cmd->drawModel.rotation.pitch = pitch;
  cmd->drawModel.rotation.yaw   = yaw;
  cmd->drawModel.rotation.roll  = roll;
  cmd->drawModel.shader    = 0;
}

void Scene_DrawCustomShaderMesh(Scene* scene, Mesh* mesh, u8 shader, Vec3f position, Rot3i rotation)
{
  $EnsureOpaque(scene);
  $$Scene* s = $$CastOpaque($$Scene, scene);
  
  $$SceneDrawCmd* cmd;
  $Array_PushAndFillOut(s->drawCmds, cmd);
  cmd->type           = SDC_DrawModel;
  cmd->drawModel.mesh     = mesh;
  cmd->drawModel.position = position;
  cmd->drawModel.rotation = rotation;
  cmd->drawModel.shader   = shader;
}

void Scene_DrawCustomShaderMeshXyz(Scene* scene, Mesh* mesh, u8 shader, f32 x, f32 y, f32 z, i16 pitch, i16 yaw, i16 roll)
{
  $EnsureOpaque(scene);
  $$Scene* s = $$CastOpaque($$Scene, scene);
  
  $$SceneDrawCmd* cmd;
  $Array_PushAndFillOut(s->drawCmds, cmd);
  cmd->type           = SDC_DrawModel;
  cmd->drawModel.mesh     = mesh;
  cmd->drawModel.position.x = x;
  cmd->drawModel.position.y = y;
  cmd->drawModel.position.z = z;
  cmd->drawModel.rotation.pitch = pitch;
  cmd->drawModel.rotation.yaw   = yaw;
  cmd->drawModel.rotation.roll  = roll;
  cmd->drawModel.shader = shader;
}

#define TF_NONE    0
#define TF_RUNNING 1
#define TF_PAUSED  2

void Sound_New(Sound* sound, const char* name)
{
#if $AUDIO_ENABLED == 1
  $Ensure(sound);
  $Ensure(name);

  $$Sound* s = $PermaNew($$Sound);
  sound->opaque = $Cast(u64) s;
  
  #if $IsWindows == 1
  u32 resourceSize = 0;
  void* resource = Resource_Load(name, &resourceSize);
  SDL_LoadWAV_RW(SDL_RWFromConstMem(resource, resourceSize), 0, &s->spec, &s->buffer, &s->length);
  #elif $IsBrowser == 1
  char n[256];
  n[0] = 0;
  strcat(&n[0], "assets/");
  strcat(&n[0], name);
  printf("Loading Sound: %s\n", n);
  SDL_LoadWAV(n, &s->spec, &s->buffer, &s->length);
  printf("Loaded Sound: %s %i\n", n, s->length);
  #endif
  
  if (s->spec.format   != $$.audioSpec.format || 
      s->spec.freq     != $$.audioSpec.freq || 
      s->spec.channels != $$.audioSpec.channels)
  {
    // Do a conversion
    SDL_AudioCVT cvt;
    SDL_BuildAudioCVT(&cvt, s->spec.format, s->spec.channels, s->spec.freq, $$.audioSpec.format, $$.audioSpec.channels, $$.audioSpec.freq);

    cvt.buf = $.Mem.PermaAllocator(NULL, s->length * cvt.len_mult);
    memcpy(cvt.buf, s->buffer, s->length);
    cvt.len = s->length;
    SDL_ConvertAudio(&cvt);
    SDL_FreeWAV(s->buffer);

    s->buffer = cvt.buf;
    s->length = cvt.len_cvt;
    s->spec = $$.audioSpec;

    // printf("Loaded Audio %s but had to convert it into a internal format.\n", name);
  }
  else
  {
   // printf("Loaded Audio %s\n", name);
  }
#endif
}

void $$Sound_Callback(void* userdata, u8* stream, int streamLength)
{
#if $AUDIO_ENABLED == 1
  // @TODO Figure out why there is a delay in audio feedback - it's noticable.

  SDL_memset(stream, 0, streamLength);

  #if $MUSIC_ENABLED == 1
  
  if ($$.musicContext != NULL)
  {
    
    long count = 0;

    if ($$.audioSpec.format == AUDIO_S16)
      count = streamLength / 2;
    else
      count = streamLength / 4;
    
    if( $$.musicContext->samples_remaining < count ) {
      /* Clear output.*/
      count = $$.musicContext->samples_remaining;
    }

    if( count > 0 ) {
      /* Get audio from replay.*/

      memset( $$.musicContext->mix_buffer, 0, count * NUM_CHANNELS * sizeof( short ) );
      micromod_get_audio( $$.musicContext->mix_buffer, count );
      
      if ($$.audioSpec.format == AUDIO_S16)
        micromod_sdl_downsample($$.musicContext, $$.musicContext->mix_buffer, (short *) stream, count );
      else
        micromod_sdl_downsample_float( $$.musicContext, $$.musicContext->mix_buffer, (float*) stream, count);
     
      $$.musicContext->samples_remaining -= count;
    }
    else
    {
      $$.musicContext->samples_remaining = $$.musicContext->length;
    }
    
  }
  
  #endif
  
  // Blend in any playing sound instances
  for(u32 ii=0;ii < $$AUDIO_INSTANCES;ii++)
  {
    $$SoundInstance* instance = &$$.soundInstances[ii];
    if (instance->sound == NULL)
      continue;
    
    i32 soundLength = instance->sound->length;
    i32 mixLength = (streamLength > soundLength ? soundLength : streamLength);
    
    if (instance->position + mixLength >= soundLength)
    {
      mixLength = soundLength - instance->position;
    }

    SDL_MixAudioFormat(stream, instance->sound->buffer + instance->position, instance->sound->spec.format, mixLength, SDL_MIX_MAXVOLUME / 2);
    
    instance->position += mixLength;
    
    if (instance->position >= instance->sound->length)
    {
      // Finished
      instance->sound = NULL;
      instance->position = 0;
      instance->volume = 0;
    }
  }
#endif
}

void $$Sound_Init()
{
#if $AUDIO_ENABLED == 1
  SDL_AudioSpec wantSpec, givenSpec;
  memset(&wantSpec, 0, sizeof(wantSpec));
  memset(&givenSpec, 0, sizeof(givenSpec));

  wantSpec.freq     = $$AUDIO_FREQUENCY;
  wantSpec.format   = AUDIO_S16;
  wantSpec.channels = $$AUDIO_CHANNELS;
  wantSpec.samples  = $$AUDIO_SAMPLES;
  wantSpec.callback = $$Sound_Callback;
  wantSpec.userdata = NULL;

  if (SDL_OpenAudio(&wantSpec, &givenSpec) < 0)
  {
    wantSpec.format = AUDIO_F32;
    if (SDL_OpenAudio(&wantSpec, &givenSpec) < 0)
    {
      printf("Sound Init Error: %s\n", SDL_GetError());
    }
  }

  $$.audioSpec = givenSpec;
  
  SDL_PauseAudio(0);
#endif
}

void Sound_Play(Sound* sound)
{
#if $AUDIO_ENABLED == 1
  $Ensure(sound);
  
  $$Sound* s = $$CastOpaque($$Sound, sound);
  
  for(u32 ii=0;ii < $$AUDIO_INSTANCES;ii++)
  {
    $$SoundInstance* instance = &$$.soundInstances[ii];
    if (instance->sound != NULL)
      continue;

    instance->sound = s;
    instance->position = 0;
    instance->volume = SDL_MIX_MAXVOLUME;
    return;
  }
#endif
}

void Sound_MuteAll()
{
#if $AUDIO_ENABLED == 1
  for(u32 ii=0;ii < $$AUDIO_INSTANCES;ii++)
  {
    $$SoundInstance* instance = &$$.soundInstances[ii];
    
    instance->sound = NULL;
    instance->position = 0;
    instance->volume = 0;
    return;
  }
#endif
}

void Music_Play(const char* name)
{
#if $MUSIC_ENABLED == 1
  if ($$.musicContext != NULL)
  {
    $PermaDelete($$.musicContext);
  }
  
  $$.musicContext =  $PermaNew(micromod_sdl_context);
  
  void* data = NULL;
  u32 dataLength = 0;
  
#if $IsWindows == 1
  data = Resource_Load(name, &dataLength);
#elif $IsBrowser == 1
  char n[256];
  n[0] = 0;
  strcat(&n[0], "assets/");
  strcat(&n[0], name);
  printf("Loading Music: %s\n", n);

  FILE* f = fopen(n, "rb");
  fseek(f, 0, SEEK_END);
  dataLength = ftell(f);
  fseek(f, 0, SEEK_SET);

  data = $.Mem.PermaAllocator(NULL, dataLength);
  fread(data, dataLength, 1, f);
  fclose(f);
  
  printf("Loaded Music: %s %i\n", n, dataLength);
#endif
  
  micromod_initialise(data, SAMPLING_FREQ * OVERSAMPLE);
  $$.musicContext->samples_remaining = micromod_calculate_song_duration();
  $$.musicContext->length            = $$.musicContext->samples_remaining;
#endif
}

void Timer_New(Timer* timer)
{
  $Ensure(timer);
  
  timer->start  = 0;
  timer->paused = 0;
  timer->state  = TF_NONE;
}

void Timer_Delete(Timer* timer)
{
  $Ensure(timer);
  
  timer->start  = 0;
  timer->paused = 0;
  timer->state  = TF_NONE;
}

void Timer_Start(Timer* timer)
{
  $Ensure(timer);
  
  timer->start  = SDL_GetTicks();
  timer->paused = 0;
  timer->state  = TF_RUNNING;
}

void Timer_Stop(Timer* timer)
{
  $Ensure(timer);
  
  timer->start  = 0;
  timer->paused = 0;
  timer->state  = TF_NONE;
}

void Timer_Pause(Timer* timer)
{
  $Ensure(timer);
  
  if (timer->state == TF_RUNNING)
  {
    timer->state  |= TF_PAUSED;
    timer->paused  = SDL_GetTicks() - timer->start;
    timer->start   = 0;
  }
}

void Timer_Unpause(Timer* timer)
{
  $Ensure(timer);
  
  if (timer->state == 3 /* Started | Paused */)
  {
    timer->state   = TF_RUNNING; // &= ~Paused
    timer->start   = SDL_GetTicks() - timer->paused;
    timer->paused  = 0;
  }
}

u32  Timer_Ticks(Timer* timer)
{
  $Ensure(timer);
  
  u32 time = 0;

  if (timer->state != 0) // Started || Paused
  {
    if (timer->state > TF_RUNNING) // Paused
    {
      time = timer->paused;
    }
    else
    {
      time = SDL_GetTicks() - timer->start;
    }
  }

  return time;
}

bool Timer_IsRunning(Timer* timer)
{
  $Ensure(timer);

  return timer->state >= TF_RUNNING;
}

bool Timer_IsPaused(Timer* timer)
{
  $Ensure(timer);
  
  return timer->state >= TF_PAUSED;
}

void Input_BindControl(u32 control, i32 key)
{
  $$Control* c;
  $Array_PushAndFillOut($$.controls, c); 
  c->key       = key;
  c->control   = control;
  c->state     = 0;
  c->lastState = 0;
}

bool Input_ControlDown(u32 control)
{
  u32 length = $Array_Size($$.controls);
  for(u32 ii=0;ii < length;ii++)
  {
    $$Control* c = &$$.controls[ii];
    if (c->control == control)
      return c->state == 1;
  }
  return false;
}

bool Input_ControlPressed(u32 control)
{
  u32 length = $Array_Size($$.controls);
  for(u32 ii=0;ii < length;ii++)
  {
    $$Control* c = &$$.controls[ii];
    if (c->control == control)
      return c->state == 1 && c->lastState == 0;
  }
  return false;
}

bool Input_ControlReleased(u32 control)
{
  u32 length = $Array_Size($$.controls);
  for(u32 ii=0;ii < length;ii++)
  {
    $$Control* c = &$$.controls[ii];
    if (c->control == control)
      return c->state == 0 && c->lastState == 1;
  }
  return false;
}

void $Vec3_Transform(Vec3f* v, Rot3i* b)
{
  if (b->pitch)
  {
    $Vec3_TransformPitch(v, b->pitch);
  }

  if (b->yaw)
  {
    $Vec3_TransformYaw(v, b->yaw);
  }
  
  if (b->pitch)
  {
    $Vec3_TransformRoll(v, b->roll);
  }
  
}

void $Vec3_TransformPitch(Vec3f* v, i16 pitch)
{
  Vec3f t = *v;

  f32 r = $Deg2Rad(pitch);
  f32 c = cosf(r), s = sinf(r);

  v->x = c * t.x - s * t.y;
  v->y = s * t.x + c * t.y;
  v->z = t.z;
}

void $Vec3_TransformYaw(Vec3f* v, i16 yaw)
{
  Vec3f t = *v;

  f32 r = $Deg2Rad(yaw);
  f32 c = cosf(r), s = sinf(r);

  v->x = c * t.x + s * t.z;
  v->y = t.y;
  v->z = -s * t.x + c * t.z;
}

void $Vec3_TransformRoll(Vec3f* v, i16 roll)
{
  Vec3f t = *v;

  f32 r = $Deg2Rad(roll);
  f32 c = cosf(r), s = sinf(r);
  
  v->x = t.x;
  v->y = c * t.y - s * t.z;
  v->z = s * t.y + c * t.z;
}

void $Vec3_InvTransform(Vec3f* v, Rot3i* b)
{
  if (b->pitch)
  {
    $Vec3_InvTransformPitch(v,  b->pitch);
  }

  if (b->yaw)
  {
    $Vec3_InvTransformYaw(v, b->yaw);
  }
  
  if (b->pitch)
  {
    $Vec3_InvTransformRoll(v, b->roll);
  }
}

void $Vec3_InvTransformPitch(Vec3f* v, i16 pitch)
{
  Vec3f t = *v;

  f32 r = $Deg2Rad(pitch);
  f32 c = cosf(r), s = sinf(r);
  f32 d = c * c + s * s;
  
  v->x = (c * t.x + s * t.y) / d;
  v->y = -(s * t.x + c * t.y) / d;
  v->z = t.z;
}

void $Vec3_InvTransformYaw(Vec3f* v,  i16 yaw)
{
  Vec3f t = *v;

  f32 r = $Deg2Rad(yaw);
  f32 c = cosf(r), s = sinf(r);
  f32 d = c * c + s * s;
  v->x = (c * t.x - s * t.z) / d;
  v->y = t.y;
  v->z = (s * t.x + c * t.z) / d;
}

void $Vec3_InvTransformRoll(Vec3f* v, i16 roll)
{
  Vec3f t = *v;

  f32 r = $Deg2Rad(roll);
  f32 c = cosf(r), s = sinf(r);
  f32 d = c * c + s * s;

  v->x = t.x;
  v->y = (c * t.y + s * t.z) / d;
  v->z = -(c * t.z - s * t.z) / d;
}

void $Mat44_Identity(Mat44* m)
{
  m->m[0][0] = 1.0f;
  m->m[0][1] = 0.0f;
  m->m[0][2] = 0.0f;
  m->m[0][3] = 0.0f;
  m->m[1][0] = 0.0f;
  m->m[1][1] = 1.0f;
  m->m[1][2] = 0.0f;
  m->m[1][3] = 0.0f;
  m->m[2][0] = 0.0f;
  m->m[2][1] = 0.0f;
  m->m[2][2] = 1.0f;
  m->m[2][3] = 0.0f;
  m->m[3][0] = 0.0f;
  m->m[3][1] = 0.0f;
  m->m[3][2] = 0.0f;
  m->m[3][3] = 1.0f;
}

void $Mat44_Multiply(Mat44* t, Mat44* a, Mat44* b)
{
  f32 a00 = a->M[0], a01 = a->M[1], a02 = a->M[2], a03 = a->M[3],
      a10 = a->M[4], a11 = a->M[5], a12 = a->M[6], a13 = a->M[7],
      a20 = a->M[8], a21 = a->M[9], a22 = a->M[10], a23 = a->M[11],
      a30 = a->M[12], a31 = a->M[13], a32 = a->M[14], a33 = a->M[15],

      b00 = b->M[0], b01 = b->M[1], b02 = b->M[2], b03 = b->M[3],
      b10 = b->M[4], b11 = b->M[5], b12 = b->M[6], b13 = b->M[7],
      b20 = b->M[8], b21 = b->M[9], b22 = b->M[10], b23 = b->M[11],
      b30 = b->M[12], b31 = b->M[13], b32 = b->M[14], b33 = b->M[15];

  t->M[0] = b00 * a00 + b01 * a10 + b02 * a20 + b03 * a30;
  t->M[1] = b00 * a01 + b01 * a11 + b02 * a21 + b03 * a31;
  t->M[2] = b00 * a02 + b01 * a12 + b02 * a22 + b03 * a32;
  t->M[3] = b00 * a03 + b01 * a13 + b02 * a23 + b03 * a33;
  t->M[4] = b10 * a00 + b11 * a10 + b12 * a20 + b13 * a30;
  t->M[5] = b10 * a01 + b11 * a11 + b12 * a21 + b13 * a31;
  t->M[6] = b10 * a02 + b11 * a12 + b12 * a22 + b13 * a32;
  t->M[7] = b10 * a03 + b11 * a13 + b12 * a23 + b13 * a33;
  t->M[8] = b20 * a00 + b21 * a10 + b22 * a20 + b23 * a30;
  t->M[9] = b20 * a01 + b21 * a11 + b22 * a21 + b23 * a31;
  t->M[10] = b20 * a02 + b21 * a12 + b22 * a22 + b23 * a32;
  t->M[11] = b20 * a03 + b21 * a13 + b22 * a23 + b23 * a33;
  t->M[12] = b30 * a00 + b31 * a10 + b32 * a20 + b33 * a30;
  t->M[13] = b30 * a01 + b31 * a11 + b32 * a21 + b33 * a31;
  t->M[14] = b30 * a02 + b31 * a12 + b32 * a22 + b33 * a32;
  t->M[15] = b30 * a03 + b31 * a13 + b32 * a23 + b33 * a33;

}

void $Mat44_MultiplyVec4(Vec4f* out, Mat44* m, Vec4f* v)
{
  out->x = v->x * m->row[0].x + v->y * m->row[1].x + v->z * m->row[2].x + v->w * m->row[3].x;
  out->y = v->x * m->row[0].y + v->y * m->row[1].y + v->z * m->row[2].y + v->w * m->row[3].y;
  out->z = v->x * m->row[0].z + v->y * m->row[1].z + v->z * m->row[2].z + v->w * m->row[3].z;
  out->w = v->x * m->row[0].w + v->y * m->row[1].w + v->z * m->row[2].w + v->w * m->row[3].w;
}

void $Mat44_Inverse(Mat44* m, Mat44* v)
{
    // Cache the v->Mrix values (makes for huge speed increases!)
    f32 a00 = v->M[0], a01 = v->M[1], a02 = v->M[2], a03 = v->M[3],
        a10 = v->M[4], a11 = v->M[5], a12 = v->M[6], a13 = v->M[7],
        a20 = v->M[8], a21 = v->M[9], a22 = v->M[10], a23 = v->M[11],
        a30 = v->M[12], a31 = v->M[13], a32 = v->M[14], a33 = v->M[15],

        b00 = a00 * a11 - a01 * a10,
        b01 = a00 * a12 - a02 * a10,
        b02 = a00 * a13 - a03 * a10,
        b03 = a01 * a12 - a02 * a11,
        b04 = a01 * a13 - a03 * a11,
        b05 = a02 * a13 - a03 * a12,
        b06 = a20 * a31 - a21 * a30,
        b07 = a20 * a32 - a22 * a30,
        b08 = a20 * a33 - a23 * a30,
        b09 = a21 * a32 - a22 * a31,
        b10 = a21 * a33 - a23 * a31,
        b11 = a22 * a33 - a23 * a32,

        d = (b00 * b11 - b01 * b10 + b02 * b09 + b03 * b08 - b04 * b07 + b05 * b06),
        invDet;

        // Calculate the determinant
        if (!d)
        {
          printf("Bad determinant");
          $Mat44_Identity(m); 
          return;
        }
        invDet = 1.0f / d;

    m->M[0] = (a11 * b11 - a12 * b10 + a13 * b09) * invDet;
    m->M[1] = (-a01 * b11 + a02 * b10 - a03 * b09) * invDet;
    m->M[2] = (a31 * b05 - a32 * b04 + a33 * b03) * invDet;
    m->M[3] = (-a21 * b05 + a22 * b04 - a23 * b03) * invDet;
    m->M[4] = (-a10 * b11 + a12 * b08 - a13 * b07) * invDet;
    m->M[5] = (a00 * b11 - a02 * b08 + a03 * b07) * invDet;
    m->M[6] = (-a30 * b05 + a32 * b02 - a33 * b01) * invDet;
    m->M[7] = (a20 * b05 - a22 * b02 + a23 * b01) * invDet;
    m->M[8] = (a10 * b10 - a11 * b08 + a13 * b06) * invDet;
    m->M[9] = (-a00 * b10 + a01 * b08 - a03 * b06) * invDet;
    m->M[10] = (a30 * b04 - a31 * b02 + a33 * b00) * invDet;
    m->M[11] = (-a20 * b04 + a21 * b02 - a23 * b00) * invDet;
    m->M[12] = (-a10 * b09 + a11 * b07 - a12 * b06) * invDet;
    m->M[13] = (a00 * b09 - a01 * b07 + a02 * b06) * invDet;
    m->M[14] = (-a30 * b03 + a31 * b01 - a32 * b00) * invDet;
    m->M[15] = (a20 * b03 - a21 * b01 + a22 * b00) * invDet;
}

void $Mat44_RotMatrixX(Mat44* m, f32 xDeg)
{
  f32 c = cosf($Deg2Rad(xDeg));
  f32 s = sinf($Deg2Rad(xDeg));

  m->m[0][0] = 1.0f;
  m->m[0][1] = 0.0f;
  m->m[0][2] = 0.0f;
  m->m[0][3] = 0.0f;
  
  m->m[1][0] = 0.0f;
  m->m[1][1] = c;
  m->m[1][2] = -s;
  m->m[1][3] = 0.0f;
  
  m->m[2][0] = 0.0f;
  m->m[2][1] = s;
  m->m[2][2] = c;
  m->m[2][3] = 0.0f;
  
  m->m[3][0] = 0.0f;
  m->m[3][1] = 0.0f;
  m->m[3][2] = 0.0f;
  m->m[3][3] = 1.0f;
}

void $Mat44_RotMatrixY(Mat44* m, f32 yDeg)
{
  f32 c = cosf($Deg2Rad(yDeg));
  f32 s = sinf($Deg2Rad(yDeg));

  m->m[0][0] = c;
  m->m[0][1] = 0.0f;
  m->m[0][2] = s;
  m->m[0][3] = 0.0f;
  
  m->m[1][0] = 0.0f;
  m->m[1][1] = 1.0f;
  m->m[1][2] = 0.0f;
  m->m[1][3] = 0.0f;
  
  m->m[2][0] = -s;
  m->m[2][1] = 0.0f;
  m->m[2][2] = c;
  m->m[2][3] = 0.0f;
  
  m->m[3][0] = 0.0f;
  m->m[3][1] = 0.0f;
  m->m[3][2] = 0.0f;
  m->m[3][3] = 1.0f;
}

void $Mat44_RotMatrixZ(Mat44* m, f32 zDeg)
{
  f32 c = cosf($Deg2Rad(zDeg));
  f32 s = sinf($Deg2Rad(zDeg));

  m->m[0][0] = c;
  m->m[0][1] = -s;
  m->m[0][2] = 0.0f;
  m->m[0][3] = 0.0f;
  
  m->m[1][0] = s;
  m->m[1][1] = c;
  m->m[1][2] = 0.0f;
  m->m[1][3] = 0.0f;
  
  m->m[2][0] = 0.0f;
  m->m[2][1] = 0.0f;
  m->m[2][2] = 1.0f;
  m->m[2][3] = 0.0f;
  
  m->m[3][0] = 0.0f;
  m->m[3][1] = 0.0f;
  m->m[3][2] = 0.0f;
  m->m[3][3] = 1.0f;
}

void $Mat44_RotMatrixZXY(Mat44* m, Rot3i rot)
{
  Mat44 t;
  // x = roll
  // y = yaw
  // z = pitch
  if (rot.pitch != 0)
  {
    $Mat44_RotMatrixZ(&t, rot.pitch);
    $Mat44_Multiply(m, m, &t);
  }

  if (rot.roll != 0)
  {
    $Mat44_RotMatrixX(&t, rot.roll);
    $Mat44_Multiply(m, m, &t);
  }

  if (rot.yaw != 0)
  {
    $Mat44_RotMatrixY(&t, rot.yaw);
    $Mat44_Multiply(m, m, &t);
  }

}

void $Mat44_MultiplyTransform(Mat44* m, Vec3f tr)
{
  static Mat44 t;
  t.M[0] = 1.0f; t.M[1] = 0.0f; t.M[2] = 0.0f; t.M[3] = 0.0f;
  t.M[4] = 0.0f; t.M[5] = 1.0f; t.M[6] = 0.0f; t.M[7] = 0.0f;
  t.M[8] = 0.0f; t.M[9] = 0.0f; t.M[10]= 1.0f; t.M[11]= 0.0f;
  t.M[12]= tr.x; t.M[13]= tr.y; t.M[14]= tr.z; t.M[15]= 1.0f;

  $Mat44_Multiply(m, &t, m);

}

Vec3f $Vec3_Normalise(Vec3f v)
{
  f32 length = $Vec3_Length(v);
  
  if (length != 0.0f)
  {
    f32 r = 1.0f / length;
    v.x *= r;
    v.y *= r;
    v.z *= r;
  }
  
  return v;
}

Vec3f $Vec3_Cross(Vec3f a, Vec3f b)
{
  Vec3f r;
  r.x = (a.y * b.z) - (a.z * b.y);
  r.y = (a.z * b.x) - (a.x * b.z);
  r.z = (a.x * b.y) - (a.y * b.x);
  return r;
}

f32   $Vec3_Length(Vec3f v)
{
  return sqrtf($Vec3_Length2(v));
}

f32   $Vec3_Length2(Vec3f v)
{
  return $Vec3_Dot(v, v);
}

void $Mat44_LookAt(Mat44* m, Vec3f pos, Vec3f target)
{
  Vec3f look  = $Vec3_Normalise($Vec3_Sub(target, pos));
  Vec3f right = $Vec3_Normalise($Vec3_Cross($Vec3_Xyz(0,1,0), look));
  Vec3f up    = $Vec3_Cross(look, right);

  m->m[0][0] = right.x;
  m->m[0][1] = right.y;
  m->m[0][2] = right.z;
  m->m[0][3] = 0.0f;
  
  m->m[1][0] = up.x;
  m->m[1][1] = up.y;
  m->m[1][2] = up.z;
  m->m[1][3] = 0.0f;
  
  m->m[2][0] = look.x;
  m->m[2][1] = look.y;
  m->m[2][2] = look.z;
  m->m[2][3] = 0.0f;
  
  m->m[3][0] = pos.x;
  m->m[3][1] = pos.y;
  m->m[3][2] = pos.z;
  m->m[3][3] = 1.0f;
}

void $$Frustrum_CreateProjection(Vec4f* f, f32 fovy, f32 aspect, f32 nearClip, f32 farClip)
{
  // http://iquilezles.org/www/articles/frustum/frustum.htm

  const f32 s = sinf($Deg2Rad(fovy));
  const f32 c = cosf($Deg2Rad(fovy));

  f[0].x = 0; f[0].y = -c; f[0].z = s;        f[0].w = 0;
  f[1].x = 0; f[1].y =  c; f[1].z = s;        f[1].w = 0;
  f[2].x =-c; f[2].y =  0; f[2].z = s*aspect; f[2].w = 0;
  f[3].x =-c; f[3].y =  0; f[3].z = s*aspect; f[3].w = 0;
  f[4].x = 0; f[4].y =  0; f[4].z = 1;        f[4].w = farClip;
  f[5].x = 0; f[5].y =  0; f[5].z =-1;        f[5].w =-nearClip;
}

bool $$Frustrum_Test(Vec4f* f, Vec3f* min, Vec3f* max)
{
  // http://www.txutxi.com/?p=584
  Vec3f box[2] = { *min, *max };
  
  for(u32 ii=0;ii < 6;ii++)
  {
    Vec4f* p = &f[ii];

    const i32 px = (i32)(p->x > 0.0f);
    const i32 py = (i32)(p->y > 0.0f);
    const i32 pz = (i32)(p->z > 0.0f);

    const f32 dp = (p->x * box[px].x) + (p->y * box[py].y) + (p->z * box[pz].z);

    if (dp < -p->w) 
      return false;

  }
  return true;
}

void* Mem_PermaAllocate(void* mem, u32 numBytes)
{
  void* m = NULL;

  if (mem == NULL && numBytes)
  {
    m = malloc(numBytes);
    memset(m, 0, numBytes);
  }
  else if (mem != NULL && numBytes)
    m =  realloc(mem, numBytes);
  else if (mem != NULL && numBytes == 0)
    free(mem);

  return m;
}

typedef struct
{
  u8* start, *end, *current;
} $$TempAllocatorInfo;

$$TempAllocatorInfo $$TempAllocator;

void $$TempAllocatorSetup(u32 memorySize)
{
  $$TempAllocator.start = (u8*) malloc(memorySize);
  $$TempAllocator.current = $$TempAllocator.start;
  $$TempAllocator.end = $$TempAllocator.start + memorySize;
}

void* Mem_TempAllocate(u32 numBytes)
{
  void* mem = $$TempAllocator.current;
  $$TempAllocator.current += numBytes;
 
  $Assert($$TempAllocator.current < $$TempAllocator.end, "Out of memory in the temporay allocator");
 
  return mem;
}

void $$Net_Init()
{
#if $NETWORK_ENABLED == 1
  i32 r = SDLNet_Init();
  if (r < 0)
  {
    printf("cannot init sdl2 net : %s", SDLNet_GetError());
  }
#endif
}

void $$Net_Shutdown()
{
#if $NETWORK_ENABLED == 1

  if ($$.netSocket != NULL)
  {
    SDLNet_TCP_DelSocket($$.netSocketSet, $$.netSocket);
  }
  
  SDLNet_FreeSocketSet($$.netSocketSet);
  SDLNet_TCP_Close($$.netSocket);

  $$.netSocket = NULL;

  SDLNet_Quit();

#endif
}

bool Net_Connect(const char* address, u16 port)
{
#if $NETWORK_ENABLED == 1

  IPaddress ip;
  if (SDLNet_ResolveHost(&ip, address, port) < 0)
  {
    printf("Unknown address %s:%i", address, port);
  }

  $$.netSocket = SDLNet_TCP_Open(&ip);
  
  if ($$.netSocket == NULL)
  {
    printf("Could not open socket. Reason: %s", SDLNet_GetError());
    return false;
  }

  $$.netSocketSet = SDLNet_AllocSocketSet(1);
  $Assert($$.netSocketSet, "Could not open socket");

  $Assert(SDLNet_TCP_AddSocket($$.netSocketSet, $$.netSocket) != -1, "Could not add socket to socket set");
  
  printf("Socket Opened\n");
  return true;
#else
  return false;
#endif
}

bool Net_PeekMessage()
{
#if $NETWORK_ENABLED == 1
  if ($$.netSocket == NULL)
    return false;

  if (SDLNet_CheckSockets($$.netSocketSet, 0) < 0)
  {
    printf("Check Sockets failed %s\n", SDLNet_GetError());
  }

  return SDLNet_SocketReady($$.netSocket) != 0;
#else
  return false;
#endif
}

bool Net_RecvMessage(u32* dataLength, u32 dataCapacity, void* data)
{
#if $NETWORK_ENABLED == 1
  i32 size = SDLNet_TCP_Recv($$.netSocket, data, dataCapacity);

  if (size < 0)
  {
    printf("Socket error! %i", size);
    return false;
  }

  *dataLength = (u32) size;

  return true;
#else
  return false;
#endif
}

void Net_SendMessage(u32 dataLength, void* data)
{
#if $NETWORK_ENABLED == 1
  if ($$.netSocket != NULL)
  {

    i32 size = SDLNet_TCP_Send($$.netSocket, data, dataLength);
    if (size < 0)
    {
      printf("Socket error!");
    }

    if (size != dataLength)
    {
      printf("Message was not fully sent!");
    }
  }
  else
  {
    printf("Cannot send message to a non-connected socket!");
  }
#endif
}

void $$TempAllocatorReset()
{
  $$TempAllocator.current = $$TempAllocator.start;
}

void $$SetupApi()
{
  $.fixedDeltaTime                  = 0;
  $.time                            = 0;
  $.drawMs                          = 8;
  $.updateMs                        = 16;
  $.title                           = "Synthwave";
  $.width                           = 320;
  $.height                          = 200;
  $.displayScale                    = 4;
  $.quit                            = false;
  $.Palette.Bind                    = Palette_Bind;
  $.Palette.New                     = Palette_New;
  $.Palette.Delete                  = Palette_Delete;
  $.Palette.Append                  = Palette_Append;
  $.Palette.AppendRgb               = Palette_AppendRgb;
  $.Palette.AppendU32               = Palette_AppendU32;
  $.Bitmap.Load                     = Bitmap_Load;
  $.Bitmap.GetSize                  = Bitmap_GetSize;
  $.Surface.New                     = Surface_New;
  $.Surface.Delete                  = Surface_Delete;
  $.Surface.Render                  = Surface_Render;
  $.Canvas.New                      = Canvas_New;
  $.Canvas.Delete                   = Canvas_Delete;
  $.Canvas.Render                   = Canvas_Render;
  $.Canvas.Clear                    = Canvas_Clear;
  $.Canvas.DrawBox                  = Canvas_DrawBox;
  $.Canvas.DrawBoxXywh              = Canvas_DrawBoxXywh;
  $.Canvas.DrawFilledBox            = Canvas_DrawFilledBox;
  $.Canvas.DrawFilledBoxXywh        = Canvas_DrawFilledBoxXywh;
  $.Canvas.DrawLine                 = Canvas_DrawLine;
  $.Canvas.DrawPoint                = Canvas_DrawPoint;
  $.Canvas.DrawBitmap               = Canvas_DrawBitmap;
  $.Canvas.DrawSprite               = Canvas_DrawSprite;
  $.Canvas.DrawSpriteXywh           = Canvas_DrawSpriteXywh;
  $.Canvas.DrawText                 = Canvas_DrawText;
  $.Canvas.DrawTextF                = Canvas_DrawTextF;
  $.Mesh.Finalise                   = Mesh_Finalise;
  $.Scene.New                       = Scene_New;
  $.Scene.Delete                    = Scene_Delete;
  $.Scene.Render                    = Scene_Render;
  $.Scene.Clear                     = Scene_Clear;
  $.Scene.SetPovLookAt              = Scene_SetPovLookAt;
  $.Scene.SetPovLookAtXyz           = Scene_SetPovLookAtXyz;
  $.Scene.DrawSkybox                = Scene_DrawSkybox;
  $.Scene.DrawMesh                  = Scene_DrawMesh;
  $.Scene.DrawMeshXyz               = Scene_DrawMeshXyz;
  $.Scene.DrawCustomShaderMesh      = Scene_DrawCustomShaderMesh;
  $.Scene.DrawCustomShaderMeshXyz   = Scene_DrawCustomShaderMeshXyz;
  $.Sound.New                       = Sound_New;
  $.Sound.Play                      = Sound_Play;
  $.Sound.MuteAll                   = Sound_MuteAll;
  $.Music.Play                      = Music_Play;
  $.Font.New                        = Font_New;
  $.Input.BindControl               = Input_BindControl;
  $.Input.ControlDown               = Input_ControlDown;
  $.Input.ControlPressed            = Input_ControlPressed;
  $.Input.ControlReleased           = Input_ControlReleased;
  $.Timer.New                       = Timer_New;
  $.Timer.Delete                    = Timer_Delete;
  $.Timer.Start                     = Timer_Start;
  $.Timer.Stop                      = Timer_Stop;
  $.Timer.Pause                     = Timer_Pause;
  $.Timer.Unpause                   = Timer_Unpause;
  $.Timer.Ticks                     = Timer_Ticks;
  $.Timer.IsRunning                 = Timer_IsRunning;
  $.Timer.IsPaused                  = Timer_IsPaused;
  $.Mem.PermaAllocator              = Mem_PermaAllocate;
  $.Mem.TempAllocator               = Mem_TempAllocate;
  $.Net.Connect                     = Net_Connect;
  $.Net.PeekMessage                 = Net_PeekMessage;
  $.Net.RecvMessage                 = Net_RecvMessage;
  $.Net.SendMessage                 = Net_SendMessage;
}

static void $$Frame()
{
    SDL_Event event;
    $.frameCount++;
    $.Timer.Start(&$$.frameLimitTimer);
    $$.deltaTimeMs = $.Timer.Ticks(&$$.deltaTimer);

    while (SDL_PollEvent(&event))
    {
      switch(event.type)
      {
        case SDL_QUIT:
        {
          $.quit = true;
        }
        break;
      }
    }
    
    const Uint8 *state = SDL_GetKeyboardState(NULL);
    u32 nbControls = $Array_Size($$.controls);
    for(u32 ii=0;ii < nbControls;ii++)
    {
      $$Control* c = &$$.controls[ii];
      
      i32 key = c->key;
      c->lastState = c->state;
      c->state = 0;

      if (key == $KEY_UNKNOWN || key >= $KEY_NUM_SCANCODES)
        break;

      c->state |= (state[key] != 0) ? 1 : 0;
    }
    
    $.Stats.fps = (f32) ($.frameCount) / (($.Timer.Ticks(&$$.fpsTimer) / 1000.0f));
    if ($.Stats.fps > 200000.0f)
    {
      $.Stats.fps = 0.0f;
    }

    u32 frameTime = (u32) ($$.deltaTimeMs);
    if (frameTime > 250)
      frameTime = 250;
    
    $$.accumulator += frameTime;
    while($$.accumulator >= $.updateMs)
    {
      $.fixedDeltaTime = 1.0f / $.updateMs;
      $Update();
      $$.accumulator -= $.updateMs;
    }

    $.deltaTime = 1.0f / $$.deltaTimeMs;
    $Draw();
    $.Stats.nbDrawCalls = 0;
    
    SDL_RenderPresent($$.renderer);
    int wx, wy;
    SDL_GetWindowPosition($$.window, &wx, &wy);
    
    $$TempAllocatorReset();
    $.Timer.Start(&$$.deltaTimer);
}

i32 main(i32 argc, char** argv)
{
  SDL_Init(SDL_INIT_EVERYTHING);

  memset(&$, 0, sizeof($));
  memset(&$$, 0, sizeof($$));
  $$TempAllocatorSetup(16384);
  $$SetupApi();
  $$.palette = $PermaNew(Palette);
  $.Palette.AppendRgb($$.palette, 0xFF, 0x00, 0xFF);

  
  $.Palette.AppendRgb($$.palette, 0xFF, 0x00, 0xFF);
  $.Palette.AppendU32($$.palette, 0xff17111D); // very_dark_violet
  $.Palette.AppendU32($$.palette, 0xff4e4a4e); // shadowy_lavender
  $.Palette.AppendU32($$.palette, 0xff716E61); // flint
  $.Palette.AppendU32($$.palette, 0xff86949F); // regent_grey
  $.Palette.AppendU32($$.palette, 0xffD7E7D0); // peppermint
  $.Palette.AppendU32($$.palette, 0xff462428); // red_earth
  $.Palette.AppendU32($$.palette, 0xff814D30); // root_beer
  $.Palette.AppendU32($$.palette, 0xffD3494E); // faded_red
  $.Palette.AppendU32($$.palette, 0xffCD7F32); // bronze
  $.Palette.AppendU32($$.palette, 0xffD4A798); // birthday_suit
  $.Palette.AppendU32($$.palette, 0xffE3CF57); // banana
  $.Palette.AppendU32($$.palette, 0xff333366); // deep_koamaru
  $.Palette.AppendU32($$.palette, 0xff5D76CB); // indigo
  $.Palette.AppendU32($$.palette, 0xff7AC5CD); // cadet_blue
  $.Palette.AppendU32($$.palette, 0xff215E21); // hunter_green
  $.Palette.AppendU32($$.palette, 0xff71AA34); // leaf

  $Array_New($$.bitmaps, 8);
  $Array_New($$.surfaces, 8);
  $Array_New($$.scenes, 4);
  $Array_New($$.canvases, 4);
  $Array_New($$.controls, 16);

  $Setup();

  $$.window = SDL_CreateWindow( 
    $.title,
    $.screenX,
    $.screenY,
    $.width * $.displayScale,
    $.height * $.displayScale,
    SDL_WINDOW_SHOWN
  );

  $$.renderer = SDL_CreateRenderer(
    $$.window, 
    -1, 
    SDL_RENDERER_ACCELERATED
  );
  
  $$Sound_Init();
  
  #if $NETWORK_ENABLED == 1
  $$Net_Init();
  #endif

  $.Timer.Start(&$$.deltaTimer);
  $.Timer.Start(&$$.frameLimitTimer);
  $.Timer.Start(&$$.fpsTimer);

  $Start();
  
  #if $IsWindows == 1
  while($.quit == false)
  {
    $$Frame();

    f32 frameTicks = (f32) $.Timer.Ticks(&$$.frameLimitTimer);
    if (frameTicks < $.drawMs)
    {
      SDL_Delay((u32) ($.drawMs - frameTicks));
    }
  }
  #elif $IsBrowser == 1
    emscripten_set_main_loop($$Frame, 0, 1);
  #endif
  
  if ($.quit)
  {
    #if $NETWORK_ENABLED == 1
    $$Net_Shutdown();
    #endif

    $Array_Delete($$.bitmaps);
    $Array_Delete($$.surfaces);
    $Array_Delete($$.scenes);
    $Array_Delete($$.canvases);
    $Array_Delete($$.controls);

    #if ($MUSIC_ENABLED == 1)
      if ($$.musicContext != NULL)
      {
        $PermaDelete($$.musicContext);
      }
    #endif

    memset(&$$, 0, sizeof($$));
    memset(&$, 0, sizeof($));
  
    SDL_Quit();
  }

  return 0;
}

$$Make_FragmentShader(Standard, {
  $$FRAGMENT_SET_COLOUR(c);
});

$$Make_FragmentShader(Shadow, {
  if ((p.x + p.y) % 2)
  {
    $$FRAGMENT_SET_COLOUR(2);
  }
});
