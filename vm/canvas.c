
#include "./canvas.h"

#if VM_USE_CANVAS
#if !defined(EMSCRIPTEN)
#error canvas only works with emscripten/emcc
#endif
#include <emscripten.h>

void InitWindow(int width, int height, const char *title) {
    SetWindowSize(width, height);
}
void CloseWindow(void) {}

int vm_canvas_target_fps = 60;

void SetTargetFPS(int fps) {
    vm_canvas_target_fps = fps;
}

bool WindowShouldClose(void) {
    return false;
}

void BeginDrawing(void) {}
void EndDrawing(void) {}

void ClearBackground(Color color) {
    DrawRectangleRec((Rectangle) {0, 0, GetScreenWidth(), GetScreenHeight()}, color);
}

EM_JS(void, vm_canvas_rect, (float x, float y, float w, float h, uint8_t r, uint8_t g, uint8_t b, uint8_t a), {
    Module._vm_canvas_draw_rect(x, y, w, h, r, g, b, a);
})

void DrawRectangleRec(Rectangle rect, Color color) {
    vm_canvas_rect(rect.x, rect.y, rect.width, rect.height, color.r, color.g, color.b, color.a);
}

bool CheckCollisionPointRec(Vector2 point, Rectangle rec) {
    return (point.x >= rec.x) && (point.x < (rec.x + rec.width)) && (point.y >= rec.y) && (point.y < (rec.y + rec.height));
}

Vector2 GetMousePosition(void) {
    return (Vector2) {
        EM_ASM_DOUBLE(return Module._vm_canvas_mouse_x()),
        EM_ASM_DOUBLE(return Module._vm_canvas_mouse_y()),
    };
}

EM_JS(bool, vm_canvas_mouse_down, (int button), {
    return Module._vm_canvas_mouse_pressed(button);
})

EM_JS(bool, vm_canvas_mouse_pressed, (int button), {
    return !Module._vm_canvas_mouse_pressed_last(button) && Module._vm_canvas_mouse_pressed(button);
})

bool IsMouseButtonDown(int button) {
    return vm_canvas_mouse_down(button);
}
bool IsMouseButtonPressed(int button) {
    return vm_canvas_mouse_pressed(button);
}

void SetTraceLogLevel(int level) {}

EM_JS(bool, vm_canvas_key_down, (int key), {
    return Module._vm_canvas_key_pressed(key);
});
EM_JS(bool, vm_canvas_key_up, (int key), {
    return !Module._vm_canvas_key_pressed(key);
});
EM_JS(bool, vm_canvas_key_pressed, (int key), {
    return !Module._vm_canvas_key_pressed_last(key) && Module._vm_canvas_key_pressed(key);
});
EM_JS(bool, vm_canvas_key_released, (int key), {
    return Module._vm_canvas_key_pressed_last(key) && !Module._vm_canvas_key_pressed(key);
});

bool IsKeyDown(int key) {
    return vm_canvas_key_down(key);
}
bool IsKeyUp(int key) {
    return vm_canvas_key_up(key);
}
bool IsKeyPressed(int key) {
    return vm_canvas_key_pressed(key);
}
bool IsKeyReleased(int key) {
    return vm_canvas_key_released(key);
}

EM_JS(bool, vm_canvas_set_size, (int w, int h), {
    return Module._vm_canvas_set_size(w, h);
});

static int vm_canvas_set_size_x = 0;
static int vm_canvas_set_size_y = 0;

int GetScreenWidth(void) {
    return vm_canvas_set_size_x;
}
int GetScreenHeight(void) {
    return vm_canvas_set_size_y;
}
void SetWindowSize(int width, int height) {
    vm_canvas_set_size_x = width;
    vm_canvas_set_size_y = height;
    vm_canvas_set_size(width, height);
}

#endif
