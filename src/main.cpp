#include "raylib.h"
#include "raymath.h"
#define RAYGUI_IMPLEMENTATION
#include "raygui.h"
#include <iostream>
#include <format>
#include "ImgConvo.h"
#include <algorithm>
#include <arm_neon.h>

// TODO:
//
// -- implement file save (and load, for that matter)
// -- video save? ffmpeg available on wasm?
// -- paintable canvas? or pingable? pluckable?

// -- circular field?
// -- next order kernel? (probably not worth the squeeze, but maybe try anyway!)
// -- SIMD

// -- new mode treating orig image colors as 0, allowing painting/plucking?
// -- drawing idea: drawable barriers: spin up an array of int8_t
// -- the array can be sized so that we can skip the bounds check in c4 and
// -- just put a bunch of zeroes out there and then multiply the image buffer value
// -- by 0 or 1

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
    currentImg = LoadImage("resources/circle2.png");
    ImageFormat(&currentImg, PIXELFORMAT_UNCOMPRESSED_R8G8B8);
    originalImg = ImageCopy(currentImg);
    ImageFormat(&originalImg, PIXELFORMAT_UNCOMPRESSED_R8G8B8);
    ImgConvolver convolver(currentImg);
    convolver.loadImage(currentImg);
    TraceLog(LOG_WARNING, "Convolver loaded image.");

    Font font = LoadFontEx("resources/lato-regular.otf", 22, 0, 0);
    float fontSize = (float)font.baseSize;
    SetTextureFilter(font.texture, TEXTURE_FILTER_BILINEAR);
    GuiSetFont(font);
    GuiSetStyle(BUTTON, TEXT_COLOR_NORMAL, 0x000000ff);
    GuiSetStyle(BUTTON, TEXT_SIZE, 18);
    // -------------

    Texture displayTexture = LoadTextureFromImage(currentImg);

    float alpha{0.1f};  // input values tied to GUI controls
    float absorb{0.0f}; //

    // SetTraceLogLevel(LOG_ALL);
    bool running{false}, saving{false}, loading{false}, infoPanel{false};
    char filename[256]{0};
    float speedLow{0.0015f}, speedHigh{0.175f};
    float absorbLow{0.0f}, absorbHigh{1.0f};

    // hide when not arm64-NEON
    // set FTZ flag
    // intptr_t mask = (1 << 24 /* FZ */);
    // intptr_t fpsr;
    // asm volatile("mrs %0, fpcr" : "=r"(fpsr));
    // fpsr |= mask;
    // asm volatile("msr fpcr, %0" : : "ri"(fpsr));

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

        if (IsFileDropped())
        {
            FilePathList droppedFiles = LoadDroppedFiles();
            TraceLog(LOG_WARNING, "got file: %s\n", droppedFiles.paths[0]);
            if (IsFileExtension(droppedFiles.paths[0], ".png") ||
                IsFileExtension(droppedFiles.paths[0], ".jpg") ||
                IsFileExtension(droppedFiles.paths[0], ".jpeg"))
            {
                frame = 0;
                saving = false;
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

        if ((IsKeyReleased(KEY_SLASH) || IsKeyReleased(KEY_H)) && !saving)
        {
            infoPanel = !infoPanel;
        }

        if (IsKeyReleased(KEY_S) && !saving)
        {
            saving = true;
            running = false;
        }

        if (IsKeyReleased(KEY_F) && !saving) // advance by one frame
        {
            running = false;
            frame += 1;
            convolver.convolve(alpha, absorb);
            UpdateTexture(displayTexture, currentImg.data);
        }

        BeginDrawing();
        ClearBackground(BLACK);

        if ((GuiButton((Rectangle){25, 25, 100, 25}, "Apply") || IsKeyReleased(KEY_A) ||
             IsKeyReleased(KEY_SPACE)) &&
            !saving)
        {
            running = !running;
        }

        if ((GuiButton((Rectangle){25, 75, 100, 25}, "Original") || IsKeyReleased(KEY_O)) &&
            !saving)
        {
            frame = 0;
            currentImg = ImageCopy(originalImg);
            convolver.loadImage(currentImg);
            UnloadTexture(displayTexture);
            displayTexture = LoadTextureFromImage(originalImg);
            UpdateTexture(displayTexture, originalImg.data);
            running = false;
        }

        DrawTextEx(font, "Speed", {25, 125}, fontSize, 0, RAYWHITE);
        GuiSlider((Rectangle){25, 150, 100, 25}, NULL, NULL, &alpha, speedLow, speedHigh);
        DrawTextEx(font, TextFormat("%1.4f", alpha), {25, 180}, fontSize, 0, RAYWHITE);

        DrawTextEx(font, "Absorb", {25, 215}, fontSize, 0, RAYWHITE);
        GuiSlider((Rectangle){25, 245, 100, 25}, NULL, NULL, &absorb, absorbLow, absorbHigh);
        DrawTextEx(font, TextFormat("%1.4f", absorb), {25, 275}, fontSize, 0, RAYWHITE);

        DrawFPS(25, 400);
        DrawTextEx(font, "Frame:", {25, 450}, fontSize, 0, RAYWHITE);
        DrawTextEx(font, TextFormat("%d", frame), {25, 475}, fontSize, 0, RAYWHITE);

        if (running)
        {
            frame += 1;
            convolver.convolve(alpha, absorb);
            UpdateTexture(displayTexture, currentImg.data);
            DrawTextEx(font, "Running", {25, 355}, 25, 0, RAYWHITE);
        }

        DrawTexture(displayTexture, picStart.x, picStart.y, WHITE);

        if (saving)
        {
            int clk = GuiTextInputBox(Rectangle{200, 0, 500, 300}, "Save", "Save Image",
                                      "OK;Cancel", filename, 255, NULL);
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

        if (infoPanel)
        {
            DrawRectangle(0, 0, GetScreenWidth(), GetScreenHeight(), Fade(RAYWHITE, 0.8f));
            DrawTextEx(font, "Drag and drop PNG or JPG", {100, 100}, 30, 0, BLACK);
            DrawTextEx(font, "A | SPC === start/stop", {100, 135}, 30, 0, BLACK);
            DrawTextEx(font, "O === original image", {100, 170}, 30, 0, BLACK);
            DrawTextEx(font, "F === advance 1 frame", {100, 205}, 30, 0, BLACK);
        }
        DrawTextEx(font, "? = help", {25, 600}, fontSize, 0, GRAY);

        EndDrawing();
    }

    UnloadTexture(displayTexture); // Unload texture from VRAM
    UnloadImage(originalImg);      // Unload image-origin from RAM
    UnloadImage(currentImg);       // Unload image-current from RAM
    CloseWindow();
    return 0;
}

// wasm compilation:

// em++ -std=c++20 -o app.html src/main.cpp src/ImgConvo.cpp -O3 -Wall \
// -I ~/dev/emsdk/upstream/emscripten/cache/sysroot/include -I./src  -I/usr/local/include \
// -L ~/dev/emsdk/upstream/emscripten/cache/sysroot/lib/libraylib.a -s USE_GLFW=3 \
// -s ASYNCIFY -s ALLOW_MEMORY_GROWTH=1 --preload-file resources \
// --shell-file src/minshell.html -DPLATFORM_WEB ~/dev/emsdk/upstream/emscripten/cache/sysroot/lib/libraylib.a
//

// app compilation for local use:

// g++ -std=c++20 -o game src/main.cpp -Os -Wall -I./src -I/opt/homebrew/Cellar/raylib/5.5/include
// -L/opt/homebrew/Cellar/raylib/5.5/lib -lraylib