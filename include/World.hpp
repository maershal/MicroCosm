#pragma once
#include <vector>
#include "Entities.hpp"

struct SpatialGrid {
    std::vector<int> fruitIndices[Config::GRID_W][Config::GRID_H];
    std::vector<int> poisonIndices[Config::GRID_W][Config::GRID_H];
    std::vector<int> agentIndices[Config::GRID_W][Config::GRID_H];
    std::vector<int> obstacleIndices[Config::GRID_W][Config::GRID_H];

    void Clear();
    void AddFruit(int index, Vector2 pos);
    void AddPoison(int index, Vector2 pos);
    void AddAgent(int index, Vector2 pos);
    void AddObstacle(int index, Vector2 pos, Vector2 size);
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
    
    float avgSpeed = 0.0f;
    float avgSize = 0.0f;
    float avgEfficiency = 0.0f;
};

struct SensorData {
    float fruitDist = 1.0f;
    float fruitAngle = 0.0f;
    float poisonDist = 1.0f;
    float poisonAngle = 0.0f;
    float obstacleDist = 1.0f;
    float obstacleAngle = 0.0f;
};

struct GeneticRecord {
    NeuralNetwork brain;
    Phenotype phenotype;
    float fitness;

    GeneticRecord(const NeuralNetwork& b, const Phenotype& p, float f)
    : brain(b), phenotype(p), fitness(f) {}
};

class World {
public:
    std::vector<Agent> agents;
    std::vector<Fruit> fruits;
    std::vector<Poison> poisons;
    std::vector<Obstacle> obstacles;
    SpatialGrid grid;
    Stats stats;

    World();
    void Update(float dt);
    void Draw();
    
    void GenerateRandomObstacles();
    void GenerateMaze();
    void GenerateArena();
    void GenerateRooms();
    void GenerateSpiral();
    void ClearObstacles();
    
    Vector2 FindSafeSpawnPosition(float minRadius = 10.0f, int maxAttempts = 50);

private:
    void InitPopulation();
    SensorData ScanSurroundings(Agent& agent);
    void HandleInteractions(Agent& agent, std::vector<Agent>& babies);
    bool CheckObstacleCollision(Vector2 pos, float radius);
    
    template <typename T>
    void CleanupEntities(std::vector<T>& entities);

    std::vector<GeneticRecord> savedGenetics;
};