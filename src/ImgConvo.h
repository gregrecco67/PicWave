#include "raylib.h"
#include <stdlib.h>
#include <math.h>
#include <memory>
#include <algorithm>

typedef struct Int3
{
    int x;
    int y;
    int z;
} Int3;

typedef struct UChar3
{
    unsigned char x;
    unsigned char y;
    unsigned char z;
} UChar3; // used internally in c2

class ImgConvolver
{
  public:
    ImgConvolver();

    // Int3 ColorToInt(Vector3 color);
    // Vector3 NormalizeColor(Color color);
    void convolve(Image &u0, Image &u1, Image &u2, float newAlpha, bool first, bool reflect);
    Vector3 c2(Image &img, const int idx) const;
    Vector3 c3(Image &img, const int i, const int j) const;

    // two-step convolve
    // load starting image, copy into 3 w/upsampled color
    // convolve, rotating

    float alpha{0}, alphaSq{0};
    int w{0}, h{0}, s{0};
    float kQuot{0};

    Color *u0;
    Color *u1;
    Color *u2;
};