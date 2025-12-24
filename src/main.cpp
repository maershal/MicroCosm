#include "raylib.h"
#include "World.hpp"
#include "Config.hpp"
#include "Brain.hpp"
#include "RNNBrain.hpp"
#include "NEATBrain.hpp"
#include "UISystem.hpp"
#include "rlImGui.h"
#include "imgui.h"
#include "implot.h"
#include <algorithm>

void HandleGodModeInput(UIState& ui, World& world) {
    if (!ui.godMode || ui.currentTool == UIState::SpawnTool::None) return;
    if (ImGui::GetIO().WantCaptureMouse) return;

    Vector2 mouseWorld = GetScreenToWorld2D(GetMousePosition(), ui.camera);
    
    if (IsMouseButtonPressed(MOUSE_LEFT_BUTTON)) {
        switch (ui.currentTool) {
            case UIState::SpawnTool::Fruit: world.fruits.push_back({mouseWorld}); break;
            case UIState::SpawnTool::Poison: world.poisons.push_back({mouseWorld}); break;
            case UIState::SpawnTool::Agent: world.agents.emplace_back(mouseWorld); break;
            case UIState::SpawnTool::AgentRNN: {
                Agent a(mouseWorld);
                a.brain = std::make_unique<RNNBrain>(7, 8, 3);
                world.agents.push_back(std::move(a));
                break;
            }
            case UIState::SpawnTool::AgentNEAT: {
                Agent a(mouseWorld);
                a.brain = std::make_unique<NEATBrain>(7, 3);
                world.agents.push_back(std::move(a));
                break;
            }
            case UIState::SpawnTool::Erase: {
                float eraseRadius = 30.0f;
                for (auto& f : world.fruits) if (f.active && Vector2Distance(f.pos, mouseWorld) < eraseRadius) f.active = false;
                for (auto& p : world.poisons) if (p.active && Vector2Distance(p.pos, mouseWorld) < eraseRadius) p.active = false;
                for (auto& a : world.agents) if (a.active && Vector2Distance(a.pos, mouseWorld) < eraseRadius) { a.active = false; world.stats.deaths++; }
                break;
            }
            default: break;
        }
    }
}

int main() {
    InitWindow(Config::SCREEN_W, Config::SCREEN_H, "MicroCosmSim - Refactored");
    SetTargetFPS(60);
    rlImGuiSetup(true);
    ImPlot::CreateContext();
    
    World world;
    UISystem uiSystem;
    UIState ui;
    
    ui.camera.offset = { Config::SCREEN_W / 2.0f, Config::SCREEN_H / 2.0f };
    ui.camera.target = { Config::SCREEN_W / 2.0f, Config::SCREEN_H / 2.0f };
    ui.camera.zoom = 1.0f;

    while (!WindowShouldClose()) {
        float dt = ui.paused ? 0.0f : GetFrameTime() * ui.timeScale;
        
        if (ui.freeCam) {
            if (IsMouseButtonDown(MOUSE_RIGHT_BUTTON)) {
                ui.camera.target = Vector2Add(ui.camera.target, Vector2Scale(GetMouseDelta(), -1.0f / ui.camera.zoom));
            }
            ui.camera.zoom = std::clamp(ui.camera.zoom + GetMouseWheelMove() * 0.1f, 0.5f, 3.0f);
        }
        
        world.Update(dt);
        HandleGodModeInput(ui, world);

        BeginDrawing();
        ClearBackground({20, 20, 25, 255});
        BeginMode2D(ui.camera);

        world.Draw();

        for (const auto& f : world.fruits) if (f.active) DrawCircleV(f.pos, 3.0f, GREEN);
        for (const auto& p : world.poisons) if (p.active) DrawRectangleV(Vector2Subtract(p.pos, {3,3}), {6,6}, PURPLE);

        for (const auto& a : world.agents) {
            if (!a.active) continue;
            
            Color col = WHITE;
            switch(a.phenotype.species) {
                case Species::Herbivore: col = {100, 255, 100, 255}; break; // Green
                case Species::Scavenger: col = {255, 165, 0, 255}; break;   // Orange
                case Species::Predator:  col = {255, 50, 50, 255}; break;   // Red
            }
            col.a = (unsigned char)(std::max(0.2f, a.energy / Config::AGENT_MAX_ENERGY) * 255);
            
            float visualSize = a.phenotype.GetVisualSize();
            
            // Pheromone Aura
            if(a.pheromoneEmission > 0.1f) {
                Color aura = {200, 100, 255, (unsigned char)(a.pheromoneEmission * 50)};
                DrawCircleV(a.pos, visualSize + 10 * a.pheromoneEmission, aura);
            }
            
            DrawCircleV(a.pos, visualSize, col);
            
            // Sex Indicator
            Color sexCol = (a.sex == Sex::Male) ? BLUE : PINK;
            DrawCircleV(a.pos, visualSize * 0.4f, sexCol);
            
            Vector2 head = { a.pos.x + cos(a.angle)*(visualSize + 3), a.pos.y + sin(a.angle)*(visualSize + 3) };
            DrawLineV(a.pos, head, RAYWHITE);
        }
        
        if (ui.selectedAgentIdx >= 0 && ui.selectedAgentIdx < (int)world.agents.size()) {
            if (world.agents[ui.selectedAgentIdx].active) DrawCircleLines(world.agents[ui.selectedAgentIdx].pos.x, world.agents[ui.selectedAgentIdx].pos.y, 15.0f, YELLOW);
        }
        
        EndMode2D();

        uiSystem.Draw(ui, world);
        EndDrawing();
    }

    ImPlot::DestroyContext();
    rlImGuiShutdown();
    CloseWindow();
    return 0;
}