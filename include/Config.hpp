#pragma once
#include "raylib.h"
#include "raymath.h"
#include <random>
#include <iostream>

namespace Config {
    inline int SCREEN_W = 1280;
    inline int SCREEN_H = 720;
    inline int FPS = 60;

    inline float AGENT_VISION_RADIUS = 200.0f;
    inline float AGENT_MAX_ENERGY = 200.0f;
    inline float AGENT_START_ENERGY = 100.0f;
    inline float METABOLISM_RATE = 15.0f;

    inline float FRUIT_ENERGY = 50.0f;
    inline float POISON_DAMAGE = 50.0f;
    
    constexpr int GRID_CELL_SIZE = 50;
    inline int GRID_W = SCREEN_W / GRID_CELL_SIZE + 1;
    inline int GRID_H = SCREEN_H / GRID_CELL_SIZE + 1;

    inline int ACTIVE_AGENTS = 20;

    enum class SimSize { Small, Medium, Large, Huge };
    inline SimSize CURRENT_SIZE = SimSize::Medium;

    inline void SetWindowSize(SimSize size) {
        CURRENT_SIZE = size;
        switch(size) {
            case SimSize::Small:  SCREEN_W = 800;  SCREEN_H = 600;  break;
            case SimSize::Medium: SCREEN_W = 1280; SCREEN_H = 720;  break;
            case SimSize::Large:  SCREEN_W = 1920; SCREEN_H = 1080; break;
            case SimSize::Huge:   SCREEN_W = 2560; SCREEN_H = 1440; break;
        }
        GRID_W = SCREEN_W / GRID_CELL_SIZE + 1;
        GRID_H = SCREEN_H / GRID_CELL_SIZE + 1;
        
        ::SetWindowSize(SCREEN_W, SCREEN_H); // Raylib function - Global scope
    }

    inline float SPEED_ENERGY_MULTIPLIER = 1.5f;
    inline float SIZE_SPEED_MULTIPLIER = 0.8f;


    inline float LEARNING_RATE = 0.02f;
    inline bool ENABLE_LIFETIME_LEARNING = true;

    inline bool OBSTACLES_ENABLED = true;
    inline int OBSTACLE_COUNT = 5;
    
    inline float COLLISION_ENERGY_PENALTY = 5.0f;
    inline float COLLISION_LEARNING_BOOST = 1.5f;

    // Balancing Variables
    inline float PREDATOR_STEAL_AMOUNT = 40.0f;
    inline float HERBIVORE_FRUIT_BONUS = 1.5f;
    inline float SCAVENGER_POISON_GAIN = 0.8f;
    inline float PREDATOR_METABOLISM_MODIFIER = 1.0f;
    inline float SEASON_DURATION = 30.0f;
    
    inline float MUTATION_RATE_MULTIPLIER = 1.0f;
    inline float MATING_ENERGY_COST = 60.0f;
    
    inline int FRUIT_SPAWN_AMOUNT = 10;
    inline int POISON_SPAWN_AMOUNT = 10;

    // Advanced Balancing
    inline float MATING_ENERGY_THRESHOLD = 120.0f;
    inline float EAT_RADIUS = 15.0f;
    inline float MATING_RANGE = 50.0f; // Sqr is 2500
    
    inline float CHILD_BRAIN_MUTATION_RATE = 0.1f;
    inline float CHILD_BRAIN_MUTATION_POWER = 0.15f;
    inline float CHILD_PHENOTYPE_MUTATION_RATE = 0.1f;


}

inline std::mt19937& GetRNG() {
    static std::mt19937 rng(std::random_device{}());
    return rng;
}

inline float RandomFloat(float min, float max) {
    std::uniform_real_distribution<float> dist(min, max);
    return dist(GetRNG());
}

inline float NormalizeAngle(float angle) {
    angle = std::fmod(angle + PI, 2.0f * PI);
    if (angle < 0) angle += 2.0f * PI;
    return angle - PI;
}
