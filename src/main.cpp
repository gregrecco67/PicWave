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
// -- video save? ffmpeg available on wasm?
// -- paintable canvas? or pingable? pluckable?

// -- time since start
// -- circular field?
// -- next order kernel
// -- SIMD
// -- give up on "radiating boundary"?
// - fix file loading/unloading! [dropped image works, but nothing else]
// -- make fudge factor a slider
// -- give sliders names

// -- reflect is just absorb at 0, get rid of button
// -- new mode treating orig image colors as 0, allowing painting/plucking?

int main()
{
    SetConfigFlags(FLAG_WINDOW_RESIZABLE);
    InitWindow(1400, 1000, "PicWave");
    SetTargetFPS(100);

    int height = GetScreenHeight();
    int width = GetScreenWidth();
    Vector2 picStart = {150.f, 15.f};

    // HYPOTHESIS we can replaces all references to images with images[0]
    // or we could just have originalImg and currentImg

    Image originalImg;
    Image currentImg;
    size_t frame{0};
    currentImg = LoadImage("resources/possums.png");
    ImageFormat(&currentImg, PIXELFORMAT_UNCOMPRESSED_R8G8B8);
    originalImg = ImageCopy(currentImg);
    ImageFormat(&originalImg, PIXELFORMAT_UNCOMPRESSED_R8G8B8);
    ImgConvolver convolver(currentImg);
    convolver.loadImage(currentImg);
    TraceLog(LOG_WARNING, "Convolver loaded image.");
    // -------------

    Texture displayTexture = LoadTextureFromImage(currentImg);

    float alpha{0.1f}; // input values tied to GUI controls
    float fudge{0.0f}; //

    // SetTraceLogLevel(LOG_ALL);
    bool running = false;
    bool saving{false}, loading{false};
    char filename[256]{0};
    float speedLow{0.0015f}, speedHigh{0.175f};
    float fudgeLow{0.0f}, fudgeHigh{1.0f};

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
                frame = 0;
                currentImg = LoadImage(droppedFiles.paths[0]);
                if (currentImg.width > width - picStart.x)
                {
                    float aspectRatio = float(currentImg.width) / float(currentImg.height);
                    ImageResize(&currentImg, width - picStart.x - 5,
                                (width - picStart.x - 5) / aspectRatio);
                }
                if (currentImg.height > height - picStart.y)
                {
                    float aspectRatio = float(currentImg.width) / float(currentImg.height);
                    ImageResize(&currentImg, (height - picStart.y - 5) * aspectRatio,
                                height - picStart.y - 5);
                }
                ImageFormat(&currentImg, PIXELFORMAT_UNCOMPRESSED_R8G8B8);
                originalImg = ImageCopy(currentImg);
                convolver.loadImage(currentImg);
                displayTexture = LoadTextureFromImage(currentImg);
                UpdateTexture(displayTexture, currentImg.data);
            }
            UnloadDroppedFiles(droppedFiles);
        }

        if (IsKeyReleased(KEY_S))
        {
            saving = true;
            running = false;
        }

        if (IsKeyReleased(KEY_F)) // advance by one frame
        {
            running = false;
            frame += 1;
            convolver.convolve(alpha, fudge);
            UpdateTexture(displayTexture, currentImg.data);
        }

        if (GuiButton((Rectangle){25, 25, 100, 25}, "Apply") || IsKeyReleased(KEY_A) ||
            IsKeyReleased(KEY_SPACE))
        {
            running = !running;
        }

        if (GuiButton((Rectangle){25, 75, 100, 25}, "Original") || IsKeyReleased(KEY_O))
        {
            frame = 0;
            currentImg = ImageCopy(originalImg);
            convolver.loadImage(currentImg);
            UnloadTexture(displayTexture);
            displayTexture = LoadTextureFromImage(originalImg);
            UpdateTexture(displayTexture, originalImg.data);
            running = false;
        }

        DrawText("Speed", 25, 125, 18, RAYWHITE);
        GuiSlider((Rectangle){25, 150, 100, 25}, NULL, NULL, &alpha, speedLow, speedHigh);
        DrawText(TextFormat("%1.4f", alpha), 25, 180, 25, RAYWHITE);

        // GuiToggle((Rectangle){25, 215, 100, 25}, "Reflections", &reflect);

        // auto textCol = (reflect) ? DARKGRAY : RAYWHITE;
        DrawText("Absorb", 25, 265, 18, RAYWHITE);
        GuiSlider((Rectangle){25, 295, 100, 25}, NULL, NULL, &fudge, fudgeLow, fudgeHigh);
        DrawText(TextFormat("%1.4f", fudge), 25, 325, 25, RAYWHITE);

        DrawFPS(25, 400);
        DrawText(TextFormat("%d", frame), 25, 450, 25, RAYWHITE);

        if (running)
        {
            frame += 1;
            convolver.convolve(alpha, fudge);
            UpdateTexture(displayTexture, currentImg.data);
            DrawText("Running", 25, 355, 25, RAYWHITE);
        }

        DrawTexture(displayTexture, picStart.x, picStart.y, WHITE);

        if (saving)
        {
            int clk = GuiTextInputBox(Rectangle{200, 0, 500, 300}, "Save", "Save File", "OK;Cancel",
                                      filename, 255, NULL);
            if (clk == 1)
            {
                Image image = LoadImageFromTexture(displayTexture);
                ExportImage(image, filename);
                UnloadImage(image);
                saving = false;
            }
            if (clk == 2)
            {
                saving = false;
            }
            if (clk == 0)
            {
                saving = false;
            }
            // set
        }

        EndDrawing();
    }

    UnloadTexture(displayTexture); // Unload texture from VRAM
    UnloadImage(originalImg);      // Unload image-origin from RAM
    UnloadImage(currentImg);       // Unload image-current from RAM
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