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

void World::GenerateRandomObstacles() {
    obstacles.clear();
    for (int i = 0; i < Config::OBSTACLE_COUNT; ++i) {
        Vector2 pos = {RandomFloat(100, Config::SCREEN_W - 200), 
                      RandomFloat(100, Config::SCREEN_H - 200)};
        Vector2 size = {RandomFloat(50, 150), RandomFloat(50, 150)};
        obstacles.push_back({pos, size, true});
    }
}

void World::GenerateMaze() {
    obstacles.clear();
    
    int wallThickness = 20;
    
    for (int i = 1; i < 4; ++i) {
        float y = Config::SCREEN_H * i / 4.0f;
        obstacles.push_back({
            {100, y - wallThickness/2}, 
            {Config::SCREEN_W - 200, wallThickness}, 
            true
        });
        
        // Add gaps
        obstacles.back().size.x *= 0.4f;
        
        obstacles.push_back({
            {Config::SCREEN_W - 100 - Config::SCREEN_W * 0.4f, y - wallThickness/2}, 
            {Config::SCREEN_W * 0.4f, wallThickness}, 
            true
        });
    }
    
    // Vertical walls
    for (int i = 1; i < 4; ++i) {
        float x = Config::SCREEN_W * i / 4.0f;
        obstacles.push_back({
            {x - wallThickness/2, 100}, 
            {wallThickness, Config::SCREEN_H - 200}, 
            true
        });
        
        // Add gaps
        obstacles.back().size.y *= 0.4f;
        
        obstacles.push_back({
            {x - wallThickness/2, Config::SCREEN_H - 100 - (Config::SCREEN_H - 200) * 0.4f}, 
            {wallThickness, (Config::SCREEN_H - 200) * 0.4f}, 
            true
        });
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
        
        Vector2 startPos;
        
        // Elite preservation
        for(int i = 0; i < eliteAgents && i < savedGenetics.size(); i++) {
            startPos = {RandomFloat(50, Config::SCREEN_W-50), RandomFloat(50, Config::SCREEN_H-50)};
            while (CheckObstacleCollision(startPos, 10.0f)) {
                startPos = {RandomFloat(50, Config::SCREEN_W-50), RandomFloat(50, Config::SCREEN_H-50)};
            }
            agents.emplace_back(startPos, savedGenetics[i].brain, savedGenetics[i].phenotype);
        }
        
        // Weak mutation
        for(int i = 0; i < weakMutationAgents; i++) {
            startPos = {RandomFloat(50, Config::SCREEN_W-50), RandomFloat(50, Config::SCREEN_H-50)};
            while (CheckObstacleCollision(startPos, 10.0f)) {
                startPos = {RandomFloat(50, Config::SCREEN_W-50), RandomFloat(50, Config::SCREEN_H-50)};
            }
            int parentIdx = rand() % savedGenetics.size();
            NeuralNetwork childBrain = savedGenetics[parentIdx].brain;
            childBrain.Mutate(0.15f, 0.08f);
            Phenotype childPheno = savedGenetics[parentIdx].phenotype;
            childPheno.Mutate(0.1f);
            agents.emplace_back(startPos, childBrain, childPheno);
        }
        
        // Strong mutation
        for(int i = 0; i < strongMutationAgents; i++) {
            startPos = {RandomFloat(50, Config::SCREEN_W-50), RandomFloat(50, Config::SCREEN_H-50)};
            while (CheckObstacleCollision(startPos, 10.0f)) {
                startPos = {RandomFloat(50, Config::SCREEN_W-50), RandomFloat(50, Config::SCREEN_H-50)};
            }
            int parentIdx = rand() % savedGenetics.size();
            NeuralNetwork childBrain = savedGenetics[parentIdx].brain;
            childBrain.Mutate(0.3f, 0.25f);
            Phenotype childPheno = savedGenetics[parentIdx].phenotype;
            childPheno.Mutate(0.3f);
            agents.emplace_back(startPos, childBrain, childPheno);
        }
        
        // Random agents
        for(int i = 0; i < randomAgents; i++) {
            startPos = {RandomFloat(50, Config::SCREEN_W-50), RandomFloat(50, Config::SCREEN_H-50)};
            while (CheckObstacleCollision(startPos, 10.0f)) {
                startPos = {RandomFloat(50, Config::SCREEN_W-50), RandomFloat(50, Config::SCREEN_H-50)};
            }
            agents.emplace_back(startPos);
        }
        
        savedGenetics.clear();
    }
    else {
        // First generation
        for(int i=0; i<80; i++) {
            Vector2 startPos = {RandomFloat(50,Config::SCREEN_W-50), RandomFloat(50, Config::SCREEN_H-50)};
            while (CheckObstacleCollision(startPos, 10.0f)) {
                startPos = {RandomFloat(50,Config::SCREEN_W-50), RandomFloat(50, Config::SCREEN_H-50)};
            }
            agents.emplace_back(startPos);
        }
    }
    
    for(int i=0; i<60; i++) {
        Vector2 pos = {RandomFloat(20, Config::SCREEN_W-20), RandomFloat(20, Config::SCREEN_H-20)};
        while (CheckObstacleCollision(pos, 5.0f)) {
            pos = {RandomFloat(20, Config::SCREEN_W-20), RandomFloat(20, Config::SCREEN_H-20)};
        }
        fruits.push_back({pos});
    }
    
    for(int i=0; i<20; i++) {
        Vector2 pos = {RandomFloat(20, Config::SCREEN_W-20), RandomFloat(20, Config::SCREEN_H-20)};
        while (CheckObstacleCollision(pos, 5.0f)) {
            pos = {RandomFloat(20, Config::SCREEN_W-20), RandomFloat(20, Config::SCREEN_H-20)};
        }
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
                            
                            Agent child(agent.pos);
                            child.brain = NeuralNetwork::Crossover(agent.brain, partner.brain);
                            child.brain.Mutate(0.1f, 0.15f);
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

    // Dynamic spawning
    if (fruits.size() < 40) {
        Vector2 pos = {RandomFloat(0, Config::SCREEN_W), RandomFloat(0, Config::SCREEN_H)};
        if (!CheckObstacleCollision(pos, 5.0f)) {
            fruits.push_back({pos});
        }
    }
    if (poisons.size() < 15) {
        Vector2 pos = {RandomFloat(0, Config::SCREEN_W), RandomFloat(0, Config::SCREEN_H)};
        if (!CheckObstacleCollision(pos, 5.0f)) {
            poisons.push_back({pos});
        }
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