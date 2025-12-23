#include "raylib.h"
#include "World.hpp"
#include "Config.hpp"

int main() {
    InitWindow(Config::SCREEN_W, Config::SCREEN_H, "Mikrokosmos - Split Files");
    SetTargetFPS(Config::FPS);

    World world;

    while (!WindowShouldClose()) {
        float dt = GetFrameTime();
        world.Update(dt);

        BeginDrawing();
        ClearBackground({20, 20, 25, 255});

        // Rysowanie (Logika rysowania jest prosta, więc może zostać tutaj, 
        // ale można ją też przenieść do World::Draw)
        
        for (const auto& f : world.fruits) DrawCircleV(f.pos, 3.0f, {0, 228, 48, 255});
        for (const auto& p : world.poisons) DrawRectangleV(p.pos, {6,6}, {160, 32, 240, 255});

        for (const auto& a : world.agents) {
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

        DrawRectangle(0, 0, 250, 120, {0,0,0,150});
        DrawText(TextFormat("Pop: %zi", world.agents.size()), 10, 10, 20, WHITE);
        DrawText(TextFormat("Gen: %i | Max: %i", world.stats.generation, world.stats.maxPop), 10, 35, 20, YELLOW);
        DrawText(TextFormat("Births: %i | Deaths: %i", world.stats.births, world.stats.deaths), 10, 60, 20, GREEN);
        DrawText(TextFormat("FPS: %i", GetFPS()), 10, 85, 20, RAYWHITE);

        EndDrawing();
    }

    CloseWindow();
    return 0;
}
