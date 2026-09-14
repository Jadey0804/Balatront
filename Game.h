#pragma once
#include "GamesEngineeringBase.h"
#include "Gameplay.h"
#include "Enemies.h"
#include "Combat.h"
#include "TileMap.h"
#include "EnemySprites.h"
#include "LevelFlow.h"
#include "HealthPickups.h"

// Owns the framework window/resources and delegates simulation to PlaySession.
class Game {
    friend class SaveGame;
public:
    int run();
private:
    GamesEngineeringBase::Window canvas;
    TileMap tileMap;
    GamesEngineeringBase::Image playerImage;
    PlaySession session;
    EnemyManager enemies;
    EnemySprites enemySprites;
    PlayerProjectilePool playerShots;
    EnemyProjectilePool enemyShots;
    ManualProjectilePool manualShots;
    Combat combat;
    HealthPickupManager healthPickups;
    LevelFlow level;
    float playerShootCooldown = 0;
    float manualShootCooldown = 0;
    Vector2 worldSize;
    Vector2 spawnPosition;
    const char* startupError = "Resource loading failed";
    CameraMode selectedMode = CameraMode::Fixed;
    std::uint32_t nextMapSeed = 1;
    const Vector2 viewport = {float(GameConfig::WindowWidth), float(GameConfig::WindowHeight)};
    bool previousEscape = false;
    bool previousEnter = false;
    bool previousDebug = false;
    bool previousAoe = false;
    bool previousSave = false;
    bool previousLoad = false;
    const char* saveMessage = "F5 SAVE / F9 LOAD";
    float saveMessageTime = 0;
    bool showCollider = false;
    bool running = true;
    float fps = 0.0f;

    bool initialize();
    void startSession();
    void enterSecondLevel();
    void update(float dt);
    void updateCombat(float dt);
    void drawCombat();
    void render();
    void drawPlayer();
    void drawEnemies();
    void drawProjectiles();
    void drawHud();
    void drawResult();
    unsigned int score() const;
    const char* grade() const;
};
