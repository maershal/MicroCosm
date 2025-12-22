#include "raylib.h"
#include <vector>
#include <ranges>
#include <iostream>
#include <concepts>
#include <variant>
#include <random>
#include <cmath>
#include "Core/NeuralNetwork.hpp"
struct Agent {
    Vector2 position;
    Vector2 velocity;
    float energy;
    float angle;    //radians

    NeuralNetwork brain{5, 8, 2};
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
            .position = {RandomFloat(100, 1180), RandomFloat(100,620)},
            .velocity = {0, 0},
            .energy = 100.0f,
            .angle = RandomFloat(0, 6.28f)  //2*PI
        });
    }

    for(int i=0; i<50; i++) entities.emplace_back(Fruit{{RandomFloat(50, 1230), RandomFloat(50, 670)}});
    while(!WindowShouldClose())
    {
        float dt = GetFrameTime();

        for (auto& entity: entities) {
            std::visit(overloaded{
                [&](Agent& a) {
                    //1. Gathering Data (sensors)
                    // So far mock data, TODO raycasting
                    std::vector<float> sensors = {
                        0.5f,   // some food right
                        0.2f,   // near
                        -0.1f,  //poison near
                        1.0f, // far away
                        a.energy / 100.0f // Energy
                    };
                    // 2. Thinking
                    std::vector<float> outputs = a.brain.feedForward(sensors);

                    float leftTrack = outputs[0];
                    float rightTrack = outputs[1];

                    //3. Physics
                    float rotSpeed = 3.0f; 
                    float speed = 100.0f;

                    a.angle += (leftTrack - rightTrack) * rotSpeed * dt;

                    Vector2 forward = {cos(a.angle), sin(a.angle)};
                    float throttle = (leftTrack + rightTrack) / 2.0f;

                    a.position.x += forward.x * throttle * speed * dt;
                    a.position.y += forward.y * throttle * speed * dt;

                    //Wall boucning
                    if(a.position.x < 0) a.position.x = 1280;
                    if(a.position.x > 1280) a.position.x = 0;
                    if(a.position.y < 0) a.position.y = 720;
                    if(a.position.y > 720) a.position.y = 0;
                    //metabolism
                    a.energy -= 5.0f * dt;
            }, [](auto&){} //other objects do not think
        } ,entity);
        }
        BeginDrawing();
        ClearBackground({20, 20, 20, 255});

        // RENDER

        for(const auto& entity : entities) {
            std::visit(overloaded{
                [](const Agent& a) {
                Color col = (a.energy > 50 ) ? BLUE : RED;
                DrawCircleV(a.position, 6.0f, col);

                DrawLine(a.position.x, a.position.y, 
                    a.position.x + cos(a.angle)*15, a.position.y + sin(a.angle)*15, RAYWHITE);
            }, 
            [](const Fruit& f) {
                DrawCircleV(f.position, 4.0f, GREEN);
            },
            [](const Poison& p) {
                DrawRectangleV(p.position, {8, 8}, PURPLE);
            }
        }, entity);
        }
        DrawText("Agents are controlled by Neural Network!", 10, 10, 20, LIGHTGRAY);
        EndDrawing();
    }
    CloseWindow();
    return 0;

}