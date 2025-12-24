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
    
    // For circular obstacles
    float radius = 0.0f;
    
    Obstacle(Vector2 p, Vector2 s, ObstacleType t = ObstacleType::Wall) 
        : pos(p), size(s), type(t) {
        if (type == ObstacleType::Circle) {
            radius = std::min(size.x, size.y) / 2.0f;
        }
        // Vary colors slightly for visual interest
        color.r = 80 + rand() % 30;
        color.g = 80 + rand() % 30;
        color.b = 80 + rand() % 30;
    }
    
    bool Contains(Vector2 point) const {
        switch (type) {
            case ObstacleType::Circle: {
                Vector2 center = {pos.x + size.x / 2, pos.y + size.y / 2};
                return Vector2Distance(point, center) <= radius;
            }
            case ObstacleType::L_Shape: {
                // L-shape is two rectangles
                bool inVertical = point.x >= pos.x && point.x <= pos.x + size.x * 0.3f &&
                                 point.y >= pos.y && point.y <= pos.y + size.y;
                bool inHorizontal = point.x >= pos.x && point.x <= pos.x + size.x &&
                                   point.y >= pos.y + size.y * 0.7f && point.y <= pos.y + size.y;
                return inVertical || inHorizontal;
            }
            case ObstacleType::Corridor: {
                // Corridor is like a wall but with gaps
                bool inWall = point.x >= pos.x && point.x <= pos.x + size.x &&
                             point.y >= pos.y && point.y <= pos.y + size.y;
                // Create gaps every 40% of length
                float relX = (point.x - pos.x) / size.x;
                bool inGap = (relX > 0.35f && relX < 0.45f) || (relX > 0.55f && relX < 0.65f);
                return inWall && !inGap;
            }
            default: // Wall
                return point.x >= pos.x && point.x <= pos.x + size.x &&
                       point.y >= pos.y && point.y <= pos.y + size.y;
        }
    }
    
    bool Intersects(Vector2 point, float checkRadius) const {
        switch (type) {
            case ObstacleType::Circle: {
                Vector2 center = {pos.x + size.x / 2, pos.y + size.y / 2};
                return Vector2Distance(point, center) <= (radius + checkRadius);
            }
            case ObstacleType::L_Shape: {
                // Check both parts of the L
                Rectangle vert = {pos.x, pos.y, size.x * 0.3f, size.y};
                Rectangle horiz = {pos.x, pos.y + size.y * 0.7f, size.x, size.y * 0.3f};
                return CheckCollisionCircleRec(point, checkRadius, vert) ||
                       CheckCollisionCircleRec(point, checkRadius, horiz);
            }
            case ObstacleType::Corridor: {
                Rectangle full = {pos.x, pos.y, size.x, size.y};
                if (!CheckCollisionCircleRec(point, checkRadius, full)) return false;
                
                // Check if in gap areas
                float relX = (point.x - pos.x) / size.x;
                bool inGap = (relX > 0.35f && relX < 0.45f) || (relX > 0.55f && relX < 0.65f);
                return !inGap;
            }
            default: { // Wall
                float closestX = std::clamp(point.x, pos.x, pos.x + size.x);
                float closestY = std::clamp(point.y, pos.y, pos.y + size.y);
                float distX = point.x - closestX;
                float distY = point.y - closestY;
                return (distX * distX + distY * distY) < (checkRadius * checkRadius);
            }
        }
    }
    
    void Draw() const {
        switch (type) {
            case ObstacleType::Circle: {
                Vector2 center = {pos.x + size.x / 2, pos.y + size.y / 2};
                DrawCircleV(center, radius, color);
                DrawCircleLines(center.x, center.y, radius, {color.r + 40, color.g + 40, color.b + 40, 255});
                break;
            }
            case ObstacleType::L_Shape: {
                // Draw vertical part
                DrawRectangle(pos.x, pos.y, size.x * 0.3f, size.y, color);
                DrawRectangleLines(pos.x, pos.y, size.x * 0.3f, size.y, {color.r + 40, color.g + 40, color.b + 40, 255});
                // Draw horizontal part
                DrawRectangle(pos.x, pos.y + size.y * 0.7f, size.x, size.y * 0.3f, color);
                DrawRectangleLines(pos.x, pos.y + size.y * 0.7f, size.x, size.y * 0.3f, {color.r + 40, color.g + 40, color.b + 40, 255});
                break;
            }
            case ObstacleType::Corridor: {
                // Draw full wall
                DrawRectangleV(pos, size, color);
                // Draw gaps
                Color gapColor = {30, 30, 35, 255};
                float gapWidth = size.x * 0.1f;
                DrawRectangle(pos.x + size.x * 0.35f, pos.y, gapWidth, size.y, gapColor);
                DrawRectangle(pos.x + size.x * 0.55f, pos.y, gapWidth, size.y, gapColor);
                DrawRectangleLinesEx({pos.x, pos.y, size.x, size.y}, 2, {color.r + 40, color.g + 40, color.b + 40, 255});
                break;
            }
            default: // Wall
                DrawRectangleV(pos, size, color);
                DrawRectangleLinesEx({pos.x, pos.y, size.x, size.y}, 2, {color.r + 40, color.g + 40, color.b + 40, 255});
                break;
        }
    }
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