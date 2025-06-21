#include "raylib.h"
#include "raymath.h"
#define RAYGUI_IMPLEMENTATION
#include "raygui.h"
#include <iostream>
#include <format>
#include "ImgConvo.h"
#include <algorithm>
#include <arm_neon.h>

// TODO: investigate reverse button
//
// -- compile for WebAssembly
// -- implement file save (and load, for that matter)
// -- paintable canvas? or pingable? pluckable?
// -- space bar also starts/stops

int main()
{
    SetConfigFlags(FLAG_WINDOW_RESIZABLE);
    InitWindow(1400, 1000, "PicWave");
    SetTargetFPS(100);

    ImgConvolver convolver;
    int height = GetScreenHeight();
    int width = GetScreenWidth();
    Vector2 picStart = {150.f, 15.f};

    Image originalImg;
    std::array<Image, 3> images;
    for (auto &image : images)
    {
        image = LoadImage("resources/possums.png");
        ImageFormat(&image, PIXELFORMAT_UNCOMPRESSED_R8G8B8);
    }
    originalImg = LoadImage("resources/possums.png");
    ImageFormat(&originalImg, PIXELFORMAT_UNCOMPRESSED_R8G8B8);
    Texture displayTexture = LoadTextureFromImage(images[0]);
    size_t curImage{0};

    float alphaSq{.12f}; // input values tied to GUI controls

    // SetTraceLogLevel(LOG_ALL);
    bool running = false;
    char *lo = "";
    char *hi = "";
    int s1v{-2}, s2v{8}, s3v{7}, s4v{1}; // tuned to -O3
    bool s1e{false}, s2e{false}, s3e{false}, s4e{false}, reflect{true}, firstRun{true};

    // set FTZ flag
    intptr_t mask = (1 << 24 /* FZ */);
    intptr_t fpsr;
    asm volatile("mrs %0, fpcr" : "=r"(fpsr));
    fpsr |= mask;
    asm volatile("msr fpcr, %0" : : "ri"(fpsr));

    while (!WindowShouldClose())
    {
        // check for resize
        int newheight = GetScreenHeight();
        int newwidth = GetScreenWidth();
        if (newheight != height || newwidth != width)
        {
            width = newwidth;
            height = newheight;
        }

        BeginDrawing();
        ClearBackground(BLACK);

        if (IsFileDropped())
        {
            FilePathList droppedFiles = LoadDroppedFiles();
            TraceLog(LOG_WARNING, "got file: %s\n", droppedFiles.paths[0]);
            if (IsFileExtension(droppedFiles.paths[0], ".png") ||
                IsFileExtension(droppedFiles.paths[0], ".jpg") ||
                IsFileExtension(droppedFiles.paths[0], ".jpeg"))
            {
                curImage = 0;
                firstRun = true;
                UnloadImage(images[0]);
                UnloadImage(images[1]);
                UnloadImage(images[2]);
                UnloadImage(originalImg);
                images[0] = LoadImage(droppedFiles.paths[0]);
                if (images[curImage].width > width - picStart.x)
                {
                    float aspectRatio =
                        float(images[curImage].width) / float(images[curImage].height);
                    ImageResize(&images[curImage], width - picStart.x - 5,
                                (width - picStart.x - 5) / aspectRatio);
                }
                if (images[curImage].height > height - picStart.y)
                {
                    float aspectRatio =
                        float(images[curImage].width) / float(images[curImage].height);
                    ImageResize(&images[curImage], (height - picStart.y - 5) * aspectRatio,
                                height - picStart.y - 5);
                }
                ImageFormat(&images[curImage], PIXELFORMAT_UNCOMPRESSED_R8G8B8);
                originalImg = ImageCopy(images[0]);
                images[1] = ImageCopy(images[0]); // set prev 2 to copy
                images[2] = ImageCopy(images[0]);
                displayTexture = LoadTextureFromImage(images[curImage]);
                UpdateTexture(displayTexture, images[curImage].data);
            }
            UnloadDroppedFiles(droppedFiles);
        }

        if (GuiButton((Rectangle){25, 25, 100, 25}, "Apply") || IsKeyReleased(KEY_A))
        {
            running = !running;
        }
        if (running)
        {
            curImage = (curImage + 1) % 3;
            int last = (curImage == 0) ? 2 : curImage - 1;
            int penult = (curImage == 2) ? 0 : (curImage == 1) ? 2 : 1;
            convolver.convolve(images[curImage], images[last], images[penult], alphaSq, firstRun,
                               reflect);
            if (firstRun)
            {
                firstRun = false;
            }
            UpdateTexture(displayTexture, images[curImage].data);
        }

        float low{0.01f}, high{0.175f};
        GuiSlider((Rectangle){25, 125, 100, 25}, lo, hi, &alphaSq, low, high);
        DrawText(TextFormat("%1.4f", alphaSq), 25, 155, 25, RAYWHITE);

        GuiToggle((Rectangle){25, 190, 100, 25}, "Reflections", &reflect);
        // if (GuiSpinner((Rectangle){25, 245, 75, 25}, nullptr, &s1v, -35, -1, s1e))
        // {
        //     s1e = !s1e; // negative exponent
        // }
        // if (GuiSpinner((Rectangle){25, 285, 75, 25}, nullptr, &s2v, 0, 99, s2e))
        // {
        //     s2e = !s2e; // 1st significant digit (or 1st and second...)
        // }
        // if (GuiSpinner((Rectangle){25, 325, 75, 25}, nullptr, &s3v, 0, 99, s3e))
        // {
        //     s3e = !s3e; // 2nd significant digit
        // }
        // if (GuiSpinner((Rectangle){25, 365, 75, 25}, nullptr, &s4v, 0, 99, s4e))
        // {
        //     s4e = !s4e; // 3rd significant digit
        // }

        if (GuiButton((Rectangle){25, 75, 100, 25}, "Original") || IsKeyReleased(KEY_O))
        {
            curImage = 0;
            UnloadImage(images[0]);
            UnloadImage(images[1]);
            UnloadImage(images[2]);
            images[0] = ImageCopy(originalImg);
            images[1] = ImageCopy(originalImg);
            images[2] = ImageCopy(originalImg);
            UnloadTexture(displayTexture);
            displayTexture = LoadTextureFromImage(originalImg);
            UpdateTexture(displayTexture, originalImg.data);
            running = false;
            firstRun = true;
        }

        DrawFPS(25, 400);

        DrawTexture(displayTexture, picStart.x, picStart.y, WHITE);
        EndDrawing();
    }

    UnloadTexture(displayTexture); // Unload texture from VRAM
    UnloadImage(originalImg);      // Unload image-origin from RAM
    for (auto &image : images)
    {
        UnloadImage(image); // Unload image-copy from RAM
    }
    CloseWindow();
    return 0;
}

// wasm compilation:

// em++ -std=c++20 -o game.html src/main.cpp -Os -Wall \
// -I ~/dev/emsdk/upstream/emscripten/cache/sysroot/include -I./src  -I/usr/local/include \
// -L ~/dev/emsdk/upstream/emscripten/cache/sysroot/lib/libraylib.a -s USE_GLFW=3 \
// -s ASYNCIFY -s ALLOW_MEMORY_GROWTH=1 --preload-file resources \
// --shell-file minshell.html -DPLATFORM_WEB ~/dev/emsdk/upstream/emscripten/cache/sysroot/lib/libraylib.a
//

// app compilation for local use:

// g++ -std=c++20 -o game src/main.cpp -Os -Wall -I./src -I/opt/homebrew/Cellar/raylib/5.5/include
// -L/opt/homebrew/Cellar/raylib/5.5/lib -lraylib