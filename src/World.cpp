#include "World.hpp"
#include <algorithm>

// --- Spatial Grid Implementation ---
void SpatialGrid::Clear() {
    for(int x=0; x<Config::GRID_W; x++)
        for(int y=0; y<Config::GRID_H; y++) {
            fruitIndices[x][y].clear();
            poisonIndices[x][y].clear();
            agentIndices[x][y].clear();
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

// --- World Implementation ---

World::World() {
    InitPopulation();
}

void World::InitPopulation() {
    agents.clear();
    fruits.clear();
    poisons.clear();
    
    if(!savedGenetics.empty()) {
        // Sort savedGenetics accoring to fitnesss
        std::sort(savedGenetics.begin(), savedGenetics.end(), 
                  [](const GeneticRecord& a, const GeneticRecord& b) {
                      return a.fitness > b.fitness;
                  });
        
        // TOP 30
        if (savedGenetics.size() > 30) {
            savedGenetics.erase(savedGenetics.begin() + 30, savedGenetics.end());
        }
        
        int totalAgents = 80;
        int randomAgents = totalAgents / 10;
        int eliteAgents = totalAgents / 5;
        int weakMutationAgents = totalAgents / 2 - randomAgents;
        int strongMutationAgents = totalAgents - randomAgents - eliteAgents - weakMutationAgents;
        
        Vector2 startPos;
        
        //Elite preservation
        for(int i = 0; i < eliteAgents && i < savedGenetics.size(); i++) {
            startPos = {RandomFloat(50, Config::SCREEN_W-50), RandomFloat(50, Config::SCREEN_H-50)};
            agents.emplace_back(startPos, savedGenetics[i].brain);
        }
        // Weak mutation
        for(int i = 0; i < weakMutationAgents; i++) {
            startPos = {RandomFloat(50, Config::SCREEN_W-50), RandomFloat(50, Config::SCREEN_H-50)};
            int parentIdx = rand() % savedGenetics.size();
            NeuralNetwork childBrain = savedGenetics[parentIdx].brain;
            childBrain.Mutate(0.15f, 0.08f);
            agents.emplace_back(startPos, childBrain);
        }
         //Strong mutation - for exploration
        for(int i = 0; i < strongMutationAgents; i++) {
            startPos = {RandomFloat(50, Config::SCREEN_W-50), RandomFloat(50, Config::SCREEN_H-50)};
            int parentIdx = rand() % savedGenetics.size();
            NeuralNetwork childBrain = savedGenetics[parentIdx].brain;
            childBrain.Mutate(0.3f, 0.25f); 
            agents.emplace_back(startPos, childBrain);
        }
        
        //Random Agents
        for(int i = 0; i < randomAgents; i++) {
            startPos = {RandomFloat(50, Config::SCREEN_W-50), RandomFloat(50, Config::SCREEN_H-50)};
            agents.emplace_back(startPos); 
        }
        
        savedGenetics.clear();
    }
    else {
        // First generation all random
        for(int i=0; i<80; i++) {
            agents.emplace_back(Vector2{RandomFloat(50,Config::SCREEN_W-50), 
                                       RandomFloat(50, Config::SCREEN_H-50)});
        }
    }
    
    for(int i=0; i<60; i++) fruits.push_back({ {RandomFloat(20, Config::SCREEN_W-20), RandomFloat(20, Config::SCREEN_H-20)} });
    for(int i=0; i<20; i++) poisons.push_back({ {RandomFloat(20, Config::SCREEN_W-20), RandomFloat(20, Config::SCREEN_H-20)} });
    
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
    
    if (sawPoison) {
        agent.poisonsAvoided++;
    }
    
    return data;
}

void World::HandleInteractions(Agent& agent, std::vector<Agent>& babies) {
    float eatRadiusSqr = 15.0f * 15.0f; 
    int gx = (int)agent.pos.x / Config::GRID_CELL_SIZE;
    int gy = (int)agent.pos.y / Config::GRID_CELL_SIZE;
    
    bool ateFruit = false;
    bool atePoison = false;

    for(int x = gx-1; x <= gx+1; x++) {
        for(int y = gy-1; y <= gy+1; y++) {
            if (x < 0 || x >= Config::GRID_W || y < 0 || y >= Config::GRID_H) continue;
            
            for (int idx : grid.fruitIndices[x][y]) {
                if (fruits[idx].active && Vector2DistanceSqr(agent.pos, fruits[idx].pos) < eatRadiusSqr) {
                    agent.energy = std::min(agent.energy + Config::FRUIT_ENERGY, Config::AGENT_MAX_ENERGY);
                    fruits[idx].active = false;
                    agent.fruitsEaten++;
                    ateFruit = true;
                }
            }
            for (int idx : grid.poisonIndices[x][y]) {
                if (poisons[idx].active && Vector2DistanceSqr(agent.pos, poisons[idx].pos) < eatRadiusSqr) {
                    agent.energy -= Config::POISON_DAMAGE;
                    poisons[idx].active = false;
                    atePoison = true;
                    agent.poisonsAvoided = std::max(0, agent.poisonsAvoided - 5);
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
                            babies.push_back(child);
                            
                            agent.childrenCount++;
                            partner.childrenCount++;
                            
                            return;
                        }
                    }
                 }
            }
        }
    }
}

void World::Update(float dt) {
    stats.time += dt;

    grid.Clear();
    for(size_t i=0; i<fruits.size(); ++i) if(fruits[i].active) grid.AddFruit(i, fruits[i].pos);
    for(size_t i=0; i<poisons.size(); ++i) if(poisons[i].active) grid.AddPoison(i, poisons[i].pos);
    for(size_t i=0; i<agents.size(); ++i) if(agents[i].active) grid.AddAgent(i, agents[i].pos);

    std::vector<Agent> babies;

    for(auto& agent : agents) {
        if (!agent.active) continue;
        
        agent.lifespan += dt;

        SensorData data = ScanSurroundings(agent);
        std::vector<float> inputs = {
            data.fruitAngle, data.fruitDist, data.poisonAngle, data.poisonDist,
            agent.energy / Config::AGENT_MAX_ENERGY
        };

        auto outputs = agent.brain.FeedForward(inputs);

        float leftTrack = outputs[0];
        float rightTrack = outputs[1];
        float rotSpeed = 3.0f;
        float moveSpeed = 120.0f;

        agent.angle += (leftTrack - rightTrack) * rotSpeed * dt;
        Vector2 forward = { cos(agent.angle), sin(agent.angle) };
        float throttle = (leftTrack + rightTrack) / 2.0f;
        if(throttle < -0.2f) throttle = -0.2f;

        agent.pos = Vector2Add(agent.pos, Vector2Scale(forward, throttle * moveSpeed * dt));

        // Wrap around screen
        if (agent.pos.x < 0) agent.pos.x = Config::SCREEN_W;
        if (agent.pos.x > Config::SCREEN_W) agent.pos.x = 0;
        if (agent.pos.y < 0) agent.pos.y = Config::SCREEN_H;
        if (agent.pos.y > Config::SCREEN_H) agent.pos.y = 0;

        agent.energy -= Config::METABOLISM_RATE * dt;
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

            //Save genetics if population is low and agent has decent fitness
            if (activeAgents <= Config::ACTIVE_AGENTS && fitness > 5.0f) {
                savedGenetics.push_back({agent.brain, fitness});
            }
            continue;
        }

        HandleInteractions(agent, babies);
    }

    if (!babies.empty()) {
        stats.births += babies.size();
        agents.insert(agents.end(), babies.begin(), babies.end());
    }

    CleanupEntities(agents);
    CleanupEntities(fruits);
    CleanupEntities(poisons);

    // Dynamic spawning
    if (fruits.size() < 40) fruits.push_back({ {RandomFloat(0, Config::SCREEN_W), RandomFloat(0, Config::SCREEN_H)} });
    if (poisons.size() < 15) poisons.push_back({ {RandomFloat(0, Config::SCREEN_W), RandomFloat(0, Config::SCREEN_H)} });

    if (agents.empty()) {
        // Calculate fitness before reset
        if (stats.deaths > 0) {
            stats.avgFitness = stats.totalFitness / stats.deaths;
        }
        InitPopulation();
        stats.totalFitness = 0.0f;
    }
    if (agents.size() > stats.maxPop) stats.maxPop = agents.size();
}