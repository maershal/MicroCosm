#include "raylib.h"
#include <vector>
#include <ranges>
#include <iostream>
#include <concepts>
#include <variant>
#include <random>
struct Agent {
    Vector2 position;
    Vector2 velocity;
    float energy;
    float angle;
};

struct Fruit {
    Vector2 position;
    float energyValue = 20.0f;
};

struct Poison {
    Vector2 position;
    float damageValue = 50.0f;
};

using SimEntity = std::variant<Agent, Fruit, Poison>;
template <class... Ts> struct overloaded : Ts ...{ using Ts::operator()...; };
template <class... Ts> overloaded(Ts...) -> overloaded<Ts...>;

float RandomFloat(float min, float max) {
    static std::mt19937 mt(std::random_device{}());
    std::uniform_real_distribution<float> dist(min, max);
    return dist(mt);
}


int main()
{
    InitWindow(1280, 720, "mikrokosmos - TEST");
    SetTargetFPS(60);

    std::vector<SimEntity> entities;

    for(int i = 0; i < 100; i++)
    {
        entities.emplace_back(Agent {
            .position = {RandomFloat(0, 1280), RandomFloat(0,720)},
            .velocity = {0, 0},
            .energy = 100.0f,
            .angle = RandomFloat(0, 360)
        });
    }

    for(int i=0; i<50; i++) entities.emplace_back(Fruit{{RandomFloat(0, 1280), RandomFloat(0, 720)}});
    for(int i=0; i<20; i++) entities.emplace_back(Fruit{{RandomFloat(0, 1280), RandomFloat(0, 720)}});

    while(!WindowShouldClose())
    {
        BeginDrawing();
        ClearBackground({20, 20, 20, 255});

        for(const auto& entity : entities)
        {
            std::visit(overloaded {
                [](const Agent& a) {
                    DrawCircleV(a.position, 5.0f, BLUE);

                    DrawLine(a.position.x, a.position.y, 
                    a.position.x + cos(a.angle) * 10,
                     a.position.y + sin(a.angle) * 10,
                    SKYBLUE);
                },
                [](const Fruit& f) {
                    DrawCircleV(f.position, 3.0f, GREEN);
                },
                [](const Poison& p) {
                    DrawRectangleV(p.position, {6, 6}, PURPLE);
                }
            }, entity);
        }
        DrawText(TextFormat("Entities %i", entities.size()), 10, 10, 20, RAYWHITE);
        EndDrawing();
    }
    CloseWindow();
    return 0;

}