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

struct Agent {
    Vector2 pos;
    float angle;
    float energy;
    Sex sex;
    NeuralNetwork brain;
    bool active = true;

    // Debug info
    Vector2 targetFruit = {-1, -1};
    Vector2 targetPoison = {-1, -1};
    Agent() : pos({0,0}), angle(0), energy(0), sex(Sex::Male), brain(5, 8, 2) {}
    Agent(Vector2 p) : pos(p), angle(RandomFloat(0, 2*PI)), energy(Config::AGENT_START_ENERGY), 
                       sex(RandomFloat(0,1) > 0.5f ? Sex::Male : Sex::Female),
                       brain(5, 8, 2) {}
};

