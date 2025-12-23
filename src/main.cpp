#include "raylib.h"
#include "World.hpp"
#include "Config.hpp"
#include "rlImGui.h"
#include "imgui.h"
#include <algorithm>

struct UIState {
    bool paused = false;
    float timeScale = 1.0f;
    bool godMode = false;
    bool showNeuralViz = false;
    bool showAgentStats = false;
    bool showHeatmap = false;
    int selectedAgentIdx = -1;
    
    // God Mode tools
    enum class SpawnTool { None, Fruit, Poison, Agent, Erase };
    SpawnTool currentTool = SpawnTool::None;
    
    // Initial conditions
    struct InitialConditions {
        int startingAgents = 80;
        int startingFruits = 60;
        int startingPoisons = 20;
        float mutationRate = 0.15f;
        float mutationStrength = 0.08f;
    } initConditions;
    
    // Camera
    Camera2D camera = {0};
    bool freeCam = false;
};

void DrawNeuralNetwork(const NeuralNetwork& brain, ImVec2 pos, ImVec2 size) {
    ImDrawList* draw = ImGui::GetWindowDrawList();
    
    float nodeRadius = 8.0f;
    float layerSpacing = size.x / 3.0f;
    
    // Input layer
    int inputCount = brain.inputSize;
    float inputSpacing = size.y / (inputCount + 1);
    std::vector<ImVec2> inputNodes;
    for (int i = 0; i < inputCount; ++i) {
        ImVec2 nodePos(pos.x, pos.y + inputSpacing * (i + 1));
        inputNodes.push_back(nodePos);
        draw->AddCircleFilled(nodePos, nodeRadius, IM_COL32(100, 200, 255, 200));
    }
    
    // Hidden layer
    int hiddenCount = brain.hiddenSize;
    float hiddenSpacing = size.y / (hiddenCount + 1);
    std::vector<ImVec2> hiddenNodes;
    for (int i = 0; i < hiddenCount; ++i) {
        ImVec2 nodePos(pos.x + layerSpacing, pos.y + hiddenSpacing * (i + 1));
        hiddenNodes.push_back(nodePos);
        draw->AddCircleFilled(nodePos, nodeRadius, IM_COL32(255, 200, 100, 200));
    }
    
    // Output layer
    int outputCount = brain.outputSize;
    float outputSpacing = size.y / (outputCount + 1);
    std::vector<ImVec2> outputNodes;
    for (int i = 0; i < outputCount; ++i) {
        ImVec2 nodePos(pos.x + layerSpacing * 2, pos.y + outputSpacing * (i + 1));
        outputNodes.push_back(nodePos);
        draw->AddCircleFilled(nodePos, nodeRadius, IM_COL32(100, 255, 150, 200));
    }
    
    // Draw connections (input -> hidden)
    int wIdx = 0;
    for (int h = 0; h < hiddenCount; ++h) {
        for (int i = 0; i < inputCount; ++i) {
            float w = brain.weights[wIdx++];
            ImU32 color = w > 0 ? IM_COL32(100, 255, 100, 100) : IM_COL32(255, 100, 100, 100);
            float thickness = std::abs(w) * 2.0f;
            draw->AddLine(inputNodes[i], hiddenNodes[h], color, thickness);
        }
    }
    
    // Draw connections (hidden -> output)
    for (int o = 0; o < outputCount; ++o) {
        for (int h = 0; h < hiddenCount; ++h) {
            float w = brain.weights[wIdx++];
            ImU32 color = w > 0 ? IM_COL32(100, 255, 100, 100) : IM_COL32(255, 100, 100, 100);
            float thickness = std::abs(w) * 2.0f;
            draw->AddLine(hiddenNodes[h], outputNodes[o], color, thickness);
        }
    }
}

void DrawGodModePanel(UIState& ui, World& world) {
    ImGui::Begin("🌟 God Mode", &ui.godMode);
    
    ImGui::Text("Click on world to spawn/modify");
    ImGui::Separator();
    
    const char* tools[] = { "None", "Spawn Fruit", "Spawn Poison", "Spawn Agent", "Erase" };
    int currentTool = (int)ui.currentTool;
    if (ImGui::Combo("Tool", &currentTool, tools, 5)) {
        ui.currentTool = (UIState::SpawnTool)currentTool;
    }
    
    ImGui::Separator();
    ImGui::Text("Quick Actions:");
    
    if (ImGui::Button("Spawn 10 Fruits")) {
        for (int i = 0; i < 10; ++i) {
            world.fruits.push_back({{RandomFloat(0, Config::SCREEN_W), RandomFloat(0, Config::SCREEN_H)}});
        }
    }
    
    if (ImGui::Button("Spawn 10 Poisons")) {
        for (int i = 0; i < 10; ++i) {
            world.poisons.push_back({{RandomFloat(0, Config::SCREEN_W), RandomFloat(0, Config::SCREEN_H)}});
        }
    }
    
    if (ImGui::Button("Spawn 5 Random Agents")) {
        for (int i = 0; i < 5; ++i) {
            world.agents.emplace_back(Vector2{RandomFloat(50, Config::SCREEN_W-50), RandomFloat(50, Config::SCREEN_H-50)});
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
    
    // Agent list
    ImGui::Text("Select Agent:");
    ImGui::BeginChild("AgentList", ImVec2(0, 200), true);
    
    for (size_t i = 0; i < world.agents.size(); ++i) {
        if (!world.agents[i].active) continue;
        
        char label[64];
        snprintf(label, 64, "Agent #%zu [%s] E:%.0f", i, 
                 world.agents[i].sex == Sex::Male ? "M" : "F",
                 world.agents[i].energy);
        
        if (ImGui::Selectable(label, ui.selectedAgentIdx == (int)i)) {
            ui.selectedAgentIdx = i;
        }
    }
    
    ImGui::EndChild();
    
    // Selected agent details
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
        }
    }
    
    ImGui::End();
}

void DrawNeuralVizPanel(UIState& ui, World& world) {
    ImGui::Begin("🧠 Neural Network Visualization", &ui.showNeuralViz, ImGuiWindowFlags_AlwaysAutoResize);
    
    if (ui.selectedAgentIdx >= 0 && ui.selectedAgentIdx < (int)world.agents.size()) {
        Agent& agent = world.agents[ui.selectedAgentIdx];
        
        if (agent.active) {
            ImGui::Text("Agent #%d Brain", ui.selectedAgentIdx);
            ImGui::Separator();
            
            ImVec2 vizSize(400, 300);
            ImVec2 cursorPos = ImGui::GetCursorScreenPos();
            
            DrawNeuralNetwork(agent.brain, cursorPos, vizSize);
            
            ImGui::Dummy(vizSize);
            
            ImGui::Separator();
            ImGui::Text("Network Architecture:");
            ImGui::Text("Input: %d neurons", agent.brain.inputSize);
            ImGui::Text("Hidden: %d neurons", agent.brain.hiddenSize);
            ImGui::Text("Output: %d neurons", agent.brain.outputSize);
            ImGui::Text("Total weights: %zu", agent.brain.weights.size());
            ImGui::Text("Total biases: %zu", agent.brain.biases.size());
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
            case UIState::SpawnTool::Erase: {
                // Erase nearby entities
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
    InitWindow(Config::SCREEN_W, Config::SCREEN_H, "Mikrokosmos - Evolution Sim");
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
        
        // Camera controls
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

            DrawCircleV(a.pos, 5.0f, col);
            
            Vector2 head = { a.pos.x + cos(a.angle)*8, a.pos.y + sin(a.angle)*8 };
            DrawLineV(a.pos, head, {255,255,255,100});

            if (a.targetFruit.x != -1) DrawLineV(a.pos, a.targetFruit, {0, 255, 0, 30});
            if (a.targetPoison.x != -1) DrawLineV(a.pos, a.targetPoison, {255, 0, 0, 30});
        }
        
        // Highlight selected agent
        if (ui.selectedAgentIdx >= 0 && ui.selectedAgentIdx < (int)world.agents.size()) {
            Agent& agent = world.agents[ui.selectedAgentIdx];
            if (agent.active) {
                DrawCircleLines(agent.pos.x, agent.pos.y, 15.0f, YELLOW);
            }
        }
        
        // Draw god mode cursor
        if (ui.godMode && ui.currentTool != UIState::SpawnTool::None) {
            Vector2 mouseWorld = GetScreenToWorld2D(GetMousePosition(), ui.camera);
            DrawCircleLines(mouseWorld.x, mouseWorld.y, 20.0f, GOLD);
        }

        EndMode2D();

        // ImGui Interface
        rlImGuiBegin();
        
        // Main Control Panel
        ImGui::Begin("🎮 Control Panel");
        
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
        if (ImGui::Button("🔄 Reset")) {
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
        
        ImGui::End();
        
        // Statistics Panel
        ImGui::Begin("📈 Statistics");
        
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
        ImGui::Separator();
        ImGui::Text("FPS: %d", GetFPS());
        ImGui::Text("Simulation Time: %.1fs", world.stats.time);
        
        ImGui::End();
        
        // Configuration Panel
        ImGui::Begin("⚙️ Configuration");
        
        ImGui::Text("Agent Parameters");
        ImGui::SliderFloat("Vision Radius", &Config::AGENT_VISION_RADIUS, 50.0f, 400.0f);
        ImGui::SliderFloat("Max Energy", &Config::AGENT_MAX_ENERGY, 100.0f, 500.0f);
        ImGui::SliderFloat("Metabolism Rate", &Config::METABOLISM_RATE, 5.0f, 30.0f);
        
        ImGui::Separator();
        ImGui::Text("Environment");
        ImGui::SliderFloat("Fruit Energy", &Config::FRUIT_ENERGY, 20.0f, 100.0f);
        ImGui::SliderFloat("Poison Damage", &Config::POISON_DAMAGE, 20.0f, 100.0f);
        
        ImGui::End();
        
        // Conditional panels
        if (ui.godMode) DrawGodModePanel(ui, world);
        if (ui.showAgentStats) DrawAgentStatsPanel(ui, world);
        if (ui.showNeuralViz) DrawNeuralVizPanel(ui, world);
        
        rlImGuiEnd();

        EndDrawing();
    }

    rlImGuiShutdown();
    CloseWindow();
    return 0;
}