#if !defined(VM_HEADER_CANVAS)
#define VM_HEADER_CANVAS

#include "lib.h"

typedef struct {
    float x;
    float y;
} Vector2;

typedef struct {
    float x;
    float y;
    float width;
    float height;
} Rectangle;

typedef struct {
    uint8_t r;
    uint8_t g;
    uint8_t b;
    uint8_t a;
} Color;

enum {
    LOG_ALL = 0,
    LOG_TRACE,
    LOG_DEBUG,
    LOG_INFO,
    LOG_WARNING,
    LOG_ERROR,
    LOG_FATAL,
    LOG_NONE
};

enum {
    MOUSE_LEFT_BUTTON    = 0,
    MOUSE_RIGHT_BUTTON   = 1,
    MOUSE_MIDDLE_BUTTON  = 2,
    MOUSE_SIDE_BUTTON    = 3,
    MOUSE_EXTRA_BUTTON   = 4,
    MOUSE_FORWARD_BUTTON = 5,
    MOUSE_BACK_BUTTON    = 6,
};

void InitWindow(int width, int height, const char *title);
void CloseWindow(void);
int GetScreenHeight(void);
int GetScreenWidth(void);

void SetTargetFPS(int fps);
bool WindowShouldClose(void);

void BeginDrawing(void);
void EndDrawing(void);

bool CheckCollisionPointRec(Vector2 point, Rectangle rec);

void ClearBackground(Color color);
void DrawRectangleRec(Rectangle rect, Color color);

Vector2 GetMousePosition(void);
bool IsMouseButtonDown(int button);
bool IsMouseButtonPressed(int button);

void SetTraceLogLevel(int level);

bool IsKeyDown(int key);
bool IsKeyUp(int key);
bool IsKeyPressed(int key);
bool IsKeyReleased(int key);

void SetWindowSize(int width, int height);

#define BLACK    (Color) { 0, 0, 0, 255 } 
#define RAYWHITE (Color) { 245, 245, 245, 255 }

#endif