#pragma once
#include <vector>
#include "Entities.hpp"

struct SpatialGrid {
    std::vector<int> fruitIndices[Config::GRID_W][Config::GRID_H];
    std::vector<int> poisonIndices[Config::GRID_W][Config::GRID_H];
    std::vector<int> agentIndices[Config::GRID_W][Config::GRID_H];

    void Clear();
    void AddFruit(int index, Vector2 pos);
    void AddPoison(int index, Vector2 pos);
    void AddAgent(int index, Vector2 pos);
};

struct Stats {
    int generation = 0;
    int births = 0;
    int deaths = 0;
    float time = 0;
    int maxPop = 0;
};

struct SensorData {
    float fruitDist = 1.0f;
    float fruitAngle = 0.0f;
    float poisonDist = 1.0f;
    float poisonAngle = 0.0f;
};

class World {
public:
    std::vector<Agent> agents;
    std::vector<Fruit> fruits;
    std::vector<Poison> poisons;
    SpatialGrid grid;
    Stats stats;

    World();
    void Update(float dt);
    void Draw();

private:
    void InitPopulation();
    SensorData ScanSurroundings(Agent& agent);
    void HandleInteractions(Agent& agent, std::vector<Agent>& babies);
    
    template <typename T>
    void CleanupEntities(std::vector<T>& entities);
};
