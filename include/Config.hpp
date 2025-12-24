#pragma once
#include "raylib.h"
#include "raymath.h"
#include <random>
#include <iostream>

namespace Config {
    constexpr int SCREEN_W = 1280;
    constexpr int SCREEN_H = 720;
    inline int FPS = 60;

    inline float AGENT_VISION_RADIUS = 200.0f;
    inline float AGENT_MAX_ENERGY = 200.0f;
    inline float AGENT_START_ENERGY = 100.0f;
    inline float METABOLISM_RATE = 15.0f;

    inline float FRUIT_ENERGY = 50.0f;
    inline float POISON_DAMAGE = 50.0f;
    
    constexpr int GRID_CELL_SIZE = 50;
    constexpr int GRID_W = SCREEN_W / GRID_CELL_SIZE + 1;
    constexpr int GRID_H = SCREEN_H / GRID_CELL_SIZE + 1;

    inline int ACTIVE_AGENTS = 20;

    inline float SPEED_ENERGY_MULTIPLIER = 1.5f;
    inline float SIZE_SPEED_MULTIPLIER = 0.8f;


    inline float LEARNING_RATE = 0.02f;
    inline bool ENABLE_LIFETIME_LEARNING = true;

    inline bool OBSTACLES_ENABLED = true;
    inline int OBSTACLE_COUNT = 5;
    
    inline float COLLISION_ENERGY_PENALTY = 5.0f;
    inline float COLLISION_LEARNING_BOOST = 1.5f;

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
    while (angle > PI) angle -= 2 * PI;
    while (angle < -PI) angle += 2 * PI;
    return angle;
}
