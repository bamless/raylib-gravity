#include <math.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>

#include "ext_vector.h"
#include "raylib.h"
#include "raymath.h"

#define G                (30)
#define SIMULATION_STEPS (120)
#define PATH_POINTS      (10000)

static const float sub_dt = 1. / SIMULATION_STEPS;

typedef struct CelestialBody {
    Vector2 position, prev_position;
    Vector2 force, prev_force;
    Vector2 velocity;
    float radius;
    float inv_mass;
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
        .prev_position = position,
        .velocity = velocity,
        .radius = radius,
        .color = color,
        .inv_mass = 1 / (density * radius * radius),
    };
}

static void integrate_pos(CelestialBody* b, float dt) {
    // Position Verlet
    //   a = F / m
    //   x(t + dt) = x(t) + v(t) * dt + 0.5 * a(t) * dt^2
    b->position = Vector2Add(Vector2Add(b->position, Vector2Scale(b->velocity, dt)),
                             Vector2Scale(b->prev_force, dt * dt * b->inv_mass * 0.5));
}

static void integrate_vel(CelestialBody* b, float dt) {
    // Velocity Verlet
    //   a = F / m
    //   v(t + dt) = v(t) + 0.5 * (a(t) + a(t + dt)) * dt
    b->velocity = Vector2Add(b->velocity, Vector2Scale(Vector2Add(b->prev_force, b->force),
                                                       dt * 0.5 * b->inv_mass));
}

static Vector2 compute_gravitational_force(const CelestialBody* b1, const CelestialBody* b2) {
    // F = G * (m1 * m2 / r^2)
    Vector2 r = Vector2Subtract(b2->position, b1->position);
    float r2 = fmaxf(Vector2LengthSqr(r), 1e-6f);
    r = Vector2Scale(r, 1 / sqrt(r2));
    float m1 = 1.0f / b1->inv_mass;
    float m2 = 1.0f / b2->inv_mass;
    return Vector2Scale(r, G * (m1 * m2) / r2);
}

static void apply_forces(CelestialBody* b, float dt) {
    vec_foreach(const CelestialBody* o, bodies) {
        if(b != o) {
            Vector2 f = compute_gravitational_force(b, o);
            b->force = Vector2Add(b->force, f);
        }
    }
}

static void update(float dt) {
    // Reset forces
    vec_foreach(CelestialBody* b, bodies) {
        b->prev_position = b->position;
        b->prev_force = b->force;
        b->force = (Vector2){0};
    }

    vec_foreach(CelestialBody* b, bodies) {
        integrate_pos(b, dt);
    }

    vec_foreach(CelestialBody* b, bodies) {
        apply_forces(b, dt);
    }

    vec_foreach(CelestialBody* b, bodies) {
        integrate_vel(b, dt);
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

        for(size_t step = 0; step < PATH_POINTS; step++) {
            b.prev_force = b.force;
            b.force = (Vector2){0};
            integrate_pos(&b, sub_dt);
            apply_forces(&b, sub_dt);
            integrate_vel(&b, sub_dt);
            spawn_path[step] = b.position;
        }
    }
}

static void print_energy() {
    float ke = 0.0f;
    vec_foreach(CelestialBody* b, bodies) {
        float v2 = Vector2LengthSqr(b->velocity);
        ke += (0.5f / b->inv_mass) * v2;
    }

    float pe = 0.0f;
    for (size_t i = 0; i < vec_size(bodies); i++) {
        for (size_t j = i + 1; j < vec_size(bodies); j++) {
            Vector2 r = Vector2Subtract(bodies[j].position, bodies[i].position);
            float r_len = Vector2Length(r);
            float m1 = 1.0f / bodies[i].inv_mass;
            float m2 = 1.0f / bodies[j].inv_mass;
            pe -= G * (m1 * m2) / r_len;
        }
    }

    DrawText(TextFormat("Total Energy: %f", ke + pe), 0, 30, 30, BLACK);
    DrawText(TextFormat("Kinetic Energy: %f", ke), 0, 60, 30, BLACK);
    DrawText(TextFormat("Potential Energy: %f", pe), 0, 90, 30, BLACK);
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

    print_energy();

    EndDrawing();
}

int main(void) {
    InitWindow(0, 0, "raylib [core] example - basic window");
    ToggleFullscreen();

    const int width = GetScreenWidth(), height = GetScreenHeight();

    vec_push_back(bodies, create_body((Vector2){width / 2., height / 2.}, (Vector2){0}, 100, 100,
                                      ORANGE));
    vec_push_back(bodies, create_body((Vector2){width / 2. + 500, height / 2.},
                                      (Vector2){0, 3 * 60}, 1, 30, BLUE));
    vec_push_back(bodies, create_body((Vector2){width / 2. - 500, height / 2.},
                                      (Vector2){0, -3 * 60}, 2, 30, RED));
    vec_push_back(bodies, create_body((Vector2){width / 2., height / 2. + 900},
                                      (Vector2){3 * 60, 0}, 10, 50, GREEN));

    float acc = 0;
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
