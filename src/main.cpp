#include "raylib.h"
#include "World.hpp"
#include "Config.hpp"
#include "Brain.hpp"
#include "RNNBrain.hpp"
#include "rlImGui.h"
#include "imgui.h"
#include <algorithm>

struct UIState {
    bool paused = false;
    float timeScale = 1.0f;
    bool godMode = false;
    bool showNeuralViz = false;
    bool showAgentStats = false;
    bool showPhenotypePanel = false;
    int selectedAgentIdx = -1;
    
    enum class SpawnTool { None, Fruit, Poison, Agent, AgentRNN, Erase };
    SpawnTool currentTool = SpawnTool::None;
    
    Camera2D camera = {0};
    bool freeCam = false;
};



void DrawPhenotypePanel(UIState& ui, World& world) {
    ImGui::Begin("Phenotype Evolution", &ui.showPhenotypePanel);
    
    ImGui::Text("Population Averages:");
    ImGui::Separator();
    
    ImGui::Text("Average Speed: %.2f", world.stats.avgSpeed);
    ImGui::ProgressBar((world.stats.avgSpeed - 0.5f) / 1.5f);
    
    ImGui::Text("Average Size: %.2f", world.stats.avgSize);
    ImGui::ProgressBar((world.stats.avgSize - 0.7f) / 0.8f);
    
    ImGui::Text("Average Efficiency: %.2f", world.stats.avgEfficiency);
    ImGui::ProgressBar((world.stats.avgEfficiency - 0.7f) / 0.6f);
    
    ImGui::Separator();
    ImGui::Text("Trade-off Parameters:");
    
    ImGui::SliderFloat("Speed->Energy Cost", &Config::SPEED_ENERGY_MULTIPLIER, 1.0f, 3.0f);
    ImGui::SliderFloat("Size->Speed Penalty", &Config::SIZE_SPEED_MULTIPLIER, 0.5f, 1.5f);
    
    ImGui::Separator();
    
    if (ui.selectedAgentIdx >= 0 && ui.selectedAgentIdx < (int)world.agents.size()) {
        Agent& agent = world.agents[ui.selectedAgentIdx];
        if (agent.active) {
            ImGui::Text("Selected Agent Phenotype:");
            ImGui::Text("Speed: %.2f", agent.phenotype.speed);
            ImGui::Text("Size: %.2f", agent.phenotype.size);
            ImGui::Text("Efficiency: %.2f", agent.phenotype.efficiency);
            ImGui::Separator();
            ImGui::Text("Actual Speed: %.2f", agent.phenotype.GetActualSpeed());
            ImGui::Text("Metabolic Rate: %.2f", agent.phenotype.GetMetabolicRate());
            ImGui::Text("Visual Size: %.1f", agent.phenotype.GetVisualSize());
        }
    }
    
    ImGui::End();
}

void DrawGodModePanel(UIState& ui, World& world) {
    ImGui::Begin("God Mode", &ui.godMode);
    
    ImGui::Text("Click on world to spawn/modify");
    ImGui::Separator();
    
    const char* tools[] = { "None", "Spawn Fruit", "Spawn Poison", "Spawn Agent", "Spawn RNN Agent", "Erase" };
    int currentTool = (int)ui.currentTool;
    if (ImGui::Combo("Tool", &currentTool, tools, 6)) {
        ui.currentTool = (UIState::SpawnTool)currentTool;
    }
    
    ImGui::Separator();
    ImGui::Text("🏗️ Obstacle Layouts:");
    
    if (ImGui::Button("Random Mixed", ImVec2(-1, 0))) {
        world.GenerateRandomObstacles();
    }
    
    if (ImGui::Button("Maze", ImVec2(-1, 0))) {
        world.GenerateMaze();
    }
    
    if (ImGui::Button("Arena", ImVec2(-1, 0))) {
        world.GenerateArena();
    }
    
    if (ImGui::Button("Rooms", ImVec2(-1, 0))) {
        world.GenerateRooms();
    }
    
    if (ImGui::Button("Spiral", ImVec2(-1, 0))) {
        world.GenerateSpiral();
    }
    
    if (ImGui::Button("Clear All", ImVec2(-1, 0))) {
        world.ClearObstacles();
    }
    
    ImGui::SliderInt("Obstacle Count", &Config::OBSTACLE_COUNT, 0, 20);
    
    ImGui::Separator();
    ImGui::Text("Quick Actions:");
    
    if (ImGui::Button("Spawn 10 Fruits")) {
        for (int i = 0; i < 10; ++i) {
            Vector2 pos = world.FindSafeSpawnPosition(5.0f);
            world.fruits.push_back({pos});
        }
    }
    
    if (ImGui::Button("Spawn 10 Poisons")) {
        for (int i = 0; i < 10; ++i) {
            Vector2 pos = world.FindSafeSpawnPosition(5.0f);
            world.poisons.push_back({pos});
        }
    }
    
    if (ImGui::Button("Spawn 5 Random Agents")) {
        for (int i = 0; i < 5; ++i) {
            Vector2 pos = world.FindSafeSpawnPosition();
            world.agents.emplace_back(pos);
        }
    }
    
    ImGui::Separator();
    
    if (ImGui::Button("Clear All Fruits")) {
        world.fruits.clear();
    }
    
    if (ImGui::Button("Clear All Poisons")) {
        world.poisons.clear();
    }
    
    if (ImGui::Button("Kill Half Population")) {
        for (size_t i = 0; i < world.agents.size() / 2; ++i) {
            if (world.agents[i].active) {
                world.agents[i].active = false;
                world.stats.deaths++;
            }
        }
    }
    
    ImGui::Separator();
    ImGui::Text("Energy Manipulation:");
    
    if (ImGui::Button("Boost All Agents (+50 Energy)")) {
        for (auto& a : world.agents) {
            if (a.active) a.energy = std::min(a.energy + 50.0f, Config::AGENT_MAX_ENERGY);
        }
    }
    
    if (ImGui::Button("Drain All Agents (-30 Energy)")) {
        for (auto& a : world.agents) {
            if (a.active) a.energy = std::max(a.energy - 30.0f, 1.0f);
        }
    }
    
    ImGui::End();
}

void DrawAgentStatsPanel(UIState& ui, World& world) {
    ImGui::Begin("📊 Agent Statistics", &ui.showAgentStats);
    
    if (world.agents.empty()) {
        ImGui::Text("No active agents");
        ImGui::End();
        return;
    }
    
    ImGui::Text("Select Agent:");
    ImGui::BeginChild("AgentList", ImVec2(0, 200), true);
    
    for (size_t i = 0; i < world.agents.size(); ++i) {
        if (!world.agents[i].active) continue;
        
        char label[128];
        snprintf(label, 128, "Agent #%zu [%s] E:%.0f S:%.1f", i, 
                 world.agents[i].sex == Sex::Male ? "M" : "F",
                 world.agents[i].energy,
                 world.agents[i].phenotype.speed);
        
        if (ImGui::Selectable(label, ui.selectedAgentIdx == (int)i)) {
            ui.selectedAgentIdx = i;
        }
    }
    
    ImGui::EndChild();
    
    if (ui.selectedAgentIdx >= 0 && ui.selectedAgentIdx < (int)world.agents.size()) {
        Agent& agent = world.agents[ui.selectedAgentIdx];
        
        if (!agent.active) {
            ImGui::Text("Selected agent is dead");
            ui.selectedAgentIdx = -1;
        } else {
            ImGui::Separator();
            ImGui::Text("Agent #%d Details:", ui.selectedAgentIdx);
            
            ImGui::Text("Position: (%.1f, %.1f)", agent.pos.x, agent.pos.y);
            ImGui::Text("Energy: %.1f / %.1f", agent.energy, Config::AGENT_MAX_ENERGY);
            ImGui::ProgressBar(agent.energy / Config::AGENT_MAX_ENERGY);
            
            ImGui::Text("Lifespan: %.1f seconds", agent.lifespan);
            ImGui::Text("Children: %d", agent.childrenCount);
            ImGui::Text("Fruits Eaten: %d", agent.fruitsEaten);
            ImGui::Text("Poisons Avoided: %d", agent.poisonsAvoided);
            ImGui::Text("Obstacles Hit: %d", agent.obstaclesHit);
            ImGui::Text("Total Reward: %.2f", agent.totalReward);
            ImGui::Text("Fitness: %.2f", agent.CalculateFitness());
            
            ImGui::Separator();
            
            if (ImGui::Button("Teleport to Agent")) {
                ui.camera.target = agent.pos;
            }
            
            if (ImGui::Button("Kill Agent")) {
                agent.active = false;
                world.stats.deaths++;
                ui.selectedAgentIdx = -1;
            }
            
            if (ImGui::Button("Max Energy")) {
                agent.energy = Config::AGENT_MAX_ENERGY;
            }
            
            if (ImGui::Button("View Neural Network")) {
                ui.showNeuralViz = true;
            }
            
            if (ImGui::Button("View Phenotype")) {
                ui.showPhenotypePanel = true;
            }
        }
    }
    
    ImGui::End();
}

void DrawNeuralVizPanel(UIState& ui, World& world) {
    ImGui::Begin("Neural Network Visualization", &ui.showNeuralViz, ImGuiWindowFlags_AlwaysAutoResize);
    
    if (ui.selectedAgentIdx >= 0 && ui.selectedAgentIdx < (int)world.agents.size()) {
        Agent& agent = world.agents[ui.selectedAgentIdx];
        
        if (agent.active) {
            ImGui::Text("Agent #%d Brain", ui.selectedAgentIdx);
            ImGui::Separator();
            
            ImVec2 vizSize(400, 300);
            ImVec2 cursorPos = ImGui::GetCursorScreenPos();
            
            agent.brain->Draw(cursorPos, vizSize);
            
            ImGui::Dummy(vizSize);
            
            ImGui::Separator();
            ImGui::Text("Network Architecture:");
            ImGui::Text("Type: %s", agent.brain->GetType().c_str());
            ImGui::Text("Input: %d", agent.brain->GetInputSize());
            ImGui::Text("Output: %d", agent.brain->GetOutputSize());
            
            if (Config::ENABLE_LIFETIME_LEARNING) {
                ImGui::Separator();
                ImGui::TextColored(ImVec4(0.2f, 1.0f, 0.2f, 1.0f), "Lifetime Learning: ENABLED");
                ImGui::Text("Learning Rate: %.4f", Config::LEARNING_RATE);
            }
        } else {
            ImGui::Text("Selected agent is no longer active");
        }
    } else {
        ImGui::Text("No agent selected");
        ImGui::Text("Select an agent from the Agent Statistics panel");
    }
    
    ImGui::End();
}

void HandleGodModeInput(UIState& ui, World& world) {
    if (!ui.godMode || ui.currentTool == UIState::SpawnTool::None) return;
    
    Vector2 mouseWorld = GetScreenToWorld2D(GetMousePosition(), ui.camera);
    
    if (IsMouseButtonPressed(MOUSE_LEFT_BUTTON)) {
        switch (ui.currentTool) {
            case UIState::SpawnTool::Fruit:
                world.fruits.push_back({mouseWorld});
                break;
            case UIState::SpawnTool::Poison:
                world.poisons.push_back({mouseWorld});
                break;
            case UIState::SpawnTool::Agent:
                world.agents.emplace_back(mouseWorld);
                break;
            case UIState::SpawnTool::AgentRNN: {
                Agent a(mouseWorld);
                // Replace default NN with RNN
                a.brain = std::make_unique<RNNBrain>(6, 8, 2);
                a.sex = (rand() % 2 == 0) ? Sex::Male : Sex::Female;
                world.agents.push_back(std::move(a));
                break;
            }
            case UIState::SpawnTool::Erase: {
                float eraseRadius = 30.0f;
                for (auto& f : world.fruits) {
                    if (f.active && Vector2Distance(f.pos, mouseWorld) < eraseRadius) {
                        f.active = false;
                    }
                }
                for (auto& p : world.poisons) {
                    if (p.active && Vector2Distance(p.pos, mouseWorld) < eraseRadius) {
                        p.active = false;
                    }
                }
                for (auto& a : world.agents) {
                    if (a.active && Vector2Distance(a.pos, mouseWorld) < eraseRadius) {
                        a.active = false;
                        world.stats.deaths++;
                    }
                }
                break;
            }
            default:
                break;
        }
    }
}

int main() {
    InitWindow(Config::SCREEN_W, Config::SCREEN_H, "MicroCosm - Enhanced Evolution Sim");
    SetTargetFPS(Config::FPS);
    
    rlImGuiSetup(true);
    
    World world;
    UIState ui;
    
    ui.camera.offset = {Config::SCREEN_W / 2.0f, Config::SCREEN_H / 2.0f};
    ui.camera.target = {Config::SCREEN_W / 2.0f, Config::SCREEN_H / 2.0f};
    ui.camera.rotation = 0.0f;
    ui.camera.zoom = 1.0f;

    while (!WindowShouldClose()) {
        float rawDt = GetFrameTime();
        float dt = ui.paused ? 0.0f : rawDt * ui.timeScale;
        
        if (ui.freeCam) {
            if (IsMouseButtonDown(MOUSE_RIGHT_BUTTON)) {
                Vector2 delta = GetMouseDelta();
                delta = Vector2Scale(delta, -1.0f / ui.camera.zoom);
                ui.camera.target = Vector2Add(ui.camera.target, delta);
            }
            
            float wheel = GetMouseWheelMove();
            if (wheel != 0) {
                ui.camera.zoom += wheel * 0.1f;
                ui.camera.zoom = std::clamp(ui.camera.zoom, 0.5f, 3.0f);
            }
        }
        
        world.Update(dt);
        HandleGodModeInput(ui, world);

        BeginDrawing();
        ClearBackground({20, 20, 25, 255});

        BeginMode2D(ui.camera);

        // Draw obstacles first
        world.Draw();

        // Draw entities
        for (const auto& f : world.fruits) {
            if (f.active) DrawCircleV(f.pos, 3.0f, {0, 228, 48, 255});
        }
        for (const auto& p : world.poisons) {
            if (p.active) DrawRectangleV(p.pos, {6,6}, {160, 32, 240, 255});
        }

        for (const auto& a : world.agents) {
            if (!a.active) continue;
            
            Color col = (a.sex == Sex::Male) ? Color{0, 121, 241, 255} : Color{230, 41, 55, 255};
            float alpha = (a.energy / Config::AGENT_MAX_ENERGY); 
            if (alpha < 0.2f) alpha = 0.2f;
            col.a = (unsigned char)(alpha * 255);

            float visualSize = a.phenotype.GetVisualSize();
            DrawCircleV(a.pos, visualSize, col);
            
            Vector2 head = { a.pos.x + cos(a.angle)*(visualSize + 3), 
                            a.pos.y + sin(a.angle)*(visualSize + 3) };
            DrawLineV(a.pos, head, {255,255,255,100});

            if (a.targetFruit.x != -1) DrawLineV(a.pos, a.targetFruit, {0, 255, 0, 30});
            if (a.targetPoison.x != -1) DrawLineV(a.pos, a.targetPoison, {255, 0, 0, 30});
        }
        
        if (ui.selectedAgentIdx >= 0 && ui.selectedAgentIdx < (int)world.agents.size()) {
            Agent& agent = world.agents[ui.selectedAgentIdx];
            if (agent.active) {
                DrawCircleLines(agent.pos.x, agent.pos.y, 15.0f, YELLOW);
            }
        }
        
        if (ui.godMode && ui.currentTool != UIState::SpawnTool::None) {
            Vector2 mouseWorld = GetScreenToWorld2D(GetMousePosition(), ui.camera);
            DrawCircleLines(mouseWorld.x, mouseWorld.y, 20.0f, GOLD);
        }

        EndMode2D();

        rlImGuiBegin();
        
        ImGui::Begin("Control Panel");
        
        ImGui::Text("Simulation Control");
        ImGui::Separator();
        
        if (ImGui::Button(ui.paused ? "▶ Resume" : "⏸ Pause")) {
            ui.paused = !ui.paused;
        }
        ImGui::SameLine();
        if (ImGui::Button("⏭ Step")) {
            world.Update(1.0f / 60.0f);
        }
        ImGui::SameLine();
        if (ImGui::Button("Reset")) {
            world = World();
        }
        
        ImGui::SliderFloat("Speed", &ui.timeScale, 0.1f, 5.0f, "%.1fx");
        
        ImGui::Separator();
        ImGui::Text("View Options");
        
        ImGui::Checkbox("Free Camera", &ui.freeCam);
        if (ImGui::Button("Reset Camera")) {
            ui.camera.target = {Config::SCREEN_W / 2.0f, Config::SCREEN_H / 2.0f};
            ui.camera.zoom = 1.0f;
        }
        
        ImGui::Separator();
        ImGui::Text("Windows");
        
        ImGui::Checkbox("God Mode", &ui.godMode);
        ImGui::Checkbox("Agent Statistics", &ui.showAgentStats);
        ImGui::Checkbox("Neural Network", &ui.showNeuralViz);
        ImGui::Checkbox("Phenotype Evolution", &ui.showPhenotypePanel);
        
        ImGui::End();
        
        ImGui::Begin("Statistics");
        
        ImGui::Text("Generation: %d", world.stats.generation);
        ImGui::Text("Population: %zu", world.agents.size());
        ImGui::Text("Max Population: %d", world.stats.maxPop);
        ImGui::Text("Births: %d", world.stats.births);
        ImGui::Text("Deaths: %d", world.stats.deaths);
        ImGui::Separator();
        ImGui::Text("Avg Fitness: %.2f", world.stats.avgFitness);
        ImGui::Text("Best Fitness: %.2f", world.stats.bestFitness);
        ImGui::Separator();
        ImGui::Text("Fruits: %zu", world.fruits.size());
        ImGui::Text("Poisons: %zu", world.poisons.size());
        ImGui::Text("Obstacles: %zu", world.obstacles.size());
        ImGui::Separator();
        ImGui::Text("FPS: %d", GetFPS());
        ImGui::Text("Simulation Time: %.1fs", world.stats.time);
        
        ImGui::End();
        
        ImGui::Begin("Configuration");
        
        ImGui::Text("Agent Parameters");
        ImGui::SliderFloat("Vision Radius", &Config::AGENT_VISION_RADIUS, 50.0f, 400.0f);
        ImGui::SliderFloat("Max Energy", &Config::AGENT_MAX_ENERGY, 100.0f, 500.0f);
        ImGui::SliderFloat("Metabolism Rate", &Config::METABOLISM_RATE, 5.0f, 30.0f);
        
        ImGui::Separator();
        ImGui::Text("Environment");
        ImGui::SliderFloat("Fruit Energy", &Config::FRUIT_ENERGY, 20.0f, 100.0f);
        ImGui::SliderFloat("Poison Damage", &Config::POISON_DAMAGE, 20.0f, 100.0f);
        ImGui::Checkbox("Obstacles Enabled", &Config::OBSTACLES_ENABLED);
        
        ImGui::Separator();
        ImGui::Text("Learning");
        ImGui::Checkbox("Enable Lifetime Learning", &Config::ENABLE_LIFETIME_LEARNING);
        ImGui::SliderFloat("Learning Rate", &Config::LEARNING_RATE, 0.001f, 0.1f, "%.4f");
        
        ImGui::End();
        
        if (ui.godMode) DrawGodModePanel(ui, world);
        if (ui.showAgentStats) DrawAgentStatsPanel(ui, world);
        if (ui.showNeuralViz) DrawNeuralVizPanel(ui, world);
        if (ui.showPhenotypePanel) DrawPhenotypePanel(ui, world);
        
        rlImGuiEnd();

        EndDrawing();
    }

    rlImGuiShutdown();
    CloseWindow();
    return 0;
}