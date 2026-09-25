#pragma once
#include "GamesEngineeringBase.h"
#include "Gameplay.h"
#include "Enemies.h"
#include "Combat.h"
#include "TileMap.h"
#include "EnemySprites.h"
#include "LevelFlow.h"
#include "HealthPickups.h"

// the class keeps the game window and runs all game parts
class Game {
    friend class SaveGame;
public:
    int run();
private:
    GamesEngineeringBase::Window canvas;
    GamesEngineeringBase::XBoxControllers controllers;
    GamesEngineeringBase::XBoxController controller;
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

    bool makeReady();
    void startOneGame();
    void goToNextMap();
    void gameUpdate(float dt);
    void fightUpdate(float dt);
    void paintFight();
    void paintAll();
    void paintHero();
    void paintMonsters();
    void paintBullets();
    void paintInformation();
    void paintFinish();
    unsigned int countPoint() const;
    const char* getLevelLetter() const;
};
