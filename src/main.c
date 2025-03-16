#include <math.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>

#include "ext_vector.h"
#include "raylib.h"
#include "raymath.h"

#define G                30
#define TARGET_FPS       60
#define SIMULATION_STEPS 600

typedef struct CelestialBody {
    Vector2 position, prev_position;
    Vector2 velocity;
    Vector2 force;
    float density, radius;
    Color color;
} CelestialBody;

static CelestialBody* bodies;
static Vector2 mouse_pressed_pos;
static CelestialBody spawned_body;
static Vector2* path;

static void integrate_position(CelestialBody* b, float dt) {
    float m = b->density * b->radius * b->radius;
    b->prev_position = b->position;
    b->position = Vector2Add(b->position, Vector2Scale(b->velocity, dt));
    b->velocity = Vector2Add(b->velocity, Vector2Scale(b->force, dt / m));
}

static void update(float dt) {
    // Compute gravitational forces
    // F = G * (m1 * m2 / r^2)
    for(size_t i = 0; i < vec_size(bodies); i++) {
        CelestialBody* b1 = &bodies[i];
        float m1 = b1->density * b1->radius * b1->radius;
        for(size_t j = i + 1; j < vec_size(bodies); j++) {
            CelestialBody* b2 = &bodies[j];

            Vector2 r = Vector2Subtract(b2->position, b1->position);
            float r2 = fmax(Vector2LengthSqr(r), 1e-2);
            r = Vector2Scale(r, 1 / sqrt(r2));

            float m2 = b2->density * b2->radius * b2->radius;
            Vector2 f = Vector2Scale(r, G * m1 * m2 / r2);
            b1->force = Vector2Add(b1->force, f);
            b2->force = Vector2Subtract(b2->force, f);
        }
    }

    // Integrate using simplectic Euler
    vec_foreach(CelestialBody * b, bodies) {
        integrate_position(b, dt);
    }

    // Reset forces
    vec_foreach(CelestialBody * b, bodies) {
        b->force = (Vector2){0};
    }
}

static float GetRandomUniform() {
    return (GetRandomValue(0, RAND_MAX) / (float)RAND_MAX);
}

static void spawn_body() {
    if(IsMouseButtonPressed(MOUSE_BUTTON_LEFT)) {
        mouse_pressed_pos = GetMousePosition();
        Vector2 dir = Vector2Subtract(GetMousePosition(), mouse_pressed_pos);
        spawned_body = (CelestialBody){
            .position = mouse_pressed_pos,
            .velocity = dir,
            .radius = fmax(GetRandomUniform() * 30, 10),
            .density = fmax(GetRandomUniform() * 10, 1),
            .color =
                {
                    .r = GetRandomValue(0, 255),
                    .g = GetRandomValue(0, 255),
                    .b = GetRandomValue(0, 255),
                    .a = 255,
                },
        };
    }

    if(IsMouseButtonReleased(MOUSE_BUTTON_LEFT)) {
        spawned_body.position = (Vector2){0};
        spawned_body.velocity = Vector2Subtract(GetMousePosition(), mouse_pressed_pos);
        spawned_body.force = (Vector2){0};
        vec_push_back(bodies, spawned_body);
    }

    if(IsMouseButtonDown(MOUSE_BUTTON_LEFT)) {
        spawned_body.velocity = Vector2Subtract(GetMousePosition(), mouse_pressed_pos);
        vec_clear(path);

        float m1 = spawned_body.density * spawned_body.radius * spawned_body.radius;
        for(size_t step = 0; step < 10000; step++) {
            const float dt = 1.0 / SIMULATION_STEPS;
            for(size_t i = 0; i < vec_size(bodies); i++) {
                CelestialBody* o = &bodies[i];

                Vector2 r = Vector2Subtract(o->position, spawned_body.position);
                float r2 = fmax(Vector2LengthSqr(r), 1e-2);
                r = Vector2Scale(r, 1 / sqrt(r2));

                float m2 = o->density * o->radius * o->radius;
                Vector2 f = Vector2Scale(r, G * m1 * m2 / r2);
                spawned_body.force = Vector2Add(spawned_body.force, f);
            }
            integrate_position(&spawned_body, dt);
            spawned_body.force = (Vector2){0};
            vec_push_back(path, spawned_body.position);
        }
    }
}

static void draw(float alpha) {
    BeginDrawing();

    ClearBackground(RAYWHITE);
    DrawText(TextFormat("FPS: %d\n", GetFPS()), 0, 0, 30, BLACK);

    vec_foreach(const CelestialBody* b, bodies) {
        DrawCircleV(Vector2Lerp(b->prev_position, b->position, alpha), b->radius, b->color);
    }

    if(IsMouseButtonDown(MOUSE_BUTTON_LEFT)) {
        DrawLineEx(mouse_pressed_pos, GetMousePosition(), 4, RED);
    }

    for(int i = 0; i < (int)vec_size(path) - 1; i++) {
        DrawLineEx(path[i], path[i + 1], 4, BLUE);
    }

    EndDrawing();
}

int main(void) {
    InitWindow(0, 0, "raylib [core] example - basic window");
    ToggleFullscreen();
    SetTargetFPS(TARGET_FPS);

    const int width = GetScreenWidth(), height = GetScreenHeight();

    CelestialBody local_bodies[] = {
        {
            .position = (Vector2){width / 2., height / 2.},
            .velocity = (Vector2){0, 0},
            .density = 100,
            .radius = 100,
            .color = ORANGE,
        },
        // {
        //     .position = (Vector2){width / 2. + 500, height / 2.},
        //     .velocity = (Vector2){0, 3 * TARGET_FPS},
        //     .density = 1,
        //     .radius = 10,
        //     .color = BLUE,
        // },
        // {
        //     .position = (Vector2){width / 2. - 500, height / 2.},
        //     .velocity = (Vector2){0, -3 * TARGET_FPS},
        //     .radius = 10,
        //     .density = 2,
        //     .color = RED,
        // },
        // {
        //     .position = (Vector2){width / 2., height / 2. + 600},
        //     .velocity = (Vector2){3 * TARGET_FPS, 0},
        //     .radius = 20,
        //     .density = 30,
        //     .color = GREEN,
        // },
    };
    vec_push_back_all(bodies, local_bodies, sizeof(local_bodies) / sizeof(local_bodies[0]));

    float acc = 0;
    const float sub_dt = 1. / SIMULATION_STEPS;
    while(!WindowShouldClose()) {
        float dt = GetFrameTime();
        acc += dt;

        float alpha = 0;
        if(dt != 0) {
            while(acc >= sub_dt) {
                update(sub_dt);
                acc -= sub_dt;
            }
            alpha = acc / sub_dt;
        }

        spawn_body();
        draw(alpha);
    }

    CloseWindow();
    vec_free(bodies);
}
