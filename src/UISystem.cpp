#include "UISystem.hpp"
#include "rlImGui.h"
#include "imgui.h"
#include "implot.h"
#include "RNNBrain.hpp"
#include "Config.hpp"
#include <cstdio>
#include <algorithm>

void UISystem::Draw(UIState& ui, World& world) {
    rlImGuiBegin();
    
    DrawControlPanel(ui, world);
    DrawStatsPanel(ui, world);
    DrawConfigPanel(ui, world);
    
    if (ui.godMode) DrawGodModePanel(ui, world);
    if (ui.showAgentStats) DrawAgentStatsPanel(ui, world);
    if (ui.showNeuralViz) DrawNeuralVizPanel(ui, world);
    if (ui.showNeuralViz) DrawNeuralVizPanel(ui, world);
    if (ui.showPhenotypePanel) DrawPhenotypePanel(ui, world);
    if (ui.showAnalytics) DrawAnalyticsPanel(ui, world);
    
    rlImGuiEnd();
}

void UISystem::DrawControlPanel(UIState& ui, World& world) {
    ImGui::Begin("Control Panel");
    ImGui::Text("Simulation Control");
    ImGui::Separator();
    
    if (ImGui::Button(ui.paused ? "▶ Resume" : "⏸ Pause")) ui.paused = !ui.paused;
    ImGui::SameLine();
    if (ImGui::Button("⏭ Step")) world.Update(1.0f / 60.0f);
    ImGui::SameLine();
    if (ImGui::Button("Reset")) world = World();
    
    ImGui::SliderFloat("Speed", &ui.timeScale, 0.1f, 5.0f, "%.1fx");
    
    ImGui::Separator();
    ImGui::Text("View Options");
    ImGui::Checkbox("Free Camera", &ui.freeCam);
    if (ImGui::Button("Reset Camera")) {
        ui.camera.target = { (float)Config::SCREEN_W / 2.0f, (float)Config::SCREEN_H / 2.0f };
        ui.camera.zoom = 1.0f;
    }
    
    ImGui::Separator();
    ImGui::Text("Windows");
    ImGui::Checkbox("God Mode", &ui.godMode);
    ImGui::Checkbox("Agent Statistics", &ui.showAgentStats);
    ImGui::Checkbox("Neural Network", &ui.showNeuralViz);
    ImGui::Checkbox("Phenotype Evolution", &ui.showPhenotypePanel);
    ImGui::Checkbox("Analytics", &ui.showAnalytics);
    ImGui::End();
}

void UISystem::DrawStatsPanel(UIState& ui, World& world) {
    (void)ui;
    ImGui::Begin("Global Statistics");
    ImGui::Text("Generation: %d", world.stats.generation);
    ImGui::Text("Population: %zu", world.agents.size());
    ImGui::Text("Births: %d | Deaths: %d", world.stats.births, world.stats.deaths);
    ImGui::Separator();
    ImGui::Text("Avg Fitness: %.2f", world.stats.avgFitness);
    ImGui::Text("Best Fitness: %.2f", world.stats.bestFitness);
    ImGui::Separator();
    ImGui::Text("FPS: %d", GetFPS());
    ImGui::Text("Elapsed: %.1fs", world.stats.time);
    ImGui::End();
}

void UISystem::DrawConfigPanel(UIState& ui, World& world) {
    (void)ui; (void)world;
    ImGui::Begin("Environment Config");
    ImGui::SliderFloat("Vision Radius", &Config::AGENT_VISION_RADIUS, 50.0f, 400.0f);
    ImGui::SliderFloat("Max Energy", &Config::AGENT_MAX_ENERGY, 100.0f, 500.0f);
    ImGui::SliderFloat("Metabolism", &Config::METABOLISM_RATE, 5.0f, 30.0f);
    ImGui::Checkbox("Obstacles", &Config::OBSTACLES_ENABLED);
    ImGui::End();
}

void UISystem::DrawGodModePanel(UIState& ui, World& world) {
    ImGui::Begin("God Mode", &ui.godMode);
    const char* tools[] = { "None", "Fruit", "Poison", "Agent", "Agent RNN", "Agent NEAT", "Erase" };
    int currentTool = (int)ui.currentTool;
    if (ImGui::Combo("Tool", &currentTool, tools, 7)) ui.currentTool = (UIState::SpawnTool)currentTool;
    
    ImGui::Separator();
    if (ImGui::Button("Random Map")) world.GenerateRandomObstacles();
    if (ImGui::Button("Maze Map")) world.GenerateMaze();
    if (ImGui::Button("Arena Map")) world.GenerateArena();
    if (ImGui::Button("Clear Map")) world.ClearObstacles();
    ImGui::End();
}

void UISystem::DrawAgentStatsPanel(UIState& ui, World& world) {
    ImGui::Begin("Agent Stats", &ui.showAgentStats);
    if (world.agents.empty()) { ImGui::Text("Empty..."); ImGui::End(); return; }
    
    ImGui::BeginChild("List", ImVec2(0, 150), true);
    for (int i = 0; i < (int)world.agents.size(); ++i) {
        if (!world.agents[i].active) continue;
        char label[64]; snprintf(label, 64, "Agent #%d (%s)", i, world.agents[i].sex == Sex::Male ? "M" : "F");
        if (ImGui::Selectable(label, ui.selectedAgentIdx == i)) ui.selectedAgentIdx = i;
    }
    ImGui::EndChild();
    
    if (ui.selectedAgentIdx >= 0 && ui.selectedAgentIdx < (int)world.agents.size()) {
        Agent& a = world.agents[ui.selectedAgentIdx];
        if (a.active) {
            ImGui::Text("Energy: %.1f", a.energy);
            ImGui::Text("Fitness: %.2f", a.CalculateFitness());
            if (ImGui::Button("Follow")) {
                ui.camera.target = a.pos;
                ui.camera.zoom = 2.0f;
            }
            if (ImGui::Button("Kill")) a.active = false;
        } else ImGui::Text("Agent is dead");
    }
    ImGui::End();
}

void UISystem::DrawNeuralVizPanel(UIState& ui, World& world) {
    ImGui::Begin("Brain Visualizer", &ui.showNeuralViz);
    if (ui.selectedAgentIdx >= 0 && ui.selectedAgentIdx < (int)world.agents.size()) {
        Agent& a = world.agents[ui.selectedAgentIdx];
        if (a.active) {
            ImVec2 vizSize(400, 300);
            a.brain->Draw(ImGui::GetCursorScreenPos(), vizSize);
            ImGui::Dummy(vizSize);
            ImGui::Text("Type: %s", a.brain->GetType().c_str());
        } else ImGui::Text("Agent is dead");
    } else ImGui::Text("Select an agent first");
    ImGui::End();
}

void UISystem::DrawPhenotypePanel(UIState& ui, World& world) {
    (void)ui;
    ImGui::Begin("Evolution Trends", &ui.showPhenotypePanel);
    ImGui::Text("Average Size: %.2f", world.stats.avgSize);
    ImGui::End();
}

void UISystem::DrawAnalyticsPanel(UIState& ui, World& world) {
    ImGui::Begin("Analytics", &ui.showAnalytics);
    
    if (world.stats.history.empty()) {
        ImGui::Text("No history data yet. Wait for a generation to complete.");
    } else {
        if (ImPlot::BeginPlot("Fitness History")) {
            std::vector<float> gens, avgFit, bestFit;
            for(size_t i=0; i<world.stats.history.size(); ++i) {
                gens.push_back((float)i);
                avgFit.push_back(world.stats.history[i].avgFitness);
                bestFit.push_back(world.stats.history[i].bestFitness);
            }
            
            ImPlot::PlotLine("Avg Fitness", gens.data(), avgFit.data(), (int)gens.size());
            ImPlot::PlotLine("Best Fitness", gens.data(), bestFit.data(), (int)gens.size());
            ImPlot::EndPlot();
        }
        
        if (ImPlot::BeginPlot("Phenotype Trends")) {
            std::vector<float> gens, avgSpeed, avgSize;
            for(size_t i=0; i<world.stats.history.size(); ++i) {
                gens.push_back((float)i);
                avgSpeed.push_back(world.stats.history[i].avgSpeed);
                avgSize.push_back(world.stats.history[i].avgSize);
            }
            
            ImPlot::PlotLine("Avg Speed", gens.data(), avgSpeed.data(), (int)gens.size());
            ImPlot::PlotLine("Avg Size", gens.data(), avgSize.data(), (int)gens.size());
            ImPlot::EndPlot();
        }
    }
    
    ImGui::End();
}
