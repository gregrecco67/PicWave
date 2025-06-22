#include "ImgConvo.h"

ImgConvolver::ImgConvolver(Image &img)
    : kQuot((alpha - 1) / (alpha + 1)), u0out(img), reds({img0R, img1R, img2R}),
      grns({img0G, img1G, img2G}), blus({img0B, img1B, img2B})
{
    img0R.reserve(500000);
    img0G.reserve(500000);
    img0B.reserve(500000);
    img1R.reserve(500000);
    img1G.reserve(500000);
    img1B.reserve(500000);
    img2R.reserve(500000);
    img2G.reserve(500000);
    img2B.reserve(500000);
}

Vector3 ImgConvolver::c2(Image &img, const int idx) const
{
    if (idx < 0 || idx >= s)
    {
        return {128, 128, 128};
    }
    else
    {
        UChar3 uchar = static_cast<UChar3 *>(img.data)[idx];
        return {(uchar.x / 255.f) - 0.5f, (uchar.y / 255.f) - 0.5f, (uchar.z / 255.f) - 0.5f};
    }
}

Vector3 ImgConvolver::c3(Image &img, const int i, const int j) const
{
    if (i < 0 || i >= h || j < 0 || j >= w)
    {
        return {128, 128, 128};
    }
    else
    {
        UChar3 uchar = static_cast<UChar3 *>(img.data)[i * w + j];
        return {(uchar.x / 255.f) - 0.5f, (uchar.y / 255.f) - 0.5f, (uchar.z / 255.f) - 0.5f};
    }
}

float ImgConvolver::c4(std::vector<float> &vec, const int i, const int j) const
{
    if (i < 0 || i >= h || j < 0 || j >= w)
    {
        return 0.0f;
    }
    else
    {
        return vec[i * w + j];
    }
}

void ImgConvolver::loadImage(Image &img)
{
    u0out = img;
    h = u0out.height;
    w = u0out.width;
    s = w * h;

    for (auto &image : reds)
    {
        image.get().clear();
    }
    for (auto &image : grns)
    {
        image.get().clear();
    }
    for (auto &image : blus)
    {
        image.get().clear();
    }

    for (int i = 0; i < s; ++i)
    {
        UChar3 u = static_cast<UChar3 *>(u0out.data)[i];
        img0R.push_back((u.x / 255.f) - 0.5f);
        img0G.push_back((u.y / 255.f) - 0.5f);
        img0B.push_back((u.z / 255.f) - 0.5f);
        img1R.push_back((u.x / 255.f) - 0.5f);
        img1G.push_back((u.y / 255.f) - 0.5f);
        img1B.push_back((u.z / 255.f) - 0.5f);
        img2R.push_back((u.x / 255.f) - 0.5f);
        img2G.push_back((u.y / 255.f) - 0.5f);
        img2B.push_back((u.z / 255.f) - 0.5f);
    }
    TraceLog(LOG_WARNING, "Convolver filled buffers: size = %d = %d.", s, img0R.size());
    curImage = 0;
}

void ImgConvolver::convolve(float newAlpha, bool reflect, bool firstRun)
{
    alpha = newAlpha;
    alphaSq = alpha * alpha;
    kQuot = (alpha - 1) / (alpha + 1);
    float fudge = 0.25f;
    UChar3 *u0d = static_cast<UChar3 *>(u0out.data);
    // indices for the current, last, and penultimate images
    curImage = (curImage + 1) % 3;
    int c = curImage;
    int l = (curImage == 0) ? 2 : curImage - 1;
    int p = (curImage == 2) ? 0 : (curImage == 1) ? 2 : 1;

    if (reflect || firstRun)
    {
        for (int col = 0; col < w; ++col)
        {
            const int zj = col;
            const int Nj = (h - 1) * w + col;
            img0R[zj] = 0;
            img0R[Nj] = 0;
            img0G[zj] = 0;
            img0G[Nj] = 0;
            img0B[zj] = 0;
            img0B[Nj] = 0;
            img1R[zj] = 0;
            img1R[Nj] = 0;
            img1G[zj] = 0;
            img1G[Nj] = 0;
            img1B[zj] = 0;
            img1B[Nj] = 0;
            img2R[zj] = 0;
            img2R[Nj] = 0;
            img2G[zj] = 0;
            img2G[Nj] = 0;
            img2B[zj] = 0;
            img2B[Nj] = 0;
        }

        // left and right
        for (int row = 0; row < h; ++row)
        {
            const int i0 = row * w;
            const int iN = row * w + (w - 1);
            img0R[i0] = 0;
            img0R[iN] = 0;
            img0G[i0] = 0;
            img0G[iN] = 0;
            img0B[i0] = 0;
            img0B[iN] = 0;
            img1R[i0] = 0;
            img1R[iN] = 0;
            img1G[i0] = 0;
            img1G[iN] = 0;
            img1B[i0] = 0;
            img1B[iN] = 0;
            img2R[i0] = 0;
            img2R[iN] = 0;
            img2G[i0] = 0;
            img2G[iN] = 0;
            img2B[i0] = 0;
            img2B[iN] = 0;
        }
        if (firstRun)
        {
            firstRun = false;
        }
    } // handle reflections and first run boundaries (gray)
    for (int i = 1; i < h - 1; ++i)
    {
        for (int j = 1; j < w - 1; ++j)
        {
            // clang-format off
            float x = 
                (alphaSq * (-c4(reds[l].get(), i, j-2) + 16 * c4(reds[l].get(), i, j-1) - c4(reds[l].get(), i-2, j) +
                    16 * c4(reds[l].get(), i-1, j) - 60 * c4(reds[l].get(), i, j) + 16 * c4(reds[l].get(), i+1, j) - c4(reds[l].get(), i+2, j) +
                    16 * c4(reds[l].get(), i, j+1) - c4(reds[l].get(), i, j+2)) + 2 * c4(reds[l].get(), i, j) - c4(reds[p].get(), i, j));
            float y = 
                (alphaSq * (-c4(grns[l].get(), i, j-2) + 16 * c4(grns[l].get(), i, j-1) - c4(grns[l].get(), i-2, j) +
                    16 * c4(grns[l].get(), i-1, j) - 60 * c4(grns[l].get(), i, j) + 16 * c4(grns[l].get(), i+1, j) - c4(grns[l].get(), i+2, j) +
                    16 * c4(grns[l].get(), i, j+1) - c4(grns[l].get(), i, j+2)) + 2 * c4(grns[l].get(), i, j) - c4(grns[p].get(), i, j));
            float z = 
                (alphaSq * (-c4(blus[l].get(), i, j-2) + 16 * c4(blus[l].get(), i, j-1) - c4(blus[l].get(), i-2, j) +
                    16 * c4(blus[l].get(), i-1, j) - 60 * c4(blus[l].get(), i, j) + 16 * c4(blus[l].get(), i+1, j) - c4(blus[l].get(), i+2, j) +
                    16 * c4(blus[l].get(), i, j+1) - c4(blus[l].get(), i, j+2)) + 2 * c4(blus[l].get(), i, j) - c4(blus[p].get(), i, j));
            int ij = i * w + j;
            reds[c].get()[ij] = x;
            grns[c].get()[ij] = y;
            blus[c].get()[ij] = z;
            // x = (x + 0.5f) * 255;
            // y = (y + 0.5f) * 255;
            // z = (z + 0.5f) * 255;
            // x = (x <= 0) ? 0.f : (x >= 255) ? 255 : x;
            // y = (y <= 0) ? 0.f : (y >= 255) ? 255 : y;
            // z = (z <= 0) ? 0.f : (z >= 255) ? 255 : z;
            // clang-format on
            u0d[ij].x = static_cast<unsigned char>(Clamp((x + 0.5f) * 255, 0, 255));
            u0d[ij].y = static_cast<unsigned char>(Clamp((y + 0.5f) * 255, 0, 255));
            u0d[ij].z = static_cast<unsigned char>(Clamp((z + 0.5f) * 255, 0, 255));
            // TraceLog(LOG_WARNING, "x = %f \t y = %f", x, y);
        }
    }
    // TraceLog(LOG_WARNING, "Convolver traced image.");

    // absorbing boundary
    if (!reflect)
    {
        float twoKp1 = 2 / (alpha + 1);
        for (int col = 0; col < w; ++col)
        {

            // const int zj = col;
            // const int Nj = (h - 1) * w + col;
            // float u0zjx = -c2(u2, zj + w).x + kQuot * (c2(u0, zj + w).x - c2(u2, zj).x) +
            //               twoKp1 * (c2(u1, zj).x - c2(u1, zj + w).x);
            // float u0Njx = -c2(u2, Nj - w).x + kQuot * (c2(u0, Nj - w).x - c2(u2, Nj).x) +
            //               twoKp1 * (c2(u1, Nj).x - c2(u1, Nj - w).x);

            // clang-format off
            const int zj = col;
            const int Nj = (h - 1) * w + col;
            float u0zjx = c4(reds[l].get(), 1, col) + kQuot * (c4(reds[c].get(), 1, col) - c4(reds[l].get(), 0, col));
            float u0Njx = c4(reds[l].get(), h - 2, col) + kQuot * (c4(reds[c].get(), h - 2, col) - c4(reds[l].get(), h - 1, col));
            float u0zjy = c4(grns[l].get(), 1, col) + kQuot * (c4(grns[c].get(), 1, col) - c4(grns[l].get(), 0, col));
            float u0Njy = c4(grns[l].get(), h - 2, col) + kQuot * (c4(grns[c].get(), h - 2, col) - c4(grns[l].get(), h - 1, col));
            float u0zjz = c4(blus[l].get(), 1, col) + kQuot * (c4(blus[c].get(), 1, col) - c4(blus[l].get(), 0, col));
            float u0Njz = c4(blus[l].get(), h - 2, col) + kQuot * (c4(blus[c].get(), h - 2, col) - c4(blus[l].get(), h - 1, col));
            // clang-format on
            reds[c].get()[zj] = Clamp(u0zjx * fudge, -.5f, .5f);
            grns[c].get()[zj] = Clamp(u0zjy * fudge, -.5f, .5f);
            blus[c].get()[zj] = Clamp(u0zjz * fudge, -.5f, .5f);
            u0d[zj].x = static_cast<unsigned char>(Clamp((u0zjx + 0.5f) * 255, 0, 255));
            u0d[zj].y = static_cast<unsigned char>(Clamp((u0zjy + 0.5f) * 255, 0, 255));
            u0d[zj].z = static_cast<unsigned char>(Clamp((u0zjz + 0.5f) * 255, 0, 255));

            reds[c].get()[Nj] = Clamp(u0Njx * fudge, -.5f, .5f);
            grns[c].get()[Nj] = Clamp(u0Njy * fudge, -.5f, .5f);
            blus[c].get()[Nj] = Clamp(u0Njz * fudge, -.5f, .5f);

            u0d[Nj].x = static_cast<unsigned char>(Clamp((u0Njx + 0.5f) * 255, 0, 255));
            u0d[Nj].y = static_cast<unsigned char>(Clamp((u0Njy + 0.5f) * 255, 0, 255));
            u0d[Nj].z = static_cast<unsigned char>(Clamp((u0Njz + 0.5f) * 255, 0, 255));
        }

        // left and right
        for (int i = 0; i < h; ++i)
        {
            const int i0 = i * w;
            const int iN = i * w + (w - 1);
            // clang-format off
            float u0i0x = c4(reds[l].get(), i, 1) + kQuot * (c4(reds[c].get(), i, 1) - c4(reds[l].get(), i, 0));
            float u0iNx = c4(reds[l].get(), i, w - 2) + kQuot * (c4(reds[c].get(), i, w - 2) - c4(reds[l].get(), i, w - 1));
            float u0i0y = c4(grns[l].get(), i, 1) + kQuot * (c4(grns[c].get(), i, 1) - c4(grns[l].get(), i, 0));
            float u0iNy = c4(grns[l].get(), i, w - 2) + kQuot * (c4(grns[c].get(), i, w - 2) - c4(grns[l].get(), i, w - 1));
            float u0i0z = c4(blus[l].get(), i, 1) + kQuot * (c4(blus[c].get(), i, 1) - c4(blus[l].get(), i, 0));
            float u0iNz = c4(blus[l].get(), i, w - 2) + kQuot * (c4(blus[c].get(), i, w - 2) - c4(blus[l].get(), i, w - 1));
            // clang-format on
            u0d[i0].x = static_cast<unsigned char>(Clamp((u0i0x + 0.5f) * 255, 0, 255));
            u0d[i0].y = static_cast<unsigned char>(Clamp((u0i0y + 0.5f) * 255, 0, 255));
            u0d[i0].z = static_cast<unsigned char>(Clamp((u0i0z + 0.5f) * 255, 0, 255));

            u0d[iN].x = static_cast<unsigned char>(Clamp((u0iNx + 0.5f) * 255, 0, 255));
            u0d[iN].y = static_cast<unsigned char>(Clamp((u0iNy + 0.5f) * 255, 0, 255));
            u0d[iN].z = static_cast<unsigned char>(Clamp((u0iNz + 0.5f) * 255, 0, 255));

            reds[c].get()[i0] = Clamp(u0i0x * fudge, -.5f, .5f);
            grns[c].get()[i0] = Clamp(u0i0y * fudge, -.5f, .5f);
            blus[c].get()[i0] = Clamp(u0i0z * fudge, -.5f, .5f);

            reds[c].get()[iN] = Clamp(u0iNx * fudge, -.5f, .5f);
            grns[c].get()[iN] = Clamp(u0iNy * fudge, -.5f, .5f);
            blus[c].get()[iN] = Clamp(u0iNz * fudge, -.5f, .5f);
        }
    }
    ImageFormat(&u0out, PIXELFORMAT_UNCOMPRESSED_R8G8B8);
}

void ImgConvolver::convolve(Image &u0, Image &u1, Image &u2, float newAlpha, bool firstRun,
                            bool reflect)
{
    // u1 is incoming, u0 is output, u2 is previous
    h = u1.height;
    w = u1.width;
    s = h * w; // size now in Int3 units
    alpha = newAlpha;
    alphaSq = newAlpha * newAlpha;
    kQuot = (newAlpha - 1) / (newAlpha + 1);

    // top and bottom row
    UChar3 *u0d = static_cast<UChar3 *>(u0.data);
    UChar3 *u1d = static_cast<UChar3 *>(u1.data);
    UChar3 *u2d = static_cast<UChar3 *>(u2.data);
    if (reflect || firstRun)
    {
        for (int col = 0; col < w - 1; ++col)
        {
            const int zj = col;
            const int Nj = (h - 1) * w + col;
            if (reflect && !firstRun)
            {
                u0d[zj] = {128, 128, 128};
                u0d[Nj] = {128, 128, 128};
            }
            if (firstRun)
            {
                u0d[zj] = {128, 128, 128};
                u0d[Nj] = {128, 128, 128};
                u1d[zj] = {128, 128, 128};
                u2d[zj] = {128, 128, 128};
                u1d[Nj] = {128, 128, 128};
                u2d[Nj] = {128, 128, 128};
            }
        }

        // left and right
        for (int row = 0; row < h - 1; ++row)
        {
            const int i0 = row * w;
            const int iN = row * w + (w - 1);
            if (reflect && !firstRun)
            {
                u0d[i0] = {128, 128, 128};
                u0d[iN] = {128, 128, 128};
            }
            if (firstRun)
            {
                u0d[i0] = {128, 128, 128};
                u0d[iN] = {128, 128, 128};
                u1d[i0] = {128, 128, 128};
                u1d[iN] = {128, 128, 128};
                u2d[i0] = {128, 128, 128};
                u2d[iN] = {128, 128, 128};
            }
        }
        if (firstRun)
        {
            firstRun = false;
        }
    }

    for (int i = 1; i < h - 1; ++i)
    {
        for (int j = 1; j < w - 1; ++j)
        {
            // clang-format off
            float x = 
                ((alphaSq * (-c3(u1, i, j-2).x + 16 * c3(u1, i, j-1).x - c3(u1, i-2, j).x +
                    16 * c3(u1, i-1, j).x - 60 * c3(u1, i, j).x + 16 * c3(u1, i+1, j).x - c3(u1, i+2, j).x +
                    16 * c3(u1, i, j+1).x - c3(u1, i, j+2).x) + 2 * c3(u1, i, j).x - c3(u2, i, j).x) * 255.f + 128);
            float y = 
                ((alphaSq * (-c3(u1, i, j-2).y + 16 * c3(u1, i, j-1).y - c3(u1, i-2, j).y +
                    16 * c3(u1, i-1, j).y - 60 * c3(u1, i, j).y + 16 * c3(u1, i+1, j).y - c3(u1, i+2, j).y +
                    16 * c3(u1, i, j+1).y - c3(u1, i, j+2).y) + 2 * c3(u1, i, j).y - c3(u2, i, j).y) * 255.f + 128);
            float z = 
                ((alphaSq * (-c3(u1, i, j-2).z + 16 * c3(u1, i, j-1).z - c3(u1, i-2, j).z +
                    16 * c3(u1, i-1, j).z - 60 * c3(u1, i, j).z + 16 * c3(u1, i+1, j).z - c3(u1, i+2, j).z +
                    16 * c3(u1, i, j+1).z - c3(u1, i, j+2).z) + 2 * c3(u1, i, j).z - c3(u2, i, j).z) * 255.f + 128);
            x = (x <= 0) ? 0.f : (x >= 255) ? 255 : x;
            y = (y <= 0) ? 0.f : (y >= 255) ? 255 : y;
            z = (z <= 0) ? 0.f : (z >= 255) ? 255 : z;
            // clang-format on
            int ij = i * w + j;
            u0d[ij].x = static_cast<unsigned char>(x);
            u0d[ij].y = static_cast<unsigned char>(y);
            u0d[ij].z = static_cast<unsigned char>(z);
        }
    }

    // absorbing boundary
    if (!reflect)
    {
        float twoKp1 = 2 / (alpha + 1);
        for (int col = 0; col < w - 1; ++col)
        {
            const int zj = col;
            const int Nj = (h - 1) * w + col;
            float u0zjx = -c2(u2, zj + w).x + kQuot * (c2(u0, zj + w).x - c2(u2, zj).x) +
                          twoKp1 * (c2(u1, zj).x - c2(u1, zj + w).x);
            float u0Njx = -c2(u2, Nj - w).x + kQuot * (c2(u0, Nj - w).x - c2(u2, Nj).x) +
                          twoKp1 * (c2(u1, Nj).x - c2(u1, Nj - w).x);
            float u0zjy = -c2(u2, zj + w).y + kQuot * (c2(u0, zj + w).y - c2(u2, zj).y) +
                          twoKp1 * (c2(u1, zj).y - c2(u1, zj + w).y);
            float u0Njy = -c2(u2, Nj - w).y + kQuot * (c2(u0, Nj - w).y - c2(u2, Nj).y) +
                          twoKp1 * (c2(u1, Nj).y - c2(u1, Nj - w).y);
            float u0zjz = -c2(u2, zj + w).z + kQuot * (c2(u0, zj + w).z - c2(u2, zj).z) +
                          twoKp1 * (c2(u1, zj).z - c2(u1, zj + w).z);
            float u0Njz = -c2(u2, Nj - w).z + kQuot * (c2(u0, Nj - w).z - c2(u2, Nj).z) +
                          twoKp1 * (c2(u1, Nj).z - c2(u1, Nj - w).z);
            u0d[zj].x = static_cast<unsigned char>(u0zjx);
            u0d[zj].y = static_cast<unsigned char>(u0zjy);
            u0d[zj].z = static_cast<unsigned char>(u0zjz);

            u0d[Nj].x = static_cast<unsigned char>(u0Njx);
            u0d[Nj].y = static_cast<unsigned char>(u0Njy);
            u0d[Nj].z = static_cast<unsigned char>(u0Njz);
        }

        // left and right
        for (int row = 0; row < h - 1; ++row)
        {
            const int i0 = row * w;
            const int iN = row * w + (w - 1);

            float u0i0x = -c2(u2, i0 + 1).x + kQuot * (c2(u0, i0 + 1).x - c2(u2, i0).x) +
                          twoKp1 * (c2(u1, i0).x - c2(u1, i0 + 1).x);
            float u0iNx = -c2(u2, iN - 1).x + kQuot * (c2(u0, iN - 1).x - c2(u2, iN).x) +
                          twoKp1 * (c2(u1, iN).x - c2(u1, iN - 1).x);
            float u0i0y = -c2(u2, i0 + 1).y + kQuot * (c2(u0, i0 + 1).y - c2(u2, i0).y) +
                          twoKp1 * (c2(u1, i0).y - c2(u1, i0 + 1).y);
            float u0iNy = -c2(u2, iN - 1).y + kQuot * (c2(u0, iN - 1).y - c2(u2, iN).y) +
                          twoKp1 * (c2(u1, iN).y - c2(u1, iN - 1).y);
            float u0i0z = -c2(u2, i0 + 1).z + kQuot * (c2(u0, i0 + 1).z - c2(u2, i0).z) +
                          twoKp1 * (c2(u1, i0).z - c2(u1, i0 + 1).z);
            float u0iNz = -c2(u2, iN - 1).z + kQuot * (c2(u0, iN - 1).z - c2(u2, iN).z) +
                          twoKp1 * (c2(u1, iN).z - c2(u1, iN - 1).z);
            u0d[i0].x = static_cast<unsigned char>(u0i0x);
            u0d[i0].y = static_cast<unsigned char>(u0i0y);
            u0d[i0].z = static_cast<unsigned char>(u0i0z);

            u0d[iN].x = static_cast<unsigned char>(u0iNx);
            u0d[iN].y = static_cast<unsigned char>(u0iNy);
            u0d[iN].z = static_cast<unsigned char>(u0iNz);
        }
    }
}
