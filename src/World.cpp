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
    
    for (int x = gxStart; x <= gxEnd && x < Config::GRID_W; ++x) {
        for (int y = gyStart; y <= gyEnd && y < Config::GRID_H; ++y) {
            if (x >= 0 && y >= 0)
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
        
        if (!CheckObstacleCollision(pos, minRadius)) {
            return pos;
        }
    }
    
    // Fallback: return center position
    return {Config::SCREEN_W / 2.0f, Config::SCREEN_H / 2.0f};
}

void World::GenerateRandomObstacles() {
    obstacles.clear();
    
    for (int i = 0; i < Config::OBSTACLE_COUNT; ++i) {
        Vector2 pos = {RandomFloat(100, Config::SCREEN_W - 200), 
                      RandomFloat(100, Config::SCREEN_H - 200)};
        Vector2 size = {RandomFloat(50, 150), RandomFloat(50, 150)};
        
        // Random obstacle type
        int typeChoice = rand() % 4;
        ObstacleType type;
        switch (typeChoice) {
            case 0: type = ObstacleType::Wall; break;
            case 1: type = ObstacleType::Circle; break;
            case 2: type = ObstacleType::L_Shape; break;
            case 3: type = ObstacleType::Corridor; break;
            default: type = ObstacleType::Wall;
        }
        
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
                if (rand() % 100 < 60) { // 60% chance of wall segment
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
                if (rand() % 100 < 60) { // 60% chance of wall segment
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
            if (rand() % 100 < 30) {
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
        int obstacleCount = 1 + rand() % 3;
        for (int j = 0; j < obstacleCount; ++j) {
            float offsetX = RandomFloat(-80, 80);
            float offsetY = RandomFloat(-80, 80);
            Vector2 obsPos = {x + offsetX, y + offsetY};
            Vector2 obsSize = {RandomFloat(30, 70), RandomFloat(30, 70)};
            
            ObstacleType type = (rand() % 2 == 0) ? ObstacleType::Circle : ObstacleType::Wall;
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
    int gx = (int)pos.x / Config::GRID_CELL_SIZE;
    int gy = (int)pos.y / Config::GRID_CELL_SIZE;
    
    for (int x = gx - 1; x <= gx + 1; ++x) {
        for (int y = gy - 1; y <= gy + 1; ++y) {
            if (x < 0 || x >= Config::GRID_W || y < 0 || y >= Config::GRID_H) continue;
            
            for (int idx : grid.obstacleIndices[x][y]) {
                if (obstacles[idx].active && obstacles[idx].Intersects(pos, radius)) {
                    return true;
                }
            }
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
        
        int totalAgents = 80;
        int randomAgents = totalAgents / 10;
        int eliteAgents = totalAgents / 5;
        int weakMutationAgents = totalAgents / 2 - randomAgents;
        int strongMutationAgents = totalAgents - randomAgents - eliteAgents - weakMutationAgents;
        
        // Elite preservation
        for(int i = 0; i < eliteAgents && i < savedGenetics.size(); i++) {
            Vector2 startPos = FindSafeSpawnPosition();
            agents.emplace_back(startPos, savedGenetics[i].brain, savedGenetics[i].phenotype);
        }
        
        // Weak mutation
        for(int i = 0; i < weakMutationAgents; i++) {
            Vector2 startPos = FindSafeSpawnPosition();
            int parentIdx = rand() % savedGenetics.size();
            NeuralNetwork childBrain = savedGenetics[parentIdx].brain;
            childBrain.Mutate(0.15f, 0.08f);
            Phenotype childPheno = savedGenetics[parentIdx].phenotype;
            childPheno.Mutate(0.1f);
            agents.emplace_back(startPos, childBrain, childPheno);
        }
        
        // Strong mutation
        for(int i = 0; i < strongMutationAgents; i++) {
            Vector2 startPos = FindSafeSpawnPosition();
            int parentIdx = rand() % savedGenetics.size();
            NeuralNetwork childBrain = savedGenetics[parentIdx].brain;
            childBrain.Mutate(0.3f, 0.25f);
            Phenotype childPheno = savedGenetics[parentIdx].phenotype;
            childPheno.Mutate(0.3f);
            agents.emplace_back(startPos, childBrain, childPheno);
        }
        
        // Random agents
        for(int i = 0; i < randomAgents; i++) {
            Vector2 startPos = FindSafeSpawnPosition();
            agents.emplace_back(startPos);
        }
        
        savedGenetics.clear();
    }
    else {
        // First generation
        for(int i=0; i<80; i++) {
            Vector2 startPos = FindSafeSpawnPosition();
            agents.emplace_back(startPos);
        }
    }
    
    // Spawn fruits in safe locations
    for(int i=0; i<60; i++) {
        Vector2 pos = FindSafeSpawnPosition(5.0f);
        fruits.push_back({pos});
    }
    
    // Spawn poisons in safe locations
    for(int i=0; i<20; i++) {
        Vector2 pos = FindSafeSpawnPosition(5.0f);
        poisons.push_back({pos});
    }
    
    stats.generation++;
    stats.avgFitness = 0.0f;
    stats.bestFitness = 0.0f;
}

template <typename T>
void World::CleanupEntities(std::vector<T>& entities) {
    size_t aliveCount = 0;
    for (size_t i = 0; i < entities.size(); ++i) {
        if (entities[i].active) {
            if (i != aliveCount) {
                entities[aliveCount] = std::move(entities[i]);
            }
            aliveCount++;
        }
    }
    entities.resize(aliveCount);
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
            
            // NEW: Obstacle sensing
            for (int idx : grid.obstacleIndices[x][y]) {
                if (!obstacles[idx].active) continue;
                Vector2 center = {obstacles[idx].pos.x + obstacles[idx].size.x / 2,
                                 obstacles[idx].pos.y + obstacles[idx].size.y / 2};
                float dSqr = Vector2DistanceSqr(agent.pos, center);
                if (dSqr < minObstacleDistSqr) {
                    minObstacleDistSqr = dSqr;
                    float angleTo = atan2(center.y - agent.pos.y, center.x - agent.pos.x);
                    data.obstacleAngle = NormalizeAngle(angleTo - agent.angle) / PI;
                    data.obstacleDist = sqrt(dSqr) / Config::AGENT_VISION_RADIUS;
                }
            }
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
                    reward += 1.0f;  // Positive reward
                }
            }
            
            for (int idx : grid.poisonIndices[x][y]) {
                if (poisons[idx].active && Vector2DistanceSqr(agent.pos, poisons[idx].pos) < eatRadiusSqr) {
                    agent.energy -= Config::POISON_DAMAGE;
                    poisons[idx].active = false;
                    agent.poisonsAvoided = std::max(0, agent.poisonsAvoided - 5);
                    reward -= 2.0f;  // Negative reward
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
                            
                            Agent child(agent.pos);
                            child.brain = NeuralNetwork::Crossover(agent.brain, partner.brain);
                            child.brain.Mutate(0.1f, 0.15f);
                            child.phenotype = Phenotype::Crossover(agent.phenotype, partner.phenotype);
                            child.phenotype.Mutate(0.1f);
                            babies.push_back(child);
                            
                            agent.childrenCount++;
                            partner.childrenCount++;
                            
                            reward += 0.5f;  // Small reward for reproduction
                            return;
                        }
                    }
                 }
            }
        }
    }
    
    if (Config::ENABLE_LIFETIME_LEARNING && reward != 0.0f) {
        agent.totalReward += reward;
        agent.brain.LearnFromReward(reward, Config::LEARNING_RATE);
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
        
        agent.lifespan += dt;
        activeCount++;
        totalSpeed += agent.phenotype.speed;
        totalSize += agent.phenotype.size;
        totalEfficiency += agent.phenotype.efficiency;

        SensorData data = ScanSurroundings(agent);
        std::vector<float> inputs = {
            data.fruitAngle, data.fruitDist, 
            data.poisonAngle, data.poisonDist,
            data.obstacleAngle, data.obstacleDist
        };

        auto outputs = agent.brain.FeedForward(inputs);

        float leftTrack = outputs[0];
        float rightTrack = outputs[1];
        float rotSpeed = 3.0f;
        float moveSpeed = 120.0f * agent.phenotype.GetActualSpeed();

        agent.angle += (leftTrack - rightTrack) * rotSpeed * dt;
        Vector2 forward = { cos(agent.angle), sin(agent.angle) };
        float throttle = (leftTrack + rightTrack) / 2.0f;
        if(throttle < -0.2f) throttle = -0.2f;

        Vector2 newPos = Vector2Add(agent.pos, Vector2Scale(forward, throttle * moveSpeed * dt));
        
        if (!CheckObstacleCollision(newPos, agent.phenotype.GetVisualSize())) {
            agent.pos = newPos;
        } else {
            agent.obstaclesHit++;
            // Small penalty for hitting obstacles
            if (Config::ENABLE_LIFETIME_LEARNING) {
                agent.brain.LearnFromReward(-0.1f, Config::LEARNING_RATE);
            }
        }

        // Wrap around screen
        if (agent.pos.x < 0) agent.pos.x = Config::SCREEN_W;
        if (agent.pos.x > Config::SCREEN_W) agent.pos.x = 0;
        if (agent.pos.y < 0) agent.pos.y = Config::SCREEN_H;
        if (agent.pos.y > Config::SCREEN_H) agent.pos.y = 0;

        float metabolismRate = Config::METABOLISM_RATE * agent.phenotype.GetMetabolicRate();
        agent.energy -= metabolismRate * dt;
        
        if (agent.energy <= 0) {
            agent.active = false;
            stats.deaths++;
            
            float fitness = agent.CalculateFitness();
            stats.totalFitness += fitness;
            if (fitness > stats.bestFitness) {
                stats.bestFitness = fitness;
            }

            size_t activeAgents = 0;
            for(const auto& a : agents) if(a.active) activeAgents++;

            if (activeAgents <= Config::ACTIVE_AGENTS && fitness > 5.0f) {
                savedGenetics.push_back({agent.brain, agent.phenotype, fitness});
            }
            continue;
        }

        HandleInteractions(agent, babies);
    }
    
    if (activeCount > 0) {
        stats.avgSpeed = totalSpeed / activeCount;
        stats.avgSize = totalSize / activeCount;
        stats.avgEfficiency = totalEfficiency / activeCount;
    }

    if (!babies.empty()) {
        stats.births += babies.size();
        agents.insert(agents.end(), babies.begin(), babies.end());
    }

    CleanupEntities(agents);
    CleanupEntities(fruits);
    CleanupEntities(poisons);

    // Dynamic spawning with safe positions
    if (fruits.size() < 40) {
        Vector2 pos = FindSafeSpawnPosition(5.0f, 20);
        fruits.push_back({pos});
    }
    if (poisons.size() < 15) {
        Vector2 pos = FindSafeSpawnPosition(5.0f, 20);
        poisons.push_back({pos});
    }

    if (agents.empty()) {
        if (stats.deaths > 0) {
            stats.avgFitness = stats.totalFitness / stats.deaths;
        }
        InitPopulation();
        stats.totalFitness = 0.0f;
    }
    if (agents.size() > stats.maxPop) stats.maxPop = agents.size();
}

void World::Draw() {
    // Draw obstacles
    for (const auto& obs : obstacles) {
        if (obs.active) {
            DrawRectangleV(obs.pos, obs.size, {100, 100, 100, 255});
            DrawRectangleLinesEx({obs.pos.x, obs.pos.y, obs.size.x, obs.size.y}, 2, {150, 150, 150, 255});
        }
    }
}