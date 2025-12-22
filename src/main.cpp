#include "raylib.h"
#include "raymath.h"
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

float NormalizeAngle(float angle)
{
    while (angle > PI) angle -= 2 * PI;
    while (angle < -PI) angle += 2 * PI;
    return angle;
}

struct SensorData {
    float closestFruitDist = 1.0f; // far 
    float closestFruitAngle = 0.0f;
    float closestPoisonDist = 1.0f;
    float closestPoisonAngle = 0.0f;

    // for debugging
    Vector2 closestFruitPos = {-1, -1};
    Vector2 closestPoisonPos = {-1, -1};
};

SensorData GetSensors(const Agent& agent, const std::vector <SimEntity>& entities, float visionRadius) {
    SensorData data;
    float minFruitDist = visionRadius;
    float minPoisonDist = visionRadius;

    for (const auto& entity: entities) {
        if (const Fruit* f = std::get_if<Fruit>(&entity)) {
            float dist = Vector2Distance(agent.position, f->position);
            if(dist < minFruitDist)
            {
                minFruitDist = dist;
                data.closestFruitPos = f -> position;

                float angleToTarget = atan2(f->position.y - agent.position.y, f->position.x - agent.position.x);
                float relativeAngle = NormalizeAngle(angleToTarget - agent.angle);

                data.closestFruitDist = minFruitDist / visionRadius;
                data.closestFruitAngle = relativeAngle / PI;
            }
        }
        else if (const Poison* p = std::get_if<Poison>(&entity)) {
            float dist = Vector2Distance(agent.position, p->position);
            if (dist < minPoisonDist) {
                minPoisonDist = dist;
                data.closestPoisonPos = p -> position;

                float angleToTarget = atan2(p->position.y - agent.position.y, p->position.x - agent.position.x);
                float relativeAngle = NormalizeAngle(angleToTarget - agent.angle);

                data.closestPoisonDist = minPoisonDist / visionRadius;
                data.closestPoisonAngle = relativeAngle / PI;

            }
        }
    }
    return data;
}


std::optional<Agent> TryReproduce (Agent& parent)
{
    const float REPRODUCTION_COST = 80.0f;
    const float REPRODUCTION_THRESHOLD = 150.0f;

    if (parent.energy > REPRODUCTION_THRESHOLD) {
        parent.energy -= REPRODUCTION_COST;

        Agent child = parent;

        child.energy = 60.0f;

        child.position.x += RandomFloat(-5, 5);
        child.position.y += RandomFloat(-5, 5);

        child.brain.mutate(0.15f, 0.2f);

        return child;
    }
    return std::nullopt;
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

    const float VISION_RADIUS = 200.0f;
    while(!WindowShouldClose())
    {
        float dt = GetFrameTime();

        for(auto& entity : entities) {
            if (Agent* agent = std::get_if<Agent>(&entity)) {
                SensorData senses = GetSensors(*agent, entities, VISION_RADIUS);

                std::vector<float> inputs = {
                    senses.closestFruitAngle,
                    senses.closestFruitDist,
                    senses.closestPoisonAngle,
                    senses.closestPoisonDist,
                    agent->energy / 100.0f
                };

                std::vector<float> outputs = agent -> brain.feedForward(inputs);

                float leftTrack = outputs[0];
                float rightTrack = outputs[1];
                float rotSpeed = 3.0f;
                float speed = 120.0f;

                agent -> angle += (leftTrack - rightTrack) * rotSpeed * dt;
                Vector2 forward = {cos(agent -> angle) , sin(agent -> angle)};
                float throttle = (leftTrack + rightTrack) / 2.0f;

                if (throttle < -0.2f) throttle = -0.2f;

                agent -> position.x += forward.x * throttle * speed * dt;
                agent -> position.y += forward.y * throttle * speed * dt;

                if(agent -> position.x < 0) agent -> position.x = 1280;
                if(agent -> position.x > 1280) agent -> position.x = 0;
                if(agent-> position.y < 0) agent -> position.y = 720;
                if(agent -> position.y > 720) agent -> position.y = 0;

                agent -> energy -= 10.0f * dt;
            }
        }

        std::vector<size_t> entitiesToRemove; 
        for (size_t i = 0; i < entities.size(); ++i) {
            if(auto* agent = std::get_if<Agent>(&entities[i])) {
                if(agent->energy <= 0) {
                    entitiesToRemove.push_back(i); // famine death
                    continue;
                }
                for (size_t j = 0; j < entities.size(); ++j) {
                    if (i == j) continue;

                    if(auto* fruit = std::get_if<Fruit>(&entities[j])) {
                        if(Vector2Distance(agent -> position, fruit->position) < 15.0f) {
                            agent -> energy += fruit -> energyValue;
                            if(agent-> energy > 200.0f) agent -> energy = 200.0f;   // energy limit
                            entitiesToRemove.push_back(j);

                            entities.emplace_back(Fruit{{RandomFloat(0, 1280), RandomFloat(0,720)}});

                        }
                    }
                    else if (auto* poison = std::get_if<Poison>(&entities[j])) {
                        if(Vector2Distance(agent -> position, poison -> position) < 15.0f) {
                            agent-> energy -= poison->damageValue;
                            entitiesToRemove.push_back(j);
                        }
                    }
                }
            }
        }

        std::sort(entitiesToRemove.begin(), entitiesToRemove.end(), std::greater<size_t>());
        entitiesToRemove.erase(std::unique(entitiesToRemove.begin(), entitiesToRemove.end()), entitiesToRemove.end());
        for (auto idx : entitiesToRemove) {
            if (idx < entities.size()) entities.erase(entities.begin() + idx);
        }

        bool anyAgentAlive = false;
        for(const auto& e : entities) if(std::holds_alternative<Agent>(e)) anyAgentAlive = true;

        if(!anyAgentAlive)
        {
            for (int i=0; i<200; i++) entities.emplace_back(Agent{{RandomFloat(100, 1100), RandomFloat(100, 600)}, {0,0}, 100.0f, RandomFloat(0, 6.28f)});
    
        }

        std::vector<SimEntity> babies;
        for (auto& entity : entities) {
            if (Agent* agent = std::get_if<Agent>(&entity)) {
                if (auto child = TryReproduce(*agent)) {
                    babies.push_back(*child);
                }
            }
        }

        entities.insert(entities.end(), std::make_move_iterator(babies.begin()), std::make_move_iterator(babies.end()));
        int fruitCount = 0;
        for (const auto& e : entities) if(std::holds_alternative<Fruit>(e)) fruitCount++;

        if (fruitCount < 40) {
            entities.emplace_back(Fruit{{RandomFloat(0, 1280), RandomFloat(0, 720)}});
        }
        
        
        //Render
        BeginDrawing();
        ClearBackground({20, 20, 20, 255});

        for (const auto& entity : entities) {
            std::visit(overloaded {
                [&](const Agent& a ) {
                    Color col = (a.energy > 50) ? SKYBLUE : ORANGE;
                    DrawCircleV(a.position, 6.0f, col);

                    Vector2 head = {a.position.x + cos(a.angle)*10, a.position.y + sin(a.angle)*10 };
                    DrawLineV(a.position, head, WHITE);

                    // DEBUGGING - SHOW WHAT AGENT SEES (lines to goal)
                    // We had to again calculate sensors only for drawing (costly but nice);

                    SensorData s = GetSensors(a, entities, VISION_RADIUS);
                    if (s.closestFruitDist < 1.0f)
                        DrawLineV(a.position, s.closestFruitPos, {0, 255, 0, 50});
                    if (s.closestPoisonDist < 1.0f) 
                        DrawLineV(a.position, s.closestPoisonPos, {255, 0, 0, 50});
                    },
                    [](const Fruit& f) { DrawCircleV(f.position, 4.0f, GREEN); },
                    [](const Poison& p) { DrawRectangleV(p.position, {8,8}, PURPLE);}
                }, entity);
            }
            DrawText(TextFormat("Entities %i", entities.size()), 10, 10, 20, WHITE);
            EndDrawing();
        }
        CloseWindow();
        return 0;
    }
