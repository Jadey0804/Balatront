#pragma once
#include "GamesEngineeringBase.h"
#include "Gameplay.h"

// Owns the framework window/resources and delegates simulation to PlaySession.
class Game {
public:
    int run();
private:
    GamesEngineeringBase::Window canvas;
    GamesEngineeringBase::Image landscape;
    GamesEngineeringBase::Image playerImage;
    PlaySession session;
    Vector2 worldSize;
    const Vector2 viewport = {float(GameConfig::WindowWidth), float(GameConfig::WindowHeight)};
    bool previousEscape = false;
    bool previousEnter = false;
    bool previousDebug = false;
    bool showCollider = false;
    bool running = true;
    float fps = 0.0f;

    bool initialize();
    void update(float dt);
    void render();
    void drawPlayer();
    void drawHud();
};
