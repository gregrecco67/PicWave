#include "ImgConvo.h"

ImgConvolver::ImgConvolver() : kQuot((alpha - 1) / (alpha + 1)) {}

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
            const int ij = i * w + j;
            const int ijm2 = ij - 2;
            const int ijm1 = ij - 1;
            const int ijp1 = ij + 1;
            const int ijp2 = ij + 2;
            const int im2j = (i - 2) * w + j;
            const int im1j = (i - 1) * w + j;
            const int ip1j = (i + 1) * w + j;
            const int ip2j = (i + 2) * w + j;

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
