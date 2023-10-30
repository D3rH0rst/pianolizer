#include <stdlib.h>
#include <string.h>
#include <assert.h>
#ifdef _WIN32
#include "../WinDependencies/include/raylib.h"
#else
#include <raylib.h>
#endif
#include "plug.h"

#define N_KEYS 88
#define N_WHITE_KEYS 52
#define N_BLACK_KEYS 36
#define KEYS_IN_OCTAVE 12
#define BLACK_GROUPS 15
#define FIRST_KEY 9
#define SCROLL_RECT_CAP 10
#define RECT_SPEED 100
#define KEY_SCROLL_RECT_OFFSET 5
float padding = 1.0f;

float small_offset;

float whiteKey_width;
float whiteKey_height;

float blackKey_width;
float blackKey_height;

Vector2 prevMousePos = {0};
typedef struct {
    Rectangle rect;
    bool finished;
} ScrollRect;

typedef struct {
    ScrollRect* scrollRects;
    size_t size;
    size_t capacity;
} ScrollRects;

typedef struct {
    size_t index;
    size_t color_index;
    size_t key_oct;
    bool pressed;
    bool white;
    Rectangle key_rect;
    ScrollRects scroll_rects;
} Key;

typedef struct {
    // Key stuff
    Key keys[N_KEYS];
    Key* white_keys[N_WHITE_KEYS];
    Key* black_keys[N_BLACK_KEYS];
} Plug;

static Plug* p = NULL;


void init_sr_array(ScrollRects* arr, size_t initialCapacity) {
    arr->scrollRects = malloc(initialCapacity * sizeof(ScrollRect));
    arr->size = 0;
    arr->capacity = initialCapacity;
}

void append_scroll_rect(ScrollRects* arr, ScrollRect sr) {
    if (arr->size == arr->capacity) {
        arr->capacity *= 2;
        arr->scrollRects = realloc(arr->scrollRects, arr->capacity * sizeof(ScrollRect));
    }
    arr->scrollRects[arr->size++] = sr;
}

void delete_scroll_rect(ScrollRects* arr, size_t index) {
    if (index >= arr->size) {
        TraceLog(LOG_WARNING, "Attempted to delete an invalid index %ld from array with size %ld", index, arr->size);
        return;
    }

    // Shift elements after index one position to the left
    for (size_t i = index; i < arr->size - 1; i++) {
        arr->scrollRects[i] = arr->scrollRects[i + 1];
    }

    // Decrease the size of the array
    arr->size--;
}

void insert_scroll_rect(ScrollRects* arr, size_t index, ScrollRect sr) {
    if (index > arr->size) {
        // Invalid index
        return;
    }

    if (arr->size == arr->capacity) {
        arr->capacity *= 2;
        arr->scrollRects = realloc(arr->scrollRects, arr->capacity * sizeof(ScrollRect));
    }

    // Shift elements after index one position to the right
    for (size_t i = arr->size; i > index; i--) {
        arr->scrollRects[i] = arr->scrollRects[i - 1];
    }

    // Insert the new rectangle at the desired index
    arr->scrollRects[index] = sr;

    // Increase the size of the array
    arr->size++;
}

bool is_white(size_t key_octave) {
    if (key_octave < 5)
        return key_octave % 2 == 0;
    return key_octave % 2 != 0;
}

void* plug_pre_reload(void) {
    return p;
}

void plug_post_reload(Plug* pP) {
    p = pP;
}

void calculate_key_rects() {
    whiteKey_width = ((float)GetScreenWidth() - (padding * (N_WHITE_KEYS - 1))) / N_WHITE_KEYS;
    whiteKey_height = whiteKey_width * 7;

    blackKey_width = whiteKey_width * 0.6f;
    blackKey_height = whiteKey_height * 0.75f;

    small_offset = whiteKey_width + padding;
    for (size_t i = 0; i < N_KEYS; i++) {
        if (p->keys[i].white) {
            p->keys[i].key_rect.x = (float) p->keys[i].color_index * (whiteKey_width + padding);
            p->keys[i].key_rect.y = (float) GetScreenHeight() - whiteKey_height;
            p->keys[i].key_rect.width = whiteKey_width;
            p->keys[i].key_rect.height = whiteKey_height;

        } else {
            p->keys[i].key_rect.x = (p->keys[i - 1].color_index + 1) * small_offset - blackKey_width / 2.f;
            p->keys[i].key_rect.y = (float) GetScreenHeight() - whiteKey_height;
            p->keys[i].key_rect.width = blackKey_width;
            p->keys[i].key_rect.height = blackKey_height;
        }
    }
}

void plug_init(void) {
    p = malloc(sizeof(*p));
    assert(p != NULL && "Buy more RAM lol");
    memset(p, 0, sizeof(*p));

    whiteKey_width = ((float)GetScreenWidth() - (padding * (N_WHITE_KEYS - 1))) / N_WHITE_KEYS;
    whiteKey_height = whiteKey_width * 7;

    blackKey_width = whiteKey_width * 0.6f;
    blackKey_height = whiteKey_height * 0.75f;

    small_offset = whiteKey_width + padding;

    size_t black_index = 0;
    size_t white_index = 0;
    for (size_t i = 0; i < N_KEYS; i++) {
        p->keys[i].index = i;
        p->keys[i].pressed = false;
        p->keys[i].key_oct = (i + 9) % KEYS_IN_OCTAVE;
        if (is_white(p->keys[i].key_oct)) {
            p->keys[i].white = true;
            p->keys[i].color_index = white_index;

            // set up the key rectangle (needed for mouse hit detection)
            p->keys[i].key_rect.x = (float)p->keys[i].color_index * (whiteKey_width + padding);
            p->keys[i].key_rect.y = (float)GetScreenHeight() - whiteKey_height;
            p->keys[i].key_rect.width = whiteKey_width;
            p->keys[i].key_rect.height = whiteKey_height;

            p->white_keys[white_index] = &p->keys[i];
            white_index++;
        } else {
            p->keys[i].white = false;
            p->keys[i].color_index = black_index;

            // key rect
            p->keys[i].key_rect.x = (p->keys[i - 1].color_index + 1) * small_offset - blackKey_width / 2.f;
            p->keys[i].key_rect.y = (float)GetScreenHeight() - whiteKey_height;
            p->keys[i].key_rect.width = blackKey_width;
            p->keys[i].key_rect.height = blackKey_height;

            p->black_keys[black_index] = &p->keys[i];
            black_index++;
        }
        init_sr_array(&p->keys[i].scroll_rects, SCROLL_RECT_CAP);
    }
}

void render_key(Key* key) {
    Color topColor;
    Color bottomColor;

    if (key->white) {
        topColor = WHITE;
        bottomColor = key->pressed ? GRAY : topColor;
        DrawRectangleGradientV(key->key_rect.x, key->key_rect.y, key->key_rect.width, key->key_rect.height, topColor, bottomColor);
    } else {
        topColor = CLITERAL(Color){ 50, 50, 50, 255 };
        bottomColor = key->pressed ? BLACK : topColor;
        DrawRectangleGradientV(key->key_rect.x, key->key_rect.y, key->key_rect.width, key->key_rect.height, topColor, bottomColor);
    }
}

void render_keys() {
    for (size_t i = 0; i < N_WHITE_KEYS; i++) {
        render_key(p->white_keys[i]);          // Render all white keys before all black keys to avoid overlapping
    }

    for (size_t i = 0; i < N_BLACK_KEYS; i++) {
        render_key(p->black_keys[i]);
    }
}

void update_keys() {
    bool black_pressed = false;
    ScrollRect sr = {0};

    if (IsMouseButtonDown(MOUSE_BUTTON_LEFT)) {
        for (size_t i = 0; i < N_BLACK_KEYS; i++) {
            if (CheckCollisionPointRec(GetMousePosition(), p->black_keys[i]->key_rect)) {
                if (!p->black_keys[i]->pressed) {
                    p->black_keys[i]->pressed = true;
                    // Create the Scroll Rect
                    sr.finished = false;
                    sr.rect.width = blackKey_width;
                    sr.rect.height = 1;
                    sr.rect.x = p->black_keys[i]->key_rect.x;
                    sr.rect.y = p->black_keys[i]->key_rect.y - sr.rect.height - KEY_SCROLL_RECT_OFFSET;
                    append_scroll_rect(&p->black_keys[i]->scroll_rects, sr);
                }
                black_pressed = true;                           // prevent white key being pressed through black key
            } else {
                p->black_keys[i]->pressed = false;
            }
        }
        for (size_t i = 0; i < N_WHITE_KEYS; i++) {
            if (black_pressed) {
                p->white_keys[i]->pressed = false;
            } else {
                if (CheckCollisionPointRec(GetMousePosition(), p->white_keys[i]->key_rect)) {
                    if (!p->white_keys[i]->pressed) {
                        p->white_keys[i]->pressed = true;
                        // Create the Scroll Rect
                        sr.finished = false;
                        sr.rect.width = whiteKey_width;
                        sr.rect.height = 1;
                        sr.rect.x = p->white_keys[i]->key_rect.x;
                        sr.rect.y = p->white_keys[i]->key_rect.y - sr.rect.height - KEY_SCROLL_RECT_OFFSET;
                        append_scroll_rect(&p->white_keys[i]->scroll_rects, sr);
                    }
                } else {
                    p->white_keys[i]->pressed = false;
                }
            }
        }
    } else {
        for (size_t i = 0; i < N_KEYS; i++) {
            p->keys[i].pressed = false;
        }
    }
}

void render_scroll_rects() {
    for (size_t i = 0; i < N_WHITE_KEYS; i++) {
        for (size_t j = 0; j < p->white_keys[i]->scroll_rects.size; j++) {
            // DrawRectangleRec(p->white_keys[i]->scroll_rects.scrollRects[j].rect, GREEN);
            DrawRectangleRounded(p->white_keys[i]->scroll_rects.scrollRects[j].rect, 0.3, 5, GREEN);
        }
    }
    for (size_t i = 0; i < N_BLACK_KEYS; i++) {
        for (size_t j = 0; j < p->black_keys[i]->scroll_rects.size; j++) {
            // DrawRectangleRec(p->black_keys[i]->scroll_rects.scrollRects[j].rect, DARKGREEN);
            DrawRectangleRounded(p->black_keys[i]->scroll_rects.scrollRects[j].rect, 0.3, 5, DARKGREEN);
        }
    }
}

void update_scroll_rects() {
    float dt = GetFrameTime();
    float offset = RECT_SPEED * dt;
    size_t n_scroll_rects = 0;
    ScrollRect* current_rects = NULL;

    for (size_t i = 0; i < N_KEYS; i++) {
        n_scroll_rects = p->keys[i].scroll_rects.size;
        current_rects = p->keys[i].scroll_rects.scrollRects;
        if (!p->keys[i].pressed) {
            if (n_scroll_rects > 0) {
                current_rects[n_scroll_rects - 1].finished = true;
            }
        } else {
            current_rects[n_scroll_rects - 1].rect.height += offset;
        }

        for (size_t j = 0; j < n_scroll_rects; j++) {
            current_rects[j].rect.y -= offset;

            if (current_rects[j].finished) {
                if (current_rects[j].rect.y + current_rects[j].rect.height < 0) {
                    delete_scroll_rect(&p->keys[i].scroll_rects, j);
                }
            }
        }
    }
}

void plug_update(void) {
    BeginDrawing();
        ClearBackground(DARKGRAY);
        render_keys();
        update_keys();
        render_scroll_rects();
        update_scroll_rects();
    EndDrawing();

    if (IsWindowResized()) {
        calculate_key_rects();
    }
}