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
#include <unordered_set>

enum class Sex{ Male, Female };
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
struct Agent {
    Vector2 position;
    Vector2 velocity;
    float energy;
    float angle;    //radians

    Sex sex;
    NeuralNetwork brain{5, 8, 2};
};

struct Fruit {
    Vector2 position;
    float energyValue = 50.0f;
};

struct Poison {
    Vector2 position;
    float damageValue = 50.0f;
};

Agent CreateRandomAgent(Vector2 pos, float energy = 100.0f) {
    return Agent{
        .position = pos,
        .velocity = {0, 0},
        .energy = energy,
        .angle = RandomFloat(0, 6.28f),
        .sex = (RandomFloat(0, 1) > 0.5f) ? Sex::Male : Sex::Female
    };
}


struct SensorData {
    float closestFruitDist = 1.0f; // far 
    float closestFruitAngle = 0.0f;
    float closestPoisonDist = 1.0f;
    float closestPoisonAngle = 0.0f;

    // for debugging
    Vector2 closestFruitPos = {-1, -1};
    Vector2 closestPoisonPos = {-1, -1};

    //closesMateDist/Angle TODO
};
using SimEntity = std::variant<Agent, Fruit, Poison>;
template <class... Ts> struct overloaded : Ts ...{ using Ts::operator()...; };
template <class... Ts> overloaded(Ts...) -> overloaded<Ts...>;


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


struct ReproductionResult {
    Agent* maleParent = nullptr;
    Agent* femaleParent = nullptr;
    bool canReproduce = false;
};

ReproductionResult FindMate(Agent& agent, std::vector<SimEntity>& entities, float mateRadius) {
    ReproductionResult result;
    
    const float MATE_ENERGY_THRESHOLD = 100.0f;
    
    if (agent.energy < MATE_ENERGY_THRESHOLD) return result;
    
    float closestDist = mateRadius;
    Agent* closestMate = nullptr;
    
    for (auto& entity : entities) {
        if (Agent* other = std::get_if<Agent>(&entity)) {
            if (other == &agent) continue;
            if (other->sex == agent.sex) continue; // Musi być przeciwna płeć
            if (other->energy < MATE_ENERGY_THRESHOLD) continue;
            
            float dist = Vector2Distance(agent.position, other->position);
            if (dist < closestDist) {
                closestDist = dist;
                closestMate = other;
            }
        }
    }
    
    if (closestMate && closestDist < 50.0f) {
        result.canReproduce = true;
        if (agent.sex == Sex::Male) {
            result.maleParent = &agent;
            result.femaleParent = closestMate;
        } else {
            result.maleParent = closestMate;
            result.femaleParent = &agent;
        }
    }
    
    return result;
}

std::optional<Agent> TryReproduceSexual(Agent& mother, Agent& father) {
    const float REPRODUCTION_COST_MOTHER = 100.0f;
    const float REPRODUCTION_COST_FATHER = 40.0f;
    const float REPRODUCTION_THRESHOLD = 120.0f;
    
    if (mother.energy < REPRODUCTION_THRESHOLD || father.energy < REPRODUCTION_THRESHOLD) {
        return std::nullopt;
    }
    
    mother.energy -= REPRODUCTION_COST_MOTHER;
    father.energy -= REPRODUCTION_COST_FATHER;
    
    Agent child;
    child.brain = NeuralNetwork::Crossover(mother.brain, father.brain);
    
    child.brain.mutate(0.1f, 0.15f);
    
    child.position = mother.position;
    child.position.x += RandomFloat(-10, 10);
    child.position.y += RandomFloat(-10, 10);
    
    child.velocity = {0, 0};
    child.energy = 80.0f;
    child.angle = RandomFloat(0, 6.28f);
    
    child.sex = (RandomFloat(0, 1) > 0.5f) ? Sex::Male : Sex::Female;
    
    return child;
}

int main()
{
    InitWindow(1280, 720, "mikrokosmos - TEST");
    SetTargetFPS(60);

    std::vector<SimEntity> entities;

    for(int i = 0; i < 100; i++)
    {
        entities.emplace_back(CreateRandomAgent(
            {RandomFloat(100, 1180), RandomFloat(100, 620)}
        ));
    }

    for(int i=0; i<50; i++) entities.emplace_back(Fruit{{RandomFloat(50, 1230), RandomFloat(50, 670)}});
    for(int i=0; i<15; i++) entities.emplace_back(Poison{{RandomFloat(0, 1280), RandomFloat(0, 720)}});
    struct Statistics {
        int totalBirths = 0;
        int totalDeaths = 0;
        int generation = 0;
        int highestPopulation = 0;
        float averageEnergy = 0.0f;
        int maleCount = 0;
        int femaleCount = 0;
        float totalLifetime = 0.0f;
    } stats;

float generationTimer = 0.0f;
    const float VISION_RADIUS = 200.0f;
    while(!WindowShouldClose())
    {
        float dt = GetFrameTime();
        // Move and Thinking
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

                agent -> energy -= 10.0f * dt;  // Metabolism
            }
        }
        // Eating and birth
        std::vector<size_t> entitiesToRemove; 
        for (size_t i = 0; i < entities.size(); ++i) {
            if(auto* agent = std::get_if<Agent>(&entities[i])) {
                if(agent->energy <= 0) {
                    entitiesToRemove.push_back(i); // famine death
                    stats.totalDeaths++;
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
            stats.generation++;
            for (int i = 0; i < 50; i++) {
                entities.emplace_back(CreateRandomAgent(
                    {RandomFloat(100, 1100), RandomFloat(100, 600)}
                ));
            }
        }

        int agentCount = 0;
        stats.maleCount = 0;
        stats.femaleCount = 0;
        float totalEnergy = 0.0f;

        for (const auto& e : entities) {
            if (const Agent* a = std::get_if<Agent>(&e)) {
                agentCount++;
                totalEnergy += a->energy;
                if (a->sex == Sex::Male) stats.maleCount++;
                else stats.femaleCount++;
            }
        }

        stats.averageEnergy = agentCount > 0 ? totalEnergy / agentCount : 0.0f;
        if (agentCount > stats.highestPopulation) stats.highestPopulation = agentCount;

        generationTimer += dt;


        std::vector<SimEntity> babies;
        std::unordered_set<Agent*> agentsWhoReproduced;
        for (auto& entity : entities) {
            if (Agent* agent = std::get_if<Agent>(&entity)) {
                if (agent->sex != Sex::Female) continue;
                if (agentsWhoReproduced.count(agent) > 0) continue;
                auto mateResult = FindMate(*agent, entities, 150.0f);
                
                if (mateResult.canReproduce) {
                    if (agentsWhoReproduced.count(mateResult.maleParent) > 0) continue;
                    
                    if (auto child = TryReproduceSexual(*mateResult.femaleParent, *mateResult.maleParent)) {
                        babies.push_back(*child);
                        stats.totalBirths++;
                        agentsWhoReproduced.insert(mateResult.femaleParent);
                        agentsWhoReproduced.insert(mateResult.maleParent);
                    }
                }
            }
        }

        entities.insert(entities.end(), std::make_move_iterator(babies.begin()), std::make_move_iterator(babies.end()));
        int fruitCount = 0;
        for (const auto& e : entities) if(std::holds_alternative<Fruit>(e)) fruitCount++;

        if (fruitCount < 40) {
            entities.emplace_back(Fruit{{RandomFloat(0, 1280), RandomFloat(0, 720)}});
        }
        int poisonCount = 0;
        for (const auto& e : entities) if(std::holds_alternative<Poison>(e)) poisonCount++;

        if (poisonCount < 15) {
            entities.emplace_back(Poison{{RandomFloat(0, 1280), RandomFloat(0, 720)}});
        }
        
        
        //Render
        BeginDrawing();
        ClearBackground({20, 20, 20, 255});

        for (const auto& entity : entities) {
            std::visit(overloaded {
                [&](const Agent& a ) {
                    Color col;
                    if (a.sex == Sex::Male) {
                        col = (a.energy > 50) ? BLUE : DARKBLUE;
                    } else {
                        col = (a.energy > 50) ? PINK : MAROON;
                    }
                    DrawCircleV(a.position, 6.0f, col);

                    Vector2 head = {a.position.x + cos(a.angle)*10, a.position.y + sin(a.angle)*10 };
                    DrawLineV(a.position, head, WHITE);
                    if (a.energy > 120.0f) {
                        DrawCircle(a.position.x, a.position.y, 12.0f, {255, 255, 255, 30});
                    }
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
            DrawText(TextFormat("Population: %i (M:%i F:%i)", agentCount, stats.maleCount, stats.femaleCount), 10, 10, 20, WHITE);
            DrawText(TextFormat("Births: %i | Deaths: %i", stats.totalBirths, stats.totalDeaths), 10, 35, 20, GREEN);
            DrawText(TextFormat("Generation: %i | Peak: %i", stats.generation, stats.highestPopulation), 10, 60, 20, YELLOW);
            DrawText(TextFormat("Avg Energy: %.1f | Time: %.1fs", stats.averageEnergy, generationTimer), 10, 85, 20, SKYBLUE);
            DrawText(TextFormat("Fruits: %i | Poisons: %i", fruitCount, poisonCount), 10, 110, 20, ORANGE);
            EndDrawing();
        }
        CloseWindow();
        return 0;
    }
