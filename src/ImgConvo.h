#pragma once

#include "raylib.h"
#include "raymath.h"
#include <stdlib.h>
#include <math.h>
#include <memory>
#include <algorithm>
#include <vector>

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
    explicit ImgConvolver(Image &);

    // Int3 ColorToInt(Vector3 color);
    // Vector3 NormalizeColor(Color color);
    void convolve(Image &u0, Image &u1, Image &u2, float newAlpha, bool first, bool reflect);
    Vector3 c2(Image &img, const int idx) const;
    Vector3 c3(Image &img, const int i, const int j) const;

    float c4(std::vector<float> &img, const int i, const int j) const;

    // two-step convolve
    // load starting image, copy into 3 w/upsampled color
    void loadImage(Image &u1);
    // convolve, rotating
    void convolve(float newAlpha, float fudgeFactor);

    float alpha{0}, alphaSq{0};
    int w{0}, h{0}, s{0};
    float kQuot{0};

    std::vector<float> img0R, img1R, img2R;
    std::vector<float> img0G, img1G, img2G;
    std::vector<float> img0B, img1B, img2B;
    std::array<std::reference_wrapper<std::vector<float>>, 3> reds;
    std::array<std::reference_wrapper<std::vector<float>>, 3> grns;
    std::array<std::reference_wrapper<std::vector<float>>, 3> blus;
    int curImage{0}; // as in, "the one we're currently writing into being"

    Image &u0out; // when we're done with internal processing, this is where we write

    Color *u0;
    Color *u1;
    Color *u2;
};