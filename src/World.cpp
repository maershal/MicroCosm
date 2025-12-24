#include "World.hpp"
#include <algorithm>

// --- Spatial Grid Implementation ---
void SpatialGrid::Clear() {
    for(int x=0; x<Config::GRID_W; x++)
        for(int y=0; y<Config::GRID_H; y++) {
            fruitIndices[x][y].clear();
            poisonIndices[x][y].clear();
            agentIndices[x][y].clear();
            obstacleIndices[x][y].clear();
        }
}

void SpatialGrid::AddFruit(int index, Vector2 pos) {
    int gx = (int)pos.x / Config::GRID_CELL_SIZE;
    int gy = (int)pos.y / Config::GRID_CELL_SIZE;
    if (gx >= 0 && gx < Config::GRID_W && gy >= 0 && gy < Config::GRID_H)
        fruitIndices[gx][gy].push_back(index);
}

void SpatialGrid::AddPoison(int index, Vector2 pos) {
    int gx = (int)pos.x / Config::GRID_CELL_SIZE;
    int gy = (int)pos.y / Config::GRID_CELL_SIZE;
    if (gx >= 0 && gx < Config::GRID_W && gy >= 0 && gy < Config::GRID_H)
        poisonIndices[gx][gy].push_back(index);
}

void SpatialGrid::AddAgent(int index, Vector2 pos) {
    int gx = (int)pos.x / Config::GRID_CELL_SIZE;
    int gy = (int)pos.y / Config::GRID_CELL_SIZE;
    if (gx >= 0 && gx < Config::GRID_W && gy >= 0 && gy < Config::GRID_H)
        agentIndices[gx][gy].push_back(index);
}

void SpatialGrid::AddObstacle(int index, Vector2 pos, Vector2 size) {
    int gxStart = (int)pos.x / Config::GRID_CELL_SIZE;
    int gyStart = (int)pos.y / Config::GRID_CELL_SIZE;
    int gxEnd = (int)(pos.x + size.x) / Config::GRID_CELL_SIZE;
    int gyEnd = (int)(pos.y + size.y) / Config::GRID_CELL_SIZE;
    
    for (int x = std::max(0, gxStart); x <= std::min(Config::GRID_W - 1, gxEnd); ++x) {
        for (int y = std::max(0, gyStart); y <= std::min(Config::GRID_H - 1, gyEnd); ++y) {
            obstacleIndices[x][y].push_back(index);
        }
    }
}

// --- World Implementation ---

World::World() {
    if (Config::OBSTACLES_ENABLED) {
        GenerateRandomObstacles();
    }
    InitPopulation();
}

Vector2 World::FindSafeSpawnPosition(float minRadius, int maxAttempts) {
    for (int attempt = 0; attempt < maxAttempts; ++attempt) {
        Vector2 pos = {
            RandomFloat(minRadius + 50, Config::SCREEN_W - minRadius - 50), 
            RandomFloat(minRadius + 50, Config::SCREEN_H - minRadius - 50)
        };
        
        // Check if position collides with any obstacle
        if (!CheckObstacleCollision(pos, minRadius)) {
            return pos;
        }
    }
    
    // Fallback: try center area
    for (int attempt = 0; attempt < 20; ++attempt) {
        Vector2 pos = {
            Config::SCREEN_W / 2.0f + RandomFloat(-100, 100),
            Config::SCREEN_H / 2.0f + RandomFloat(-100, 100)
        };
        if (!CheckObstacleCollision(pos, minRadius)) {
            return pos;
        }
    }
    
    // Last resort: return center
    return {Config::SCREEN_W / 2.0f, Config::SCREEN_H / 2.0f};
}

void World::GenerateRandomObstacles() {
    obstacles.clear();
    
    for (int i = 0; i < Config::OBSTACLE_COUNT; ++i) {
        Vector2 pos = {RandomFloat(100, Config::SCREEN_W - 300), 
                      RandomFloat(100, Config::SCREEN_H - 300)};
        Vector2 size = {RandomFloat(60, 120), RandomFloat(60, 120)};
        
        // Random obstacle type
        ObstacleType type = (ObstacleType)(int)RandomFloat(0, 4);
        obstacles.push_back(Obstacle(pos, size, type));
    }
}

void World::GenerateMaze() {
    obstacles.clear();
    
    int wallThickness = 15;
    int gridSize = 4;
    float cellWidth = (Config::SCREEN_W - 200) / gridSize;
    float cellHeight = (Config::SCREEN_H - 200) / gridSize;
    
    // Create grid walls with strategic gaps
    for (int i = 0; i <= gridSize; ++i) {
        // Horizontal walls
        if (i < gridSize) {
            float y = 100 + i * cellHeight;
            for (int j = 0; j < gridSize; ++j) {
                if (RandomFloat(0, 100) < 60) { // 60% chance of wall segment
                    float x = 100 + j * cellWidth;
                    obstacles.push_back(Obstacle(
                        {x, y}, 
                        {cellWidth * 0.8f, wallThickness}, 
                        ObstacleType::Wall
                    ));
                }
            }
        }
        
        // Vertical walls
        if (i < gridSize) {
            float x = 100 + i * cellWidth;
            for (int j = 0; j < gridSize; ++j) {
                if (RandomFloat(0, 100) < 60) { // 60% chance of wall segment
                    float y = 100 + j * cellHeight;
                    obstacles.push_back(Obstacle(
                        {x, y}, 
                        {wallThickness, cellHeight * 0.8f}, 
                        ObstacleType::Wall
                    ));
                }
            }
        }
    }
    
    // Add some circular obstacles at intersections
    for (int i = 1; i < gridSize; ++i) {
        for (int j = 1; j < gridSize; ++j) {
            if (RandomFloat(0, 100) < 30) {
                float x = 100 + i * cellWidth - 20;
                float y = 100 + j * cellHeight - 20;
                obstacles.push_back(Obstacle({x, y}, {40, 40}, ObstacleType::Circle));
            }
        }
    }
}

void World::GenerateArena() {
    obstacles.clear();
    
    int wallThickness = 20;
    
    // Outer border walls
    obstacles.push_back(Obstacle({50, 50}, {Config::SCREEN_W - 100, wallThickness}, ObstacleType::Wall));
    obstacles.push_back(Obstacle({50, Config::SCREEN_H - 70}, {Config::SCREEN_W - 100, wallThickness}, ObstacleType::Wall));
    obstacles.push_back(Obstacle({50, 50}, {wallThickness, Config::SCREEN_H - 100}, ObstacleType::Wall));
    obstacles.push_back(Obstacle({Config::SCREEN_W - 70, 50}, {wallThickness, Config::SCREEN_H - 100}, ObstacleType::Wall));
    
    // Central structure - mix of shapes
    float centerX = Config::SCREEN_W / 2.0f;
    float centerY = Config::SCREEN_H / 2.0f;
    
    // Large central circle
    obstacles.push_back(Obstacle({centerX - 60, centerY - 60}, {120, 120}, ObstacleType::Circle));
    
    // Four L-shapes in corners creating chambers
    obstacles.push_back(Obstacle({150, 150}, {100, 100}, ObstacleType::L_Shape));
    obstacles.push_back(Obstacle({Config::SCREEN_W - 250, 150}, {100, 100}, ObstacleType::L_Shape));
    obstacles.push_back(Obstacle({150, Config::SCREEN_H - 250}, {100, 100}, ObstacleType::L_Shape));
    obstacles.push_back(Obstacle({Config::SCREEN_W - 250, Config::SCREEN_H - 250}, {100, 100}, ObstacleType::L_Shape));
    
    // Corridors connecting areas
    obstacles.push_back(Obstacle({centerX - 150, centerY - 10}, {120, 20}, ObstacleType::Corridor));
    obstacles.push_back(Obstacle({centerX + 30, centerY - 10}, {120, 20}, ObstacleType::Corridor));
    obstacles.push_back(Obstacle({centerX - 10, centerY - 150}, {20, 120}, ObstacleType::Corridor));
    obstacles.push_back(Obstacle({centerX - 10, centerY + 30}, {20, 120}, ObstacleType::Corridor));
}

void World::GenerateRooms() {
    obstacles.clear();
    
    int wallThickness = 15;
    
    // Create a layout with distinct rooms
    float midX = Config::SCREEN_W / 2.0f;
    float midY = Config::SCREEN_H / 2.0f;
    
    // Horizontal divider with gaps (doorways)
    obstacles.push_back(Obstacle({100, midY - wallThickness/2}, {midX - 150, wallThickness}, ObstacleType::Wall));
    obstacles.push_back(Obstacle({midX + 50, midY - wallThickness/2}, {Config::SCREEN_W - midX - 150, wallThickness}, ObstacleType::Wall));
    
    // Vertical divider with gaps
    obstacles.push_back(Obstacle({midX - wallThickness/2, 100}, {wallThickness, midY - 150}, ObstacleType::Wall));
    obstacles.push_back(Obstacle({midX - wallThickness/2, midY + 50}, {wallThickness, Config::SCREEN_H - midY - 150}, ObstacleType::Wall));
    
    // Add furniture/obstacles in each room
    int roomCount = 4;
    float roomPositions[4][2] = {
        {Config::SCREEN_W * 0.25f, Config::SCREEN_H * 0.25f},
        {Config::SCREEN_W * 0.75f, Config::SCREEN_H * 0.25f},
        {Config::SCREEN_W * 0.25f, Config::SCREEN_H * 0.75f},
        {Config::SCREEN_W * 0.75f, Config::SCREEN_H * 0.75f}
    };
    
    for (int i = 0; i < roomCount; ++i) {
        float x = roomPositions[i][0];
        float y = roomPositions[i][1];
        
        // Add 1-3 obstacles per room
        int obstacleCount = 1 + (int)RandomFloat(0, 3);
        for (int j = 0; j < obstacleCount; ++j) {
            float offsetX = RandomFloat(-80, 80);
            float offsetY = RandomFloat(-80, 80);
            Vector2 obsPos = {x + offsetX, y + offsetY};
            Vector2 obsSize = {RandomFloat(30, 70), RandomFloat(30, 70)};
            
            ObstacleType type = (RandomFloat(0, 2) < 1) ? ObstacleType::Circle : ObstacleType::Wall;
            obstacles.push_back(Obstacle(obsPos, obsSize, type));
        }
    }
}

void World::GenerateSpiral() {
    obstacles.clear();
    
    int wallThickness = 15;
    float centerX = Config::SCREEN_W / 2.0f;
    float centerY = Config::SCREEN_H / 2.0f;
    
    // Create a spiral pattern
    int segments = 20;
    float angleStep = 360.0f / segments;
    float radiusStep = 15.0f;
    
    for (int i = 0; i < segments; ++i) {
        float angle = (i * angleStep) * DEG2RAD;
        float radius = 50 + i * radiusStep;
        
        float x = centerX + cos(angle) * radius;
        float y = centerY + sin(angle) * radius;
        
        float nextAngle = ((i + 1) * angleStep) * DEG2RAD;
        float nextRadius = 50 + (i + 1) * radiusStep;
        float nextX = centerX + cos(nextAngle) * nextRadius;
        float nextY = centerY + sin(nextAngle) * nextRadius;
        
        // Calculate wall orientation
        float dx = nextX - x;
        float dy = nextY - y;
        float length = sqrt(dx * dx + dy * dy);
        
        obstacles.push_back(Obstacle({x - wallThickness/2, y - wallThickness/2}, {length, wallThickness}, ObstacleType::Wall));
    }
    
    // Add some circular obstacles for variety
    for (int i = 0; i < 8; ++i) {
        float angle = (i * 45) * DEG2RAD;
        float radius = 150 + RandomFloat(-30, 30);
        float x = centerX + cos(angle) * radius - 20;
        float y = centerY + sin(angle) * radius - 20;
        obstacles.push_back(Obstacle({x, y}, {40, 40}, ObstacleType::Circle));
    }
}

void World::ClearObstacles() {
    obstacles.clear();
}

bool World::CheckObstacleCollision(Vector2 pos, float radius) {
    // Check all obstacles using proper collision detection
    for (const auto& obs : obstacles) {
        if (obs.active && obs.Intersects(pos, radius)) {
            return true;
        }
    }
    return false;
}

void World::InitPopulation() {
    agents.clear();
    fruits.clear();
    poisons.clear();
    
    if(!savedGenetics.empty()) {
        std::sort(savedGenetics.begin(), savedGenetics.end(), 
                  [](const GeneticRecord& a, const GeneticRecord& b) {
                      return a.fitness > b.fitness;
                  });
        
        if (savedGenetics.size() > 30) {
            savedGenetics.erase(savedGenetics.begin() + 30, savedGenetics.end());
        }
        
        int totalAgents = 180;
        int randomAgents = totalAgents / 10;
        int eliteAgents = totalAgents / 5;
        int weakMutationAgents = totalAgents / 2 - randomAgents;
        int strongMutationAgents = totalAgents - randomAgents - eliteAgents - weakMutationAgents;
        
        // Elite preservation - use safe spawn
        for(int i = 0; i < eliteAgents && i < savedGenetics.size(); i++) {
            Vector2 startPos = FindSafeSpawnPosition(15.0f);
            agents.emplace_back(startPos, *savedGenetics[i].brain, savedGenetics[i].phenotype);
        }
        
        // Weak mutation - use safe spawn
        for(int i = 0; i < weakMutationAgents; i++) {
            Vector2 startPos = FindSafeSpawnPosition(15.0f);
            int parentIdx = (int)RandomFloat(0, savedGenetics.size());
            std::unique_ptr<IBrain> childBrain = savedGenetics[parentIdx].brain->Clone();
            childBrain->Mutate(0.15f, 0.08f);
            Phenotype childPheno = savedGenetics[parentIdx].phenotype;
            childPheno.Mutate(0.1f);
            agents.emplace_back(startPos, *childBrain, childPheno);
        }
        
        // Strong mutation - use safe spawn
        for(int i = 0; i < strongMutationAgents; i++) {
            Vector2 startPos = FindSafeSpawnPosition(15.0f);
            int parentIdx = (int)RandomFloat(0, savedGenetics.size());
            std::unique_ptr<IBrain> childBrain = savedGenetics[parentIdx].brain->Clone();
            childBrain->Mutate(0.3f, 0.25f);
            Phenotype childPheno = savedGenetics[parentIdx].phenotype;
            childPheno.Mutate(0.3f);
            agents.emplace_back(startPos, *childBrain, childPheno);
        }
        
        // Random agents - use safe spawn
        for(int i = 0; i < randomAgents; i++) {
            Vector2 startPos = FindSafeSpawnPosition(15.0f);
            agents.emplace_back(startPos);
        }
        
        savedGenetics.clear();
    }
    else {
        // First generation - ALSO use safe spawn positions
        for(int i=0; i<180; i++) {
            Vector2 startPos = FindSafeSpawnPosition(15.0f);
            agents.emplace_back(startPos);
        }
    }
    
    // Spawn fruits in safe locations
    for(int i=0; i<60; i++) {
        Vector2 pos = FindSafeSpawnPosition(5.0f, 30);
        fruits.push_back({pos});
    }
    
    // Spawn poisons in safe locations
    for(int i=0; i<20; i++) {
        Vector2 pos = FindSafeSpawnPosition(5.0f, 30);
        poisons.push_back({pos});
    }
    
    stats.generation++;
    stats.avgFitness = 0.0f;
    stats.bestFitness = 0.0f;
}

template <typename T>
void World::CleanupEntities(std::vector<T>& entities) {
    entities.erase(std::remove_if(entities.begin(), entities.end(), 
                   [](const T& e) { return !e.active; }), entities.end());
}

SensorData World::ScanSurroundings(Agent& agent) {
    SensorData data;
    float minFruitDistSqr = Config::AGENT_VISION_RADIUS * Config::AGENT_VISION_RADIUS;
    float minPoisonDistSqr = minFruitDistSqr;
    float minObstacleDistSqr = minFruitDistSqr;
    
    int gx = (int)agent.pos.x / Config::GRID_CELL_SIZE;
    int gy = (int)agent.pos.y / Config::GRID_CELL_SIZE;
    int range = (int)(Config::AGENT_VISION_RADIUS / Config::GRID_CELL_SIZE) + 1;

    agent.targetFruit = {-1, -1};
    agent.targetPoison = {-1, -1};
    
    bool sawPoison = false;

    for (int x = gx - range; x <= gx + range; x++) {
        for (int y = gy - range; y <= gy + range; y++) {
            if (x < 0 || x >= Config::GRID_W || y < 0 || y >= Config::GRID_H) continue;

            for (int idx : grid.fruitIndices[x][y]) {
                if (!fruits[idx].active) continue;
                float dSqr = Vector2DistanceSqr(agent.pos, fruits[idx].pos);
                if (dSqr < minFruitDistSqr) {
                    minFruitDistSqr = dSqr;
                    agent.targetFruit = fruits[idx].pos;
                    float angleTo = atan2(fruits[idx].pos.y - agent.pos.y, fruits[idx].pos.x - agent.pos.x);
                    data.fruitAngle = NormalizeAngle(angleTo - agent.angle) / PI;
                    data.fruitDist = sqrt(dSqr) / Config::AGENT_VISION_RADIUS;
                }
            }
            
            for (int idx : grid.poisonIndices[x][y]) {
                if (!poisons[idx].active) continue;
                float dSqr = Vector2DistanceSqr(agent.pos, poisons[idx].pos);
                if (dSqr < minPoisonDistSqr) {
                    minPoisonDistSqr = dSqr;
                    agent.targetPoison = poisons[idx].pos;
                    float angleTo = atan2(poisons[idx].pos.y - agent.pos.y, poisons[idx].pos.x - agent.pos.x);
                    data.poisonAngle = NormalizeAngle(angleTo - agent.angle) / PI;
                    data.poisonDist = sqrt(dSqr) / Config::AGENT_VISION_RADIUS;
                    sawPoison = true;
                }
            }
        }
    }
    
    // Improved obstacle detection - check all obstacles
    float visionRadiusSqr = Config::AGENT_VISION_RADIUS * Config::AGENT_VISION_RADIUS;
    for (const auto& obs : obstacles) {
        if (!obs.active) continue;
        
        Vector2 center = {obs.pos.x + obs.size.x / 2, obs.pos.y + obs.size.y / 2};
        float dSqr = Vector2DistanceSqr(agent.pos, center);
        
        if (dSqr < visionRadiusSqr && dSqr < minObstacleDistSqr) {
            minObstacleDistSqr = dSqr;
            float angleTo = atan2(center.y - agent.pos.y, center.x - agent.pos.x);
            data.obstacleAngle = NormalizeAngle(angleTo - agent.angle) / PI;
            data.obstacleDist = sqrt(dSqr) / Config::AGENT_VISION_RADIUS;
        }
    }
    
    if (sawPoison) {
        agent.poisonsAvoided++;
    }
    
    return data;
}

void World::HandleInteractions(Agent& agent, std::vector<Agent>& babies) {
    float eatRadiusSqr = 15.0f * 15.0f; 
    int gx = (int)agent.pos.x / Config::GRID_CELL_SIZE;
    int gy = (int)agent.pos.y / Config::GRID_CELL_SIZE;
    
    float reward = 0.0f;

    for(int x = gx-1; x <= gx+1; x++) {
        for(int y = gy-1; y <= gy+1; y++) {
            if (x < 0 || x >= Config::GRID_W || y < 0 || y >= Config::GRID_H) continue;
            
            for (int idx : grid.fruitIndices[x][y]) {
                if (fruits[idx].active && Vector2DistanceSqr(agent.pos, fruits[idx].pos) < eatRadiusSqr) {
                    agent.energy = std::min(agent.energy + Config::FRUIT_ENERGY, Config::AGENT_MAX_ENERGY);
                    fruits[idx].active = false;
                    agent.fruitsEaten++;
                    reward += 1.0f;
                }
            }
            
            for (int idx : grid.poisonIndices[x][y]) {
                if (poisons[idx].active && Vector2DistanceSqr(agent.pos, poisons[idx].pos) < eatRadiusSqr) {
                    agent.energy -= Config::POISON_DAMAGE;
                    poisons[idx].active = false;
                    agent.poisonsAvoided = std::max(0, agent.poisonsAvoided - 5);
                    reward -= 2.0f;
                }
            }
            
            if (agent.sex == Sex::Female && agent.energy > 120.0f) {
                 for (int idx : grid.agentIndices[x][y]) {
                    Agent& partner = agents[idx];
                    if (&partner == &agent) continue;
                    if (!partner.active) continue;
                    if (partner.sex == Sex::Male && partner.energy > 120.0f) {
                        if (Vector2DistanceSqr(agent.pos, partner.pos) < 2500.0f) {
                            agent.energy -= 60.0f;
                            partner.energy -= 60.0f;
                            
                            // Spawn child at safe location near parents
                            Vector2 childBasePos = Vector2Add(agent.pos, partner.pos);
                            childBasePos = Vector2Scale(childBasePos, 0.5f);
                            
                            Vector2 childPos = childBasePos;
                            // Try to find safe position near parents
                            for (int attempt = 0; attempt < 10; ++attempt) {
                                Vector2 testPos = {
                                    childBasePos.x + RandomFloat(-30, 30),
                                    childBasePos.y + RandomFloat(-30, 30)
                                };
                                if (!CheckObstacleCollision(testPos, 10.0f)) {
                                    childPos = testPos;
                                    break;
                                }
                            }
                            
                            Agent child(childPos);
                            child.brain = agent.brain->Crossover(*partner.brain);
                            child.brain->Mutate(0.1f, 0.15f);
                            child.phenotype = Phenotype::Crossover(agent.phenotype, partner.phenotype);
                            child.phenotype.Mutate(0.1f);
                            babies.push_back(child);
                            
                            agent.childrenCount++;
                            partner.childrenCount++;
                            
                            reward += 0.5f;
                            return;
                        }
                    }
                 }
            }
        }
    }
    
    if (Config::ENABLE_LIFETIME_LEARNING && reward != 0.0f) {
        agent.totalReward += reward;
        agent.brain->LearnFromReward(reward, Config::LEARNING_RATE);
    }
}

void World::Update(float dt) {
    stats.time += dt;

    grid.Clear();
    for(size_t i=0; i<fruits.size(); ++i) if(fruits[i].active) grid.AddFruit(i, fruits[i].pos);
    for(size_t i=0; i<poisons.size(); ++i) if(poisons[i].active) grid.AddPoison(i, poisons[i].pos);
    for(size_t i=0; i<agents.size(); ++i) if(agents[i].active) grid.AddAgent(i, agents[i].pos);
    for(size_t i=0; i<obstacles.size(); ++i) if(obstacles[i].active) grid.AddObstacle(i, obstacles[i].pos, obstacles[i].size);

    std::vector<Agent> babies;
    
    // Track phenotype averages
    float totalSpeed = 0, totalSize = 0, totalEfficiency = 0;
    int activeCount = 0;

    for(auto& agent : agents) {
        if (!agent.active) continue;
        
        activeCount++;
        totalSpeed += agent.phenotype.speed;
        totalSize += agent.phenotype.size;
        totalEfficiency += agent.phenotype.efficiency;

        UpdateAgent(agent, dt, babies);
    }
    
    if (activeCount > 0) {
        stats.avgSpeed = totalSpeed / activeCount;
        stats.avgSize = totalSize / activeCount;
        stats.avgEfficiency = totalEfficiency / activeCount;
    }

    if (!babies.empty()) {
        stats.births += (int)babies.size();
        agents.insert(agents.end(), std::make_move_iterator(babies.begin()), std::make_move_iterator(babies.end()));
    }

    CleanupEntities(agents);
    CleanupEntities(fruits);
    CleanupEntities(poisons);

    if (fruits.size() < 40) {
        Vector2 pos = FindSafeSpawnPosition(5.0f, 30);
        fruits.push_back({pos});
    }
    if (poisons.size() < 15) {
        Vector2 pos = FindSafeSpawnPosition(5.0f, 30);
        poisons.push_back({pos});
    }

    if (agents.empty()) {
        if (stats.deaths > 0) {
            stats.avgFitness = stats.totalFitness / stats.deaths;
        }
        
        // Record history
        stats.history.push_back({
            stats.avgFitness,
            stats.bestFitness,
            stats.avgSpeed,
            stats.avgSize,
            stats.maxPop
        });
        
        InitPopulation();
        stats.totalFitness = 0.0f;
    }
    if (agents.size() > (size_t)stats.maxPop) stats.maxPop = (int)agents.size();
}

void World::UpdateAgent(Agent& agent, float dt, std::vector<Agent>& babies) {
    agent.lifespan += dt;

    SensorData data = ScanSurroundings(agent);
    std::vector<float> inputs = {
        data.fruitAngle, data.fruitDist, 
        data.poisonAngle, data.poisonDist,
        data.obstacleAngle, data.obstacleDist
    };

    auto outputs = agent.brain->FeedForward(inputs);

    float leftTrack = outputs[0];
    float rightTrack = outputs[1];
    float rotSpeed = 3.0f;
    float moveSpeed = 120.0f * agent.phenotype.GetActualSpeed();

    agent.angle += (leftTrack - rightTrack) * rotSpeed * dt;
    Vector2 forward = { cos(agent.angle), sin(agent.angle) };
    float throttle = std::clamp((leftTrack + rightTrack) / 2.0f, -0.2f, 1.0f);

    Vector2 newPos = Vector2Add(agent.pos, Vector2Scale(forward, throttle * moveSpeed * dt));
    
    // Check collision before moving
    float agentRadius = agent.phenotype.GetVisualSize();
    if (!CheckObstacleCollision(newPos, agentRadius)) {
        agent.pos = newPos;
    } else {
        // Collision detected
        agent.obstaclesHit++;
        agent.energy -= Config::COLLISION_ENERGY_PENALTY; 
        
        if (Config::ENABLE_LIFETIME_LEARNING) {
            agent.brain->LearnFromReward(-1.0f, Config::LEARNING_RATE * Config::COLLISION_LEARNING_BOOST);
        }
        
        // Sliding logic
        Vector2 slideDir = {-forward.y, forward.x};
        Vector2 slidePos1 = Vector2Add(agent.pos, Vector2Scale(slideDir, throttle * moveSpeed * dt * 0.5f));
        Vector2 slidePos2 = Vector2Add(agent.pos, Vector2Scale(slideDir, -throttle * moveSpeed * dt * 0.5f));
        
        if (!CheckObstacleCollision(slidePos1, agentRadius)) {
            agent.pos = slidePos1;
        } else if (!CheckObstacleCollision(slidePos2, agentRadius)) {
            agent.pos = slidePos2;
        }
    }

    // Screen wrapping with safety check
    Vector2 wrappedPos = agent.pos;
    bool needsWrap = false;
    if (agent.pos.x < 0) { wrappedPos.x = Config::SCREEN_W; needsWrap = true; }
    else if (agent.pos.x > Config::SCREEN_W) { wrappedPos.x = 0; needsWrap = true; }
    if (agent.pos.y < 0) { wrappedPos.y = Config::SCREEN_H; needsWrap = true; }
    else if (agent.pos.y > Config::SCREEN_H) { wrappedPos.y = 0; needsWrap = true; }
    
    if (needsWrap && !CheckObstacleCollision(wrappedPos, agentRadius)) {
        agent.pos = wrappedPos;
    } else if (needsWrap) {
        agent.pos.x = std::clamp(agent.pos.x, agentRadius, Config::SCREEN_W - agentRadius);
        agent.pos.y = std::clamp(agent.pos.y, agentRadius, Config::SCREEN_H - agentRadius);
    }

    float metabolismRate = Config::METABOLISM_RATE * agent.phenotype.GetMetabolicRate();
    agent.energy -= metabolismRate * dt;
    
    if (agent.energy <= 0) {
        agent.active = false;
        stats.deaths++;
        
        float fitness = agent.CalculateFitness();
        stats.totalFitness += fitness;
        if (fitness > stats.bestFitness) stats.bestFitness = fitness;

        size_t activeCount = std::count_if(agents.begin(), agents.end(), [](const Agent& a){ return a.active; });
        if (activeCount <= (size_t)Config::ACTIVE_AGENTS && fitness > 5.0f) {
            savedGenetics.push_back({*agent.brain, agent.phenotype, fitness});
        }
        return;
    }

    HandleInteractions(agent, babies);
}

void World::Draw() {
    for (const auto& obs : obstacles) {
        if (obs.active) {
            obs.Draw();
        }
    }
}
