#pragma once
#include "Config.hpp"
#include "NeuralNetwork.hpp"
#include <memory>
#include <utility>

enum class Sex { Male, Female };

struct Fruit { 
    Vector2 pos; 
    bool active = true; 
};

struct Poison { 
    Vector2 pos; 
    bool active = true; 
};

// Obstacle types for variety
enum class ObstacleType { 
    Wall,      // Solid rectangular wall
    Circle,    // Circular obstacle
    L_Shape,   // L-shaped corner
    Corridor   // Narrow passage
};

// Enhanced Obstacle structure
struct Obstacle {
    Vector2 pos;
    Vector2 size;
    ObstacleType type;
    float rotation = 0.0f;
    bool active = true;
    Color color = {80, 80, 80, 255};
    float radius = 0.0f;
    
    Obstacle(Vector2 p, Vector2 s, ObstacleType t = ObstacleType::Wall);
    bool Contains(Vector2 point) const;
    bool Intersects(Vector2 point, float checkRadius) const;
    void Draw() const;
};

struct Phenotype {
    float speed = 1.0f;      // Movement speed multiplier (0.5 - 2.0)
    float size = 1.0f;       // Body size multiplier (0.7 - 1.5)
    float efficiency = 1.0f; // Metabolic efficiency (0.7 - 1.3)
    
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
        if (RandomFloat(0, 1) < rate) {
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
    std::unique_ptr<IBrain> brain;
    Phenotype phenotype;
    bool active = true;

    float lifespan = 0.0f;
    int childrenCount = 0;
    int fruitsEaten = 0;
    int poisonsAvoided = 0;
    int obstaclesHit = 0; 
    
    float totalReward = 0.0f;
    std::vector<float> lastInputs;
    std::vector<float> lastOutputs;
    
    Vector2 targetFruit = {-1, -1};
    Vector2 targetPoison = {-1, -1};
    
    Agent() : pos({0,0}), angle(0), energy(0), sex(Sex::Male) {
        brain = std::make_unique<NeuralNetwork>(6, 8, 2);
    }
    
    Agent(Vector2 p) : pos(p), angle(RandomFloat(0, 2*PI)), energy(Config::AGENT_START_ENERGY), 
                       sex(RandomFloat(0,1) > 0.5f ? Sex::Male : Sex::Female) {
        brain = std::make_unique<NeuralNetwork>(6, 8, 2);
    }
    
    Agent(Vector2 p, const IBrain& net, const Phenotype& pheno) 
        : pos(p), angle(RandomFloat(0, 2*PI)), 
          energy(Config::AGENT_START_ENERGY),
          sex(RandomFloat(0,1) > 0.5f ? Sex::Male : Sex::Female),
          phenotype(pheno) {
          brain = net.Clone();
    }

    // Deep Copy Constructor
    Agent(const Agent& other) 
        : pos(other.pos), angle(other.angle), energy(other.energy), 
          sex(other.sex), phenotype(other.phenotype), active(other.active),
          lifespan(other.lifespan), childrenCount(other.childrenCount),
          fruitsEaten(other.fruitsEaten), poisonsAvoided(other.poisonsAvoided),
          obstaclesHit(other.obstaclesHit), totalReward(other.totalReward),
          lastInputs(other.lastInputs), lastOutputs(other.lastOutputs),
          targetFruit(other.targetFruit), targetPoison(other.targetPoison)
    {
        if (other.brain) brain = other.brain->Clone();
    }
    
    // Deep Copy Assignment
    Agent& operator=(const Agent& other) {
        if (this != &other) {
             pos = other.pos;
             angle = other.angle;
             energy = other.energy;
             sex = other.sex;
             phenotype = other.phenotype;
             active = other.active;
             lifespan = other.lifespan;
             childrenCount = other.childrenCount;
             fruitsEaten = other.fruitsEaten;
             poisonsAvoided = other.poisonsAvoided;
             obstaclesHit = other.obstaclesHit;
             totalReward = other.totalReward;
             lastInputs = other.lastInputs;
             lastOutputs = other.lastOutputs;
             targetFruit = other.targetFruit;
             targetPoison = other.targetPoison;
             
             if (other.brain) brain = other.brain->Clone();
             else brain.reset();
        }
        return *this;
    }

    Agent(Agent&&) = default;
    Agent& operator=(Agent&&) = default;
    
    float CalculateFitness() const {
        float baseFitness = lifespan * 0.3f +
                           childrenCount * 15.0f +
                           fruitsEaten * 2.0f +
                           poisonsAvoided * 0.5f +
                           totalReward * 0.1f;
        
        float obstaclePenalty = obstaclesHit * 1.0f;
        
        if (lifespan > 0) {
            float hitRate = obstaclesHit / lifespan;
            if (hitRate > 0.5f) {
                obstaclePenalty += (hitRate - 0.5f) * 10.0f;
            }
        }
        
        return std::max(0.0f, baseFitness - obstaclePenalty);
    }
};