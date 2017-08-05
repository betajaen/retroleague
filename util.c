#include "funk.h"

f32 constrainAngle(f32 x)
{
    x = fmodf(x, 360.0f);
    if (x < 0.0f)
        x += 360.0f;
    return x;
}
