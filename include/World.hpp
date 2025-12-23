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
    float avgFitness = 0.0f;
    float bestFitness = 0.0f;
    float totalFitness = 0.0f;
};

struct SensorData {
    float fruitDist = 1.0f;
    float fruitAngle = 0.0f;
    float poisonDist = 1.0f;
    float poisonAngle = 0.0f;
};

struct GeneticRecord {
    NeuralNetwork brain;
    float fitness;

    GeneticRecord(const NeuralNetwork& b, float f)
    : brain(b), fitness(f) {}
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

    std::vector<GeneticRecord> savedGenetics; // best genes with fitness!
};