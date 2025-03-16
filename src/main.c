#include <math.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>

#include "ext_vector.h"
#include "raylib.h"
#include "raymath.h"

#define G                30
#define TARGET_FPS       60
#define SIMULATION_STEPS 120
#define PATH_POINTS      10000

typedef struct CelestialBody {
    Vector2 position, prev_position;
    Vector2 velocity;
    Vector2 force;
    float radius, inv_mass;
    Color color;
} CelestialBody;

static CelestialBody* bodies;
static Vector2 mouse_pressed_pos;
static CelestialBody spawned_body;
static Vector2 spawn_path[PATH_POINTS];
static bool show_spawn_path = false;

static CelestialBody create_body(Vector2 position, Vector2 velocity, float density, float radius,
                                 Color color) {
    return (CelestialBody){
        .position = position,
        .velocity = velocity,
        .radius = radius,
        .color = color,
        .inv_mass = 1 / (density * radius * radius),
    };
}

static void integrate_pos(CelestialBody* b, float dt) {
    b->prev_position = b->position;
    b->position = Vector2Add(Vector2Add(b->position, Vector2Scale(b->velocity, dt)),
                             Vector2Scale(b->force, dt * dt * b->inv_mass * 0.5));
}

static void integrate_vel(CelestialBody* b, float dt) {
    // a = F / m
    b->velocity = Vector2Add(b->velocity, Vector2Scale(b->force, dt * 0.5 * b->inv_mass));
}

static Vector2 compute_gravitational_force(const CelestialBody* b1, const CelestialBody* b2) {
    // F = G * (m1 * m2 / r^2)
    Vector2 r = Vector2Subtract(b2->position, b1->position);
    float r2 = fmax(Vector2LengthSqr(r), 1e-2);
    r = Vector2Scale(r, 1 / sqrt(r2));
    return Vector2Scale(r, G / (b1->inv_mass * b2->inv_mass) / r2);
}

static void update(float dt) {
    // Integrate positions using velocity Verlet
    vec_foreach(CelestialBody* b, bodies) {
        integrate_pos(b, dt);
    }

    // Compute gravitational forces
    for(size_t i = 0; i < vec_size(bodies); i++) {
        CelestialBody* b1 = &bodies[i];
        for(size_t j = i + 1; j < vec_size(bodies); j++) {
            CelestialBody* b2 = &bodies[j];
            Vector2 f = compute_gravitational_force(b1, b2);
            b1->force = Vector2Add(b1->force, f);
            b2->force = Vector2Subtract(b2->force, f);
        }
    }

    // Integrate velocities using velocity Verlet
    vec_foreach(CelestialBody* b, bodies) {
        integrate_vel(b, dt);
    }

    // Reset forces
    vec_foreach(CelestialBody* b, bodies) {
        b->force = (Vector2){0};
    }
}

static float GetRandomUniform() {
    return (float)GetRandomValue(0, RAND_MAX) / (float)RAND_MAX;
}

static void spawn_body() {
    if(IsMouseButtonPressed(MOUSE_BUTTON_LEFT)) {
        show_spawn_path = true;
        mouse_pressed_pos = GetMousePosition();
        Vector2 vel = Vector2Subtract(GetMousePosition(), mouse_pressed_pos);
        Color col = {
            .r = GetRandomValue(0, 255),
            .g = GetRandomValue(0, 255),
            .b = GetRandomValue(0, 255),
            .a = 255,
        };
        float density = fmax(GetRandomUniform() * 20, 1);
        float radius = fmax(GetRandomUniform() * 60, 20);
        spawned_body = create_body(mouse_pressed_pos, vel, density, radius, col);
    }

    if(IsMouseButtonReleased(MOUSE_BUTTON_LEFT)) {
        show_spawn_path = false;
        spawned_body.velocity = Vector2Subtract(GetMousePosition(), mouse_pressed_pos);
        vec_push_back(bodies, spawned_body);
    }

    // Compute the path of the spawned body
    if(IsMouseButtonDown(MOUSE_BUTTON_LEFT)) {
        CelestialBody b = spawned_body;
        b.velocity = Vector2Subtract(GetMousePosition(), mouse_pressed_pos);

        const float dt = 1.0 / SIMULATION_STEPS;
        for(size_t step = 0; step < PATH_POINTS; step++) {
            integrate_pos(&b, dt);
            vec_foreach(const CelestialBody* o, bodies) {
                Vector2 f = compute_gravitational_force(&b, o);
                b.force = Vector2Add(b.force, f);
            }
            integrate_vel(&b, dt);
            b.force = (Vector2){0};
            spawn_path[step] = b.position;
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

    if(show_spawn_path) {
        for(size_t i = 0; i < PATH_POINTS - 1; i++) {
            DrawLineEx(spawn_path[i], spawn_path[i + 1], 4, BLUE);
        }
    }

    EndDrawing();
}

int main(void) {
    InitWindow(0, 0, "raylib [core] example - basic window");
    ToggleFullscreen();
    SetTargetFPS(TARGET_FPS);

    const int width = GetScreenWidth(), height = GetScreenHeight();

    vec_push_back(bodies, create_body((Vector2){width / 2., height / 2.}, (Vector2){0, 0}, 100, 100,
                                      ORANGE));
    vec_push_back(bodies, create_body((Vector2){width / 2. + 500, height / 2.},
                                      (Vector2){0, 3 * TARGET_FPS}, 1, 30, BLUE));
    vec_push_back(bodies, create_body((Vector2){width / 2. - 500, height / 2.},
                                      (Vector2){0, -3 * TARGET_FPS}, 2, 30, RED));
    vec_push_back(bodies, create_body((Vector2){width / 2., height / 2. + 600},
                                      (Vector2){3 * TARGET_FPS, 0}, 10, 50, GREEN));

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
