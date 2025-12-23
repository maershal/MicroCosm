#pragma once
#include "Config.hpp"
#include "NeuralNetwork.hpp"

enum class Sex { Male, Female };

struct Fruit { 
    Vector2 pos; 
    bool active = true; 
};

struct Poison { 
    Vector2 pos; 
    bool active = true; 
};

struct Obstacle {
    Vector2 pos;
    Vector2 size;
    bool active = true;

    bool Contains(Vector2 point) const {
        return point.x >= pos.x && point.x <= pos.x + size.x &&
               point.y >= pos.y && point.y <= pos.y + size.y;
    }

    bool Intersects(Vector2 point, float radius) const {
        float closestX = std::clamp(point.x, pos.x, pos.x + size.x);
        float closestY = std::clamp(point.y, pos.y, pos.y + size.y);
        float distX = point.x - closestX;
        float distY = point.y - closestY;
        return (distX * distX + distY * distY) < radius * radius;
    }
};

struct Phenotype {
    float speed = 1.0f;     // move speed 0.5 - 2.0
    float size = 1.0f;      // body size multiplier 0.7 - 1.5
    float efficiency = 1.0f; // metabolic efficiency 0.7 - 1.3

    Phenotype() {
        speed = RandomFloat(0.8f, 1.2f);
        size = RandomFloat(0.85f, 1.15f);
        efficiency = RandomFloat(0.9f, 1.1f);
    }
    
    Phenotype(float s, float sz, float e) : speed(s), size(sz), efficiency(e) {}

    float GetActualSpeed() const {
        return speed * (2.0f - size * Config::SIZE_SPEED_MULTIPLIER);
    }

    float GetMetabolicRate() const {
        return (speed * Config::SPEED_ENERGY_MULTIPLIER) / efficiency;
    }

    float GetVisualSize() const {
        return 5.0f * size;
    }

    static Phenotype Crossover(const Phenotype& a, const Phenotype& b) {
        return Phenotype(
            RandomFloat(0, 1) > 0.5f ? a.speed : b.speed,
            RandomFloat(0, 1) > 0.5f ? a.size : b.size,
            RandomFloat(0, 1) > 0.5f ? a.efficiency : b.efficiency
        );
    }

    void Mutate(float rate) {
        if(RandomFloat(0, 1) < rate) {
            speed = std::clamp(speed + RandomFloat(-0.1f, 0.1f), 0.5f, 2.0f);
        }
        if (RandomFloat(0, 1) < rate) {
            size = std::clamp(size + RandomFloat(-0.1f, 0.1f), 0.7f, 1.5f);
        }
        if (RandomFloat(0, 1) < rate) {
            efficiency = std::clamp(efficiency + RandomFloat(-0.1f, 0.1f), 0.7f, 1.3f);
        }
    }
};

struct Agent {
    Vector2 pos;
    float angle;
    float energy;
    Sex sex;
    NeuralNetwork brain;
    Phenotype phenotype;
    bool active = true;

    // Fitness tracking
    float lifespan = 0.0f;
    int childrenCount = 0;
    int fruitsEaten = 0;
    int poisonsAvoided = 0;
    int obstaclesHit = 0;
    
    float totalReward = 0.0f;
    std::vector<float> lastInputs;
    std::vector<float> lastOutputs;
    // Debug info
    Vector2 targetFruit = {-1, -1};
    Vector2 targetPoison = {-1, -1};
    
    Agent() : pos({0,0}), angle(0), energy(0), sex(Sex::Male), brain(6, 8, 2) {}
    
    Agent(Vector2 p) : pos(p), angle(RandomFloat(0, 2*PI)), energy(Config::AGENT_START_ENERGY), 
                       sex(RandomFloat(0,1) > 0.5f ? Sex::Male : Sex::Female),
                       brain(6, 8, 2) {}
    
    Agent(Vector2 p, const NeuralNetwork& net, const Phenotype& pheno) : pos(p), angle(RandomFloat(0, 2*PI)), 
                                                  energy(Config::AGENT_START_ENERGY),
                                                  sex(RandomFloat(0,1) > 0.5f ? Sex::Male : Sex::Female),
                                                  brain(net), phenotype(pheno) {}
    
    float CalculateFitness() const {
        return lifespan * 0.3f +
               childrenCount * 15.0f +
               fruitsEaten * 2.0f +
               poisonsAvoided * 0.5f+ 
               totalReward * 0.1f -
               obstaclesHit * 0.2f;
    }
};