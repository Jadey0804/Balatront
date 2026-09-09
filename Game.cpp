#include "Game.h"
#include "Hud.h"
#include <cstdio>
#include <fstream>
#include <iostream>
#include <ctime>
#include <chrono>

namespace {
    template<unsigned int Capacity>
    void drawShotPool(GamesEngineeringBase::Window& canvas, const Camera& camera,
        const ProjectilePool<Capacity>& pool, unsigned char red, unsigned char green, unsigned char blue) {
        for (unsigned int i = 0; i < Capacity; ++i) {
            const Projectile& shot = pool.at(i);
            if (!shot.active) continue;
            const Vector2 screen = camera.worldToScreen(shot.position);
            const int radius = int(shot.radius), centreX = int(screen.x), centreY = int(screen.y);
            if (centreX + radius < 0 || centreY + radius < 0
                || centreX - radius >= int(canvas.getWidth()) || centreY - radius >= int(canvas.getHeight())) continue;
            for (int y = -radius; y <= radius; ++y)
                for (int x = -radius; x <= radius; ++x) {
                    const int px = centreX + x, py = centreY + y;
                    if (x*x + y*y <= radius*radius && px >= 0 && py >= 0
                        && px < int(canvas.getWidth()) && py < int(canvas.getHeight()))
                        canvas.draw(px, py, red, green, blue);
                }
        }
    }

    bool loadImage(GamesEngineeringBase::Image& image, const char* path) {
        // The immutable course loader does not safely handle missing files.
        std::ifstream file(path, std::ios::binary);
        if (!file) { std::cerr << "Missing resource: " << path << '\n'; return false; }
        file.close();
        if (!image.load(path) || image.width == 0 || image.height == 0) {
            std::cerr << "Unsupported image: " << path << '\n'; return false;
        }
        return true;
    }
}

bool Game::initialize() {
    nextMapSeed = static_cast<std::uint32_t>(std::chrono::high_resolution_clock::now().time_since_epoch().count());
    canvas.create(GameConfig::WindowWidth, GameConfig::WindowHeight, "Balatront");
    if (!loadImage(playerImage, "Resources/L.png")) return false;
    if (!tileMap.load("Resources/tiles.txt")) { startupError = tileMap.error(); return false; }
    worldSize = tileMap.size();
    if (worldSize.x < viewport.x || worldSize.y < viewport.y) {
        startupError = "Map must cover the viewport";
        std::cerr << startupError << '\n'; return false;
    }
    if (!tileMap.findSpawn(session.player.radius, spawnPosition)) { startupError = tileMap.error(); return false; }
    session.camera.follow(spawnPosition, worldSize, viewport);
    return true;
}

int Game::run() {
    if (!initialize()) {
        canvas.clear();
        Hud::text(canvas, 16, 16, "MAP OR RESOURCE ERROR - ESC TO QUIT");
        char message[128];
        std::snprintf(message, sizeof(message), "%s", startupError);
        for (char* c = message; *c; ++c) if (*c >= 'a' && *c <= 'z') *c -= 'a' - 'A';
        Hud::text(canvas, 16, 48, message);
        do { canvas.checkInput(); canvas.present(); } while (!canvas.keyPressed(VK_ESCAPE));
        return 1;
    }
    GamesEngineeringBase::Timer timer;
    double measuredSeconds = 0.0;
    unsigned int frames = 0;
    while (running) {
        canvas.checkInput();
        const float dt = timer.dt();
        update(dt);
        if (!running) break;
        render();
        canvas.present();
        measuredSeconds += dt;
        ++frames;
        if (measuredSeconds >= 2.0) {
            fps = float(frames / measuredSeconds);
            std::cout << "FPS " << fps << " | position " << session.player.position.x << ','
                << session.player.position.y << " | time " << session.elapsed
                << " | state " << int(session.state) << " | enemies " << enemies.activeCount()
                << " | inside " << enemies.insideCount << " | outside " << enemies.outsideCount
                << " | interval " << EnemyManager::spawnInterval(session.elapsed) << std::endl;
            measuredSeconds = 0.0;
            frames = 0;
        }
    }
    return 0;
}

void Game::startSession() {
    tileMap.mode = selectedMode;
    session.camera.mode = selectedMode;
    if (selectedMode == CameraMode::Infinite) {
        nextMapSeed = nextMapSeed * 1664525u + 1013904223u;
        tileMap.procedural.seed = nextMapSeed;
    }
    if (!tileMap.findSpawn(session.player.radius, spawnPosition)) { running = false; return; }
    session.start(worldSize, viewport);
    session.player.position = session.player.previousPosition = spawnPosition;
    session.camera.follow(spawnPosition, worldSize, viewport);
    enemies.reset(nextMapSeed);
    playerShots.reset();
    enemyShots.reset();
    playerShootCooldown = 0;
    combat = Combat{};
    std::cout << "Started " << (selectedMode == CameraMode::Infinite ? "infinite" : "fixed")
        << " map; seed " << tileMap.procedural.seed << '\n';
}

void Game::update(float dt) {
    if (session.state == GameState::Menu || session.state == GameState::GameOver) {
        if (canvas.keyPressed('1')) selectedMode = CameraMode::Fixed;
        if (canvas.keyPressed('2')) selectedMode = CameraMode::Infinite;
    }
    const bool escape = canvas.keyPressed(VK_ESCAPE);
    const bool enter = canvas.keyPressed(VK_RETURN);
    const bool debug = canvas.keyPressed(VK_F1);
    const bool escapePressed = escape && !previousEscape;
    const bool enterPressed = enter && !previousEnter;
    if (escapePressed) {
        if (session.state == GameState::Menu || session.state == GameState::GameOver) running = false;
        else session.togglePause();
    }
    else if (enterPressed) {
        if (session.state == GameState::Menu || session.state == GameState::GameOver) {
            startSession();
        }
        else if (session.state == GameState::Paused) session.togglePause();
    }
    if (session.state == GameState::Paused && canvas.keyPressed('Q')) running = false;
    if (debug && !previousDebug) showCollider = !showCollider;
    previousEscape = escape;
    previousEnter = enter;
    previousDebug = debug;

    const Vector2 input = {
        float(canvas.keyPressed('D')) - float(canvas.keyPressed('A')),
        float(canvas.keyPressed('S')) - float(canvas.keyPressed('W'))
    };
    // A pause/resume/start event consumes no simulation time from the old state.
    const float step = (escapePressed || enterPressed) ? 0.0f : dt;
    const float terrainMultiplier = tileMap.isRoad(session.player.position) ? GameConfig::RoadSpeedMultiplier : 1.0f;
    session.update(input, step, worldSize, viewport, terrainMultiplier);
    if (session.state == GameState::Playing) {
        tileMap.resolveMovement(session.player, step);
        session.camera.follow(session.player.position, worldSize, viewport);
    }
    enemies.update(session, step, worldSize, viewport);
    updateCombat(step);
    if (session.state == GameState::Playing && step > 0) {
        for (UpgradePickup& pickup : combat.pickups) {
            if (!pickup.active) continue;
            const float dx = pickup.position.x - session.player.position.x;
            const float dy = pickup.position.y - session.player.position.y;
            if (selectedMode == CameraMode::Infinite
                && dx*dx + dy*dy > GameConfig::InfiniteReclaimDistance * GameConfig::InfiniteReclaimDistance)
                pickup.active = false;
            else tileMap.placeOnLand(session.player.radius, pickup.position);
        }
    }
}

void Game::updateCombat(float dt) {
    const bool shoot = canvas.mouseButtonPressed(GamesEngineeringBase::MouseLeft);
    const bool clicked = shoot && !previousShoot;
    previousShoot = shoot;
    const bool aoe = canvas.keyPressed(VK_SPACE);
    const bool aoeClicked = aoe && !previousAoe;
    previousAoe = aoe;
    if (session.state != GameState::Playing || dt <= 0) return;
    combat.update(dt);
    playerShots.update(dt);
    enemyShots.update(dt);
    combat.resolve(session.player, enemies, playerShots, enemyShots);
    playerShots.recycle(worldSize, selectedMode == CameraMode::Fixed);
    enemyShots.recycle(worldSize, selectedMode == CameraMode::Fixed);
    if (session.player.health <= 0) {
        session.state = GameState::GameOver;
        session.player.velocity = {};
        return;
    }
    combat.collect(session.player);
    if (aoeClicked) combat.fireAoe(enemies, session.player.aoeTargets);
    playerShootCooldown = ClampValue(playerShootCooldown - dt, 0, session.player.attackInterval);
    if (clicked && playerShootCooldown <= 0) {
        const Vector2 aim = {
            float(canvas.getMouseX()) + session.camera.position.x - session.player.position.x,
            float(canvas.getMouseY()) + session.camera.position.y - session.player.position.y
        };
        if (playerShots.spawn(session.player.position, aim, ProjectileConfig::PlayerSpeed,
            ProjectileConfig::PlayerLifetime, ProjectileConfig::PlayerDamage, ProjectileConfig::PlayerRadius)) {
            playerShootCooldown = session.player.attackInterval;
        }
    }
    enemies.shootAtPlayer(session.player.position, dt, enemyShots);
}

void Game::drawProjectiles() {
    drawShotPool(canvas, session.camera, playerShots, 255, 245, 110);
    drawShotPool(canvas, session.camera, enemyShots, 255, 80, 50);
}

void Game::drawEnemies() {
    for (unsigned int i = 0; i < GameConfig::MaxEnemies; ++i) {
        const Enemy& enemy = enemies.at(i);
        if (!enemy.active) continue;
        const EnemyDefinition& stats = EnemyManager::definition(enemy.type);
        const Vector2 screen = session.camera.worldToScreen(enemy.position);
        const int height = stats.imageHeight;
        const int width = int(playerImage.width * height / playerImage.height);
        const int left = int(screen.x) - width/2, top = int(screen.y) - height/2;
        if (left + width < 0 || top + height < 0 || left >= int(canvas.getWidth()) || top >= int(canvas.getHeight())) continue;
        for (int row = 0; row < height; ++row)
            for (int col = 0; col < width; ++col) {
                const int x = left + col, y = top + row;
                if (x < 0 || y < 0 || x >= int(canvas.getWidth()) || y >= int(canvas.getHeight())) continue;
                const unsigned int sx = col * playerImage.width / width, sy = row * playerImage.height / height;
                if (playerImage.alphaAtUnchecked(sx, sy) == 0) continue;
                const unsigned char* pixel = playerImage.atUnchecked(sx, sy);
                if (enemy.hitFlash > 0) canvas.draw(x, y, 255, 255, 255);
                else canvas.draw(x, y, pixel[0] * stats.red / 255, pixel[1] * stats.green / 255, pixel[2] * stats.blue / 255);
            }
        Hud::text(canvas, left, top - 17, stats.label);
        if (showCollider) {
            char health[24];
            std::snprintf(health, sizeof(health), "HP %.0f", enemy.health);
            Hud::text(canvas, left + 16, top - 17, health);
            const int radius = int(stats.radius);
            for (int y = -radius; y <= radius; ++y)
                for (int x = -radius; x <= radius; ++x) {
                    const int distance = x*x+y*y, px = int(screen.x)+x, py = int(screen.y)+y;
                    if (distance <= radius*radius && distance >= (radius-1)*(radius-1)
                        && px >= 0 && py >= 0 && px < int(canvas.getWidth()) && py < int(canvas.getHeight()))
                        canvas.draw(px, py, stats.red, stats.green, stats.blue);
                }
            Hud::text(canvas, left, top + height + 2, enemy.spawnedInside ? "IN" : "OUT");
        }
    }
}

void Game::drawPlayer() {
    if (session.player.invulnerability > 0 && int(session.player.invulnerability * 24) % 2 == 1) return;
    const Vector2 screen = session.camera.worldToScreen(session.player.position);
    constexpr int height = 48;
    const int width = int(playerImage.width * height / playerImage.height);
    const int left = int(screen.x) - width / 2;
    const int top = int(screen.y) - height / 2;
    // Nearest-neighbour scaling of the user's placeholder, preserving aspect.
    for (int row = 0; row < height; ++row)
        for (int col = 0; col < width; ++col) {
            const int x = left + col, y = top + row;
            if (x < 0 || y < 0 || x >= int(canvas.getWidth()) || y >= int(canvas.getHeight())) continue;
            const unsigned int sx = col * playerImage.width / width;
            const unsigned int sy = row * playerImage.height / height;
            if (playerImage.alphaAtUnchecked(sx, sy) > 0)
                canvas.draw(x, y, playerImage.atUnchecked(sx, sy));
        }
    if (showCollider) {
        const int radius = int(session.player.radius);
        for (int y = -radius; y <= radius; ++y)
            for (int x = -radius; x <= radius; ++x) {
                const int distance = x*x + y*y;
                const int px = int(screen.x) + x, py = int(screen.y) + y;
                if (distance <= radius*radius && distance >= (radius-1)*(radius-1)
                    && px >= 0 && py >= 0 && px < int(canvas.getWidth()) && py < int(canvas.getHeight()))
                    canvas.draw(px, py, 255, 80, 80);
            }
    }
}

void Game::drawCombat() {
    for (const UpgradePickup& pickup : combat.pickups) {
        if (!pickup.active) continue;
        const Vector2 screen = session.camera.worldToScreen(pickup.position);
        for (int y = -10; y <= 10; ++y)
            for (int x = -10; x <= 10; ++x) {
                const int px = int(screen.x) + x, py = int(screen.y) + y;
                if (px >= 0 && py >= 0 && px < int(canvas.getWidth()) && py < int(canvas.getHeight()))
                    canvas.draw(px, py, pickup.attackSpeed ? 40 : 180, 140, 200);
            }
        Hud::text(canvas, int(screen.x) - 5, int(screen.y) - 7, pickup.attackSpeed ? "F" : "N");
    }
    if (combat.feedbackTime <= 0) return;
    for (unsigned int i = 0; i < combat.targetCount; ++i) {
        const Vector2 screen = session.camera.worldToScreen(combat.targetPositions[i]);
        // Store positions rather than enemy slots: lethal hits and slot reuse keep valid feedback.
        constexpr int radius = 26;
        for (int y = -radius; y <= radius; ++y)
            for (int x = -radius; x <= radius; ++x) {
                const int distance = x*x + y*y;
                const int px = int(screen.x) + x, py = int(screen.y) + y;
                if (distance <= radius*radius && distance >= (radius-2)*(radius-2)
                    && px >= 0 && py >= 0 && px < int(canvas.getWidth()) && py < int(canvas.getHeight()))
                    canvas.draw(px, py, 100, 255, 255);
            }
        Hud::text(canvas, int(screen.x) - 17, int(screen.y) - 8, "AOE");
    }
}

void Game::drawHud() {
    const int panelHeight = showCollider ? 206 : 134;
    // Move the overlay out of the way when the player reaches the top edge.
    const int panelY = session.state != GameState::Menu
        && session.camera.worldToScreen(session.player.position).y < panelHeight + 24
        ? int(canvas.getHeight()) - panelHeight : 0;
    for (int y = 0; y < panelHeight; ++y)
        for (unsigned int x = 0; x < canvas.getWidth(); ++x) canvas.draw(x, panelY + y, 20, 24, 32);
    if (session.state == GameState::Menu) {
        Hud::text(canvas, 16, panelY + 12, "BALATRONT - ENTER TO START / ESC TO QUIT");
        Hud::text(canvas, 16, panelY + 36, "WASD MOVE / LEFT CLICK SHOOT - USE ENGLISH INPUT");
        Hud::text(canvas, 16, panelY + 60, "1 FIXED MAP / 2 RANDOM INFINITE MAP");
        Hud::text(canvas, 16, panelY + 84, selectedMode == CameraMode::Infinite
            ? "SELECTED: INFINITE" : "SELECTED: FIXED");
    } else {
        char status[128];
        const bool onRoad = tileMap.isRoad(session.player.position);
        std::snprintf(status, sizeof(status), "HP %.0f  X %.1f  Y %.1f  TIME %.1f  FPS %.0f  %s SPD %.0f",
            session.player.health, session.player.position.x, session.player.position.y, session.elapsed, fps,
            onRoad ? "ROAD" : "GRASS", session.player.speed * (onRoad ? GameConfig::RoadSpeedMultiplier : 1.0f));
        Hud::text(canvas, 16, panelY + 12, status);
        Hud::text(canvas, 16, panelY + 36, session.state == GameState::Paused
            ? "PAUSED - ESC OR ENTER TO RESUME / Q TO QUIT"
            : session.state == GameState::GameOver ? "GAME OVER - ENTER RESTART / ESC QUIT"
            : "WASD MOVE / LEFT CLICK SHOOT / SPACE AOE / ESC PAUSE / F1 DEBUG");
        std::snprintf(status, sizeof(status), "AOE %.1f S  N %u  SHOT %.3f S  KILLS %u",
            combat.aoeCooldown, session.player.aoeTargets, session.player.attackInterval, combat.kills);
        Hud::text(canvas, 16, panelY + 60, status);
        Hud::text(canvas, 16, panelY + 84, combat.upgradeFeedback > 0
            ? (combat.lastUpgradeSpeed ? "FAST DEAL COLLECTED" : "FULL HOUSE COLLECTED")
            : "PICK UP F FOR FIRE RATE / N FOR AOE TARGETS");
        if (session.state == GameState::GameOver)
            std::snprintf(status, sizeof(status), "1 FIXED / 2 INFINITE - NEXT: %s",
                selectedMode == CameraMode::Infinite ? "INFINITE" : "FIXED");
        else if (selectedMode == CameraMode::Infinite)
            std::snprintf(status, sizeof(status), "INFINITE - SEED %u", tileMap.procedural.seed);
        else std::snprintf(status, sizeof(status), "FIXED - RESOURCES/TILES.TXT");
        Hud::text(canvas, 16, panelY + 108, status);
        if (showCollider) {
            std::snprintf(status, sizeof(status), "NPC %u/256  SPAWN %.2f  IN %u  OUT %u  SKIP %u",
                enemies.activeCount(), EnemyManager::spawnInterval(session.elapsed), enemies.insideCount,
                enemies.outsideCount, enemies.capacitySkips + enemies.locationSkips);
            Hud::text(canvas, 16, panelY + 132, status);
            std::snprintf(status, sizeof(status), "G %u HP35  S %u HP18  B %u HP120  T %u HP65",
                enemies.typeCounts[0], enemies.typeCounts[1], enemies.typeCounts[2], enemies.typeCounts[3]);
            Hud::text(canvas, 16, panelY + 156, status);
            std::snprintf(status, sizeof(status), "SHOTS PLAYER %u/%u  ENEMY %u/%u  SKIP %u",
                playerShots.activeCount(), GameConfig::MaxPlayerProjectiles,
                enemyShots.activeCount(), GameConfig::MaxEnemyProjectiles,
                playerShots.skippedCount() + enemyShots.skippedCount());
            Hud::text(canvas, 16, panelY + 180, status);
        }
    }
}

void Game::render() {
    canvas.clear();
    tileMap.draw(canvas, session.camera);
    if (session.state != GameState::Menu) {
        drawEnemies();
        drawPlayer();
        drawProjectiles();
        drawCombat();
    }
    drawHud();
}
