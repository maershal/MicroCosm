#include "raylib.h"
#include "World.hpp"
#include "Config.hpp"

#include "rlImGui.h"
#include "imgui.h"
int main() {
    InitWindow(Config::SCREEN_W, Config::SCREEN_H, "Mikrokosmos - Evolution Sim");
    SetTargetFPS(Config::FPS);
    
    World world;

    while (!WindowShouldClose()) {
        float dt = GetFrameTime();
        world.Update(dt);

        BeginDrawing();
        ClearBackground({20, 20, 25, 255});

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

        DrawRectangle(0, 0, 280, 160, {0,0,0,180});
        DrawText(TextFormat("Pop: %zi | Gen: %i", world.agents.size(), world.stats.generation), 10, 10, 20, WHITE);
        DrawText(TextFormat("Max Pop: %i", world.stats.maxPop), 10, 35, 18, YELLOW);
        DrawText(TextFormat("Births: %i | Deaths: %i", world.stats.births, world.stats.deaths), 10, 60, 18, GREEN);
        DrawText(TextFormat("Avg Fitness: %.1f", world.stats.avgFitness), 10, 85, 18, SKYBLUE);
        DrawText(TextFormat("Best Fitness: %.1f", world.stats.bestFitness), 10, 110, 18, GOLD);
        DrawText(TextFormat("FPS: %i", GetFPS()), 10, 135, 18, RAYWHITE);

        DrawRectangle(Config::SCREEN_W - 180, 0, 180, 90, {0,0,0,150});
        DrawCircle(Config::SCREEN_W - 165, 15, 5, {0, 121, 241, 255});
        DrawText("Male", Config::SCREEN_W - 150, 10, 16, WHITE);
        DrawCircle(Config::SCREEN_W - 165, 40, 5, {230, 41, 55, 255});
        DrawText("Female", Config::SCREEN_W - 150, 35, 16, WHITE);
        DrawCircle(Config::SCREEN_W - 165, 65, 3, {0, 228, 48, 255});
        DrawText("Fruit", Config::SCREEN_W - 150, 60, 16, WHITE);
        DrawRectangle(Config::SCREEN_W - 168, 80, 6, 6, {160, 32, 240, 255});
        DrawText("Poison", Config::SCREEN_W - 150, 80, 16, WHITE);

        EndDrawing();
    }

    CloseWindow();
    return 0;
}