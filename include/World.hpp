#pragma once
#include <vector>
#include <vector>
#include "Entities.hpp"

enum class Season { Spring, Summer, Autumn, Winter };

struct SeasonState {
    Season currentSeason = Season::Spring;
    float seasonTimer = 0.0f;
    float seasonDuration = 30.0f; // Seconds per season
    const char* GetName() const {
        switch(currentSeason) {
            case Season::Spring: return "Spring";
            case Season::Summer: return "Summer";
            case Season::Autumn: return "Autumn";
            case Season::Winter: return "Winter";
            default: return "Unknown";
        }
    }
};

struct SpatialGrid {
    // Flattened grid: cells[x * GRID_H + y]
    // Vector of Vectors for ID lists
    std::vector<std::vector<int>> fruitIndices;
    std::vector<std::vector<int>> poisonIndices;
    std::vector<std::vector<int>> agentIndices;
    std::vector<std::vector<int>> obstacleIndices;

    void Resize(int w, int h) {
        int size = w * h;
        fruitIndices.assign(size, {});
        poisonIndices.assign(size, {});
        agentIndices.assign(size, {});
        obstacleIndices.assign(size, {});
    }

    void Clear();
    void AddFruit(int index, Vector2 pos);
    void AddPoison(int index, Vector2 pos);
    void AddAgent(int index, Vector2 pos);
    void AddObstacle(int index, Vector2 pos, Vector2 size);
    
    // Helper to get cell index safely
    int GetCellIndex(int x, int y) const {
        if (x < 0) x = 0; if (x >= Config::GRID_W) x = Config::GRID_W - 1;
        if (y < 0) y = 0; if (y >= Config::GRID_H) y = Config::GRID_H - 1;
        return x * Config::GRID_H + y;
    }
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
    
    struct HistoryPoint {
        float avgFitness;
        float bestFitness;
        float avgSpeed;
        float avgSize;
        int population;
        int herbivoreCount;
        int scavengerCount;
        int predatorCount;
        
        int countRNN;
        int countNEAT;
        int countNN;
    };
    std::vector<HistoryPoint> history;
};

struct SensorData {
    float fruitDist = 1.0f;
    float fruitAngle = 0.0f;
    float poisonDist = 1.0f;
    float poisonAngle = 0.0f;
    float obstacleDist = 1.0f;
    float obstacleAngle = 0.0f;
    float pheromoneIntensity = 0.0f;
};

struct GeneticRecord {
    std::unique_ptr<IBrain> brain;
    Phenotype phenotype;
    float fitness;

    GeneticRecord(const IBrain& b, const Phenotype& p, float f)
    : brain(b.Clone()), phenotype(p), fitness(f) {}
    
    GeneticRecord(GeneticRecord&&) = default;
    GeneticRecord& operator=(GeneticRecord&&) = default;
};

class World {
public:
    std::vector<Agent> agents;
    std::vector<Fruit> fruits;
    std::vector<Poison> poisons;
    std::vector<Obstacle> obstacles;
    SpatialGrid grid;
    Stats stats;
    SeasonState season;

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
    
    // God Mode Powers
    void ThanosSnap();
    void FertilityBlessing();
    void ForceMutation();
    void SpawnSpecies(Species type, int count);

    void UpdateSeasons(float dt);

private:
    void InitPopulation();
    void UpdateAgent(Agent& agent, float dt, std::vector<Agent>& babies);
    SensorData ScanSurroundings(Agent& agent);
    void HandleInteractions(Agent& agent, std::vector<Agent>& babies);
    bool CheckObstacleCollision(Vector2 pos, float radius);
    
    template <typename T>
    void CleanupEntities(std::vector<T>& entities);

    std::vector<GeneticRecord> savedGenetics;
};