#include "UISystem.hpp"
#include "rlImGui.h"
#include "imgui.h"
#include "implot.h"
#include "RNNBrain.hpp"
#include "Config.hpp"
#include <cstdio>
#include <algorithm>

void ApplyDarkTheme() {
    auto& style = ImGui::GetStyle();
    style.TabRounding = 5.0f;
    style.WindowRounding = 8.0f;
    style.ChildRounding = 5.0f;
    style.FrameRounding = 4.0f;
    style.GrabRounding = 4.0f;
    style.PopupRounding = 5.0f;
    style.ScrollbarRounding = 5.0f;
    style.FrameBorderSize = 1.0f;
    style.WindowBorderSize = 1.0f;
    
    ImVec4* colors = style.Colors;
    colors[ImGuiCol_Text]                   = ImVec4(0.90f, 0.90f, 0.93f, 1.00f);
    colors[ImGuiCol_WindowBg]               = ImVec4(0.10f, 0.10f, 0.12f, 0.95f);
    colors[ImGuiCol_Header]                 = ImVec4(0.15f, 0.15f, 0.18f, 1.00f);
    colors[ImGuiCol_HeaderHovered]          = ImVec4(0.20f, 0.20f, 0.25f, 1.00f);
    colors[ImGuiCol_HeaderActive]           = ImVec4(0.25f, 0.25f, 0.30f, 1.00f);
    colors[ImGuiCol_Button]                 = ImVec4(0.20f, 0.25f, 0.30f, 1.00f);
    colors[ImGuiCol_ButtonHovered]          = ImVec4(0.25f, 0.30f, 0.35f, 1.00f);
    colors[ImGuiCol_ButtonActive]           = ImVec4(0.30f, 0.35f, 0.40f, 1.00f);
    colors[ImGuiCol_FrameBg]                = ImVec4(0.15f, 0.15f, 0.18f, 1.00f);
    colors[ImGuiCol_TitleBg]                = ImVec4(0.12f, 0.12f, 0.14f, 1.00f);
    colors[ImGuiCol_TitleBgActive]          = ImVec4(0.15f, 0.15f, 0.18f, 1.00f);
}

void UISystem::Draw(UIState& ui, World& world) {
    static bool themeApplied = false;
    if(!themeApplied) { ApplyDarkTheme(); themeApplied = true; }

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
    DrawSpeciesLegendPanel(ui, world); // Always show legend or make toggleable? Let's keep it always or in control panel.
    // Let's make it small and unobtrusive.
    
    rlImGuiEnd();
}



void UISystem::DrawControlPanel(UIState& ui, World& world) {
    ImGui::Begin("Control Panel");
    ImGui::Text("Simulation Control");
    ImGui::Separator();
    
    // Size Selection
    const char* sizes[] = { "Small (800x600)", "Medium (1280x720)", "Large (1920x1080)", "Huge (2560x1440)" };
    int currentSize = (int)Config::CURRENT_SIZE;
    if (ImGui::Combo("Sim Size", &currentSize, sizes, 4)) {
        Config::SetWindowSize((Config::SimSize)currentSize);
        world = World(); // Reset world to apply new size and population
    }

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
    
    ImGui::Separator();
    const char* seasonName = world.season.GetName();
    ImVec4 seasonCol = ImVec4(1,1,1,1);
    switch(world.season.currentSeason) {
        case Season::Spring: seasonCol = ImVec4(0.4f, 1.0f, 0.4f, 1.0f); break;
        case Season::Summer: seasonCol = ImVec4(1.0f, 0.9f, 0.2f, 1.0f); break;
        case Season::Autumn: seasonCol = ImVec4(0.8f, 0.5f, 0.2f, 1.0f); break;
        case Season::Winter: seasonCol = ImVec4(0.4f, 0.6f, 1.0f, 1.0f); break;
    }
    ImGui::TextColored(seasonCol, "Season: %s", seasonName);
    ImGui::ProgressBar(world.season.seasonTimer / world.season.seasonDuration, ImVec2(0,0), "Progress");
    
    ImGui::End();
}

void UISystem::DrawConfigPanel(UIState& ui, World& world) {
    (void)ui; (void)world;
    ImGui::Begin("Environment Config");
    ImGui::SliderFloat("Vision Radius", &Config::AGENT_VISION_RADIUS, 50.0f, 400.0f);
    ImGui::SliderFloat("Max Energy", &Config::AGENT_MAX_ENERGY, 100.0f, 500.0f);
    ImGui::SliderFloat("Metabolism", &Config::METABOLISM_RATE, 5.0f, 30.0f);
    ImGui::Checkbox("Obstacles", &Config::OBSTACLES_ENABLED);
    
    ImGui::Separator();
    ImGui::Text("Species Balance");
    ImGui::SliderFloat("Predator Steal", &Config::PREDATOR_STEAL_AMOUNT, 0.0f, 100.0f);
    ImGui::SliderFloat("Herbivore Bonus", &Config::HERBIVORE_FRUIT_BONUS, 1.0f, 3.0f);
    ImGui::SliderFloat("Scavenger Gain", &Config::SCAVENGER_POISON_GAIN, 0.1f, 1.0f);
    ImGui::SliderFloat("Predator Meta", &Config::PREDATOR_METABOLISM_MODIFIER, 0.5f, 2.0f);
    
    ImGui::Separator();
    ImGui::Text("Evolution Control");
    ImGui::SliderFloat("Mutation Rate", &Config::MUTATION_RATE_MULTIPLIER, 0.0f, 5.0f);
    ImGui::SliderFloat("Mating Cost", &Config::MATING_ENERGY_COST, 10.0f, 100.0f);
    ImGui::SliderFloat("Mating Threshold", &Config::MATING_ENERGY_THRESHOLD, 50.0f, 180.0f);
    ImGui::SliderFloat("Mating Dist (Sqr)", &Config::MATING_RANGE, 10.0f, 100.0f);
    ImGui::SliderFloat("Eat Radius", &Config::EAT_RADIUS, 5.0f, 50.0f);
    
    ImGui::Separator();
    ImGui::Text("Mutation Control");
    ImGui::SliderFloat("Brain Mut Rate", &Config::CHILD_BRAIN_MUTATION_RATE, 0.0f, 1.0f);
    ImGui::SliderFloat("Brain Mut Power", &Config::CHILD_BRAIN_MUTATION_POWER, 0.0f, 1.0f);
    ImGui::SliderFloat("Pheno Mut Rate", &Config::CHILD_PHENOTYPE_MUTATION_RATE, 0.0f, 1.0f);
    
    ImGui::Separator();
    ImGui::Text("Season Control");
    ImGui::SliderFloat("Duration", &Config::SEASON_DURATION, 10.0f, 120.0f);

    ImGui::End();
}

void UISystem::DrawGodModePanel(UIState& ui, World& world) {
    ImGui::Begin("God Mode", &ui.godMode);
    const char* tools[] = { "None", "Fruit", "Poison", "Agent", "Agent RNN", "Agent NEAT", "Erase" };
    int currentTool = (int)ui.currentTool;
    if (ImGui::Combo("Tool", &currentTool, tools, 7)) ui.currentTool = (UIState::SpawnTool)currentTool;
    
    ImGui::Separator();
    ImGui::Text("Spawning");
    if (ImGui::Button("Spawn 10 Fruits")) {
        for(int i=0; i<10; i++) world.fruits.push_back({world.FindSafeSpawnPosition(5.0f, 30)});
    }
    ImGui::SameLine();
    if (ImGui::Button("Spawn 10 Poisons")) {
        for(int i=0; i<10; i++) world.poisons.push_back({world.FindSafeSpawnPosition(5.0f, 30)});
    }
    
    if (ImGui::Button("+5 Herbivores")) world.SpawnSpecies(Species::Herbivore, 5);
    ImGui::SameLine();
    if (ImGui::Button("+5 Scavengers")) world.SpawnSpecies(Species::Scavenger, 5);
    ImGui::SameLine();
    if (ImGui::Button("+5 Predators")) world.SpawnSpecies(Species::Predator, 5);
    
    ImGui::Separator();
    ImGui::Text("Global Powers");
    if (ImGui::Button("Start Next Season")) {
        world.season.seasonTimer = world.season.seasonDuration + 1.0f; // Force switch
    }
    
    ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.6f, 0.1f, 0.1f, 1.0f));
    ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.8f, 0.2f, 0.2f, 1.0f));
    ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4(0.4f, 0.0f, 0.0f, 1.0f));
    if (ImGui::Button("THANOS SNAP (Kill 50%)")) world.ThanosSnap();
    ImGui::PopStyleColor(3);
    
    ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.2f, 0.6f, 0.2f, 1.0f));
    if (ImGui::Button("FERTILITY RAY (Max Energy)")) world.FertilityBlessing();
    ImGui::PopStyleColor(1);
    
    ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.6f, 0.2f, 0.8f, 1.0f));
    if (ImGui::Button("BRAIN SCRAMBLE (Mutate All)")) world.ForceMutation();
    ImGui::PopStyleColor(1);
    
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

void UISystem::DrawSpeciesLegendPanel(UIState& ui, World& world) {
    (void)ui; (void)world;
    ImGui::Begin("Species Legend", nullptr, ImGuiWindowFlags_AlwaysAutoResize);
    
    auto DrawLegendItem = [](const char* name, ImVec4 col, const char* desc) {
        ImGui::ColorButton(name, col, ImGuiColorEditFlags_NoTooltip | ImGuiColorEditFlags_NoPicker, ImVec2(20,20));
        ImGui::SameLine();
        ImGui::TextColored(col, "%s", name);
        ImGui::SameLine();
        ImGui::TextDisabled("(?)");
        if(ImGui::IsItemHovered()) ImGui::SetTooltip("%s", desc);
    };
    
    DrawLegendItem("Herbivore", ImVec4(0.4f, 1.0f, 0.4f, 1.0f), "Bonus energy from Fruit, sensitive to Poison.");
    DrawLegendItem("Scavenger", ImVec4(1.0f, 0.64f, 0.0f, 1.0f), "Can consume Poison for energy.");
    DrawLegendItem("Predator",  ImVec4(1.0f, 0.2f, 0.2f, 1.0f), "Hunts other agents. Less energy from Fruit.");
    
    ImGui::Separator();
    ImGui::Text("Pheromone: Purple Aura");
    ImGui::Text("Male: Blue Dot | Female: Pink Dot");
    
    ImGui::End();
}

// ApplyDarkTheme moved to top


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
            
            ImPlot::PlotLine("Avg Size", gens.data(), avgSize.data(), (int)gens.size());
            ImPlot::EndPlot();
        }
        
        if (ImPlot::BeginPlot("Species Population")) {
            std::vector<float> gens, hCount, sCount, pCount;
            for(size_t i=0; i<world.stats.history.size(); ++i) {
                gens.push_back((float)i);
                hCount.push_back((float)world.stats.history[i].herbivoreCount);
                sCount.push_back((float)world.stats.history[i].scavengerCount);
                pCount.push_back((float)world.stats.history[i].predatorCount);
            }
            
            ImPlot::PlotLine("Herbivores", gens.data(), hCount.data(), (int)gens.size());
            ImPlot::PlotLine("Scavengers", gens.data(), sCount.data(), (int)gens.size());
            ImPlot::PlotLine("Predators", gens.data(), pCount.data(), (int)gens.size());
            ImPlot::EndPlot();
        }
        
        if (ImPlot::BeginPlot("Brain Demographics")) {
            std::vector<float> gens, rnn, neat, nn;
            for(size_t i=0; i<world.stats.history.size(); ++i) {
                gens.push_back((float)i);
                rnn.push_back((float)world.stats.history[i].countRNN);
                neat.push_back((float)world.stats.history[i].countNEAT);
                nn.push_back((float)world.stats.history[i].countNN);
            }
            
            ImPlot::PlotLine("RNN", gens.data(), rnn.data(), (int)gens.size());
            ImPlot::PlotLine("NEAT", gens.data(), neat.data(), (int)gens.size());
            ImPlot::PlotLine("FeedForward", gens.data(), nn.data(), (int)gens.size());
            ImPlot::EndPlot();
        }
    }
    
    ImGui::End();
}
