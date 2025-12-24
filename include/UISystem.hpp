#pragma once
#include "raylib.h"
#include "World.hpp"

struct UIState {
    bool paused = false;
    float timeScale = 1.0f;
    bool godMode = false;
    bool showNeuralViz = false;
    bool showAgentStats = false;
    bool showPhenotypePanel = false;
    bool showAnalytics = false;
    int selectedAgentIdx = -1;
    
    enum class SpawnTool { None, Fruit, Poison, Agent, AgentRNN, AgentNEAT, Erase };
    SpawnTool currentTool = SpawnTool::None;
    
    Camera2D camera = { 0 };
    bool freeCam = false;
};

class UISystem {
public:
    void Draw(UIState& ui, World& world);
private:
    void DrawControlPanel(UIState& ui, World& world);
    void DrawStatsPanel(UIState& ui, World& world);
    void DrawConfigPanel(UIState& ui, World& world);
    void DrawGodModePanel(UIState& ui, World& world);
    void DrawAgentStatsPanel(UIState& ui, World& world);
    void DrawNeuralVizPanel(UIState& ui, World& world);
    void DrawPhenotypePanel(UIState& ui, World& world);
    void DrawAnalyticsPanel(UIState& ui, World& world);
    void DrawSpeciesLegendPanel(UIState& ui, World& world);
};
