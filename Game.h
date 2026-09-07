#pragma once
#include "GamesEngineeringBase.h"
#include "Gameplay.h"
#include "Enemies.h"
#include "Combat.h"

// Owns the framework window/resources and delegates simulation to PlaySession.
class Game {
public:
    int run();
private:
    GamesEngineeringBase::Window canvas;
    GamesEngineeringBase::Image landscape;
    GamesEngineeringBase::Image playerImage;
    PlaySession session;
    EnemyManager enemies;
    PlayerProjectilePool playerShots;
    EnemyProjectilePool enemyShots;
    Combat combat;
    float playerShootCooldown = 0;
    Vector2 worldSize;
    const Vector2 viewport = {float(GameConfig::WindowWidth), float(GameConfig::WindowHeight)};
    bool previousEscape = false;
    bool previousEnter = false;
    bool previousDebug = false;
    bool previousShoot = false;
    bool previousAoe = false;
    bool showCollider = false;
    bool running = true;
    float fps = 0.0f;

    bool initialize();
    void update(float dt);
    void updateCombat(float dt);
    void drawCombat();
    void render();
    void drawPlayer();
    void drawEnemies();
    void drawProjectiles();
    void drawHud();
};
