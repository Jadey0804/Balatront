#include "Game.h"
#include "Hud.h"
#include "SaveGame.h"
#include <cstdio>
#include <fstream>
#include <iostream>
#include <ctime>
#include <chrono>
#include <cstring>

namespace {
    template<unsigned int Capacity>
    void paintBulletPool(GamesEngineeringBase::Window& window, const Camera& gameCamera,
        const ProjectilePool<Capacity>& bulletList, unsigned char colorR, unsigned char colorG, unsigned char colorB) {
        for (unsigned int i = 0; i < Capacity; ++i) {
            const Projectile& shot = bulletList.at(i);
            if (!shot.active) continue;
            const Vector2 screen = gameCamera.worldToScreen(shot.position);
            const int radius = int(shot.radius), centreX = int(screen.x), centreY = int(screen.y);
            if (centreX + radius < 0 || centreY + radius < 0
                || centreX - radius >= int(window.getWidth()) || centreY - radius >= int(window.getHeight())) continue;
            for (int y = -radius; y <= radius; ++y)
                for (int x = -radius; x <= radius; ++x) {
                    const int px = centreX + x, py = centreY + y;
                    if (x*x + y*y <= radius*radius && px >= 0 && py >= 0
                        && px < int(window.getWidth()) && py < int(window.getHeight()))
                        window.draw(px, py, colorR, colorG, colorB);
                }
        }
    }

    bool readOneImage(GamesEngineeringBase::Image& image, const char* filePlace) {
        // Check the file first because the course loader cannot read a missing file safely.
        std::ifstream file(filePlace, std::ios::binary);
        if (!file) { std::cerr << "Missing resource: " << filePlace << '\n'; return false; }
        file.close();
        if (!image.load(filePlace) || image.width == 0 || image.height == 0) {
            std::cerr << "Unsupported image: " << filePlace << '\n'; return false;
        }
        return true;
    }

    bool readAttackDirection(const GamesEngineeringBase::Window& window, Vector2 heroPlace, Vector2& outWay) {
        float x = 0, y = 0;
        for (unsigned int id = 0; id < XUSER_MAX_COUNT; ++id) {
            XINPUT_STATE state = {};
            if (XInputGetState(id, &state) != ERROR_SUCCESS) continue;
            const float rx = float(state.Gamepad.sThumbRX), ry = float(state.Gamepad.sThumbRY);
            if (rx*rx + ry*ry > float(XINPUT_GAMEPAD_RIGHT_THUMB_DEADZONE)
                * float(XINPUT_GAMEPAD_RIGHT_THUMB_DEADZONE)) { x = rx; y = -ry; break; }
        }
        if (x == 0 && y == 0 && window.mouseButtonPressed(GamesEngineeringBase::MouseLeft)) {
            x = float(window.getMouseInWindowX()) - heroPlace.x;
            y = float(window.getMouseInWindowY()) - heroPlace.y;
        }
        if (x == 0 && y == 0) return false;
        const float angle = std::atan2(y, x);
        int sector = int(std::floor((angle + 0.392699082f) / 0.785398163f));
        if (sector < 0) sector += 8;
        static const Vector2 directions[8] = {
            {1,0}, {0.707106781f,0.707106781f}, {0,1}, {-0.707106781f,0.707106781f},
            {-1,0}, {-0.707106781f,-0.707106781f}, {0,-1}, {0.707106781f,-0.707106781f}
        };
        outWay = directions[sector % 8];
        return true;
    }

    bool readControllerMove(Vector2& outMove) {
        for (unsigned int id = 0; id < XUSER_MAX_COUNT; ++id) {
            XINPUT_STATE state = {};
            if (XInputGetState(id, &state) != ERROR_SUCCESS) continue;
            const float x = float(state.Gamepad.sThumbLX);
            const float y = -float(state.Gamepad.sThumbLY);
            const float length = std::sqrt(x*x + y*y);
            if (length <= XINPUT_GAMEPAD_LEFT_THUMB_DEADZONE) continue;
            const float magnitude = ClampValue((length - XINPUT_GAMEPAD_LEFT_THUMB_DEADZONE)
                / (32767.0f - XINPUT_GAMEPAD_LEFT_THUMB_DEADZONE), 0, 1);
            outMove = {x / length * magnitude, y / length * magnitude};
            return true;
        }
        return false;
    }

    void writeMiddleText(GamesEngineeringBase::Window& window, int lineY, const char* words) {
        Hud::text(window, (int(window.getWidth()) - int(std::strlen(words)) * 12) / 2, lineY, words);
    }
}

bool Game::makeReady() {
    nextMapSeed = static_cast<std::uint32_t>(std::chrono::high_resolution_clock::now().time_since_epoch().count());
    canvas.create(GameConfig::WindowWidth, GameConfig::WindowHeight, "Balatront");
    if (!GameplaySettings::load("Resources/gameplay.txt")) { startupError = GameplaySettings::error(); return false; }
    session.player = Player{};
    if (!readOneImage(playerImage, "Resources/L.png")) return false;
    if (!enemySprites.load("Resources/Sprites/sprites.txt")) { startupError = enemySprites.error(); return false; }
    enemySprites.configure(enemies);
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
    if (!makeReady()) {
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
        gameUpdate(dt);
        if (!running) break;
        paintAll();
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

void Game::startOneGame() {
    selectedMode = CameraMode::Fixed;
    level.reset(worldSize);
    tileMap.mode = selectedMode;
    session.camera.mode = selectedMode;
    if (!tileMap.findSpawn(session.player.radius, spawnPosition)) { running = false; return; }
    session.start(worldSize, viewport);
    session.player.position = session.player.previousPosition = spawnPosition;
    session.camera.follow(spawnPosition, worldSize, viewport);
    enemies.reset(nextMapSeed);
    playerShots.reset();
    enemyShots.reset();
    manualShots.reset();
    playerShootCooldown = 0;
    manualShootCooldown = 0;
    combat = Combat{};
    healthPickups.reset(nextMapSeed);
    std::cout << "Started " << (selectedMode == CameraMode::Infinite ? "infinite" : "fixed")
        << " map; seed " << tileMap.procedural.seed << '\n';
}

void Game::goToNextMap() {
    selectedMode = CameraMode::Infinite;
    tileMap.mode = session.camera.mode = selectedMode;
    nextMapSeed = nextMapSeed * 1664525u + 1013904223u;
    tileMap.procedural.seed = nextMapSeed;
    if (!tileMap.findSpawn(session.player.radius, spawnPosition)) { running = false; return; }
    // Keep the player health and power items when changing to the next map.
    session.player.position = session.player.previousPosition = spawnPosition;
    session.player.velocity = {};
    session.camera.follow(spawnPosition, worldSize, viewport);
    session.elapsed = 0;
    enemies.reset(nextMapSeed);
    playerShots.reset();
    enemyShots.reset();
    manualShots.reset();
    healthPickups.reset(nextMapSeed);
    for (UpgradePickup& pickup : combat.pickups) pickup = UpgradePickup{};
    combat.targetCount = 0;
    combat.feedbackTime = combat.upgradeFeedback = 0;
    for (HitEffect& hit : combat.hitEffects) hit = HitEffect{};
    combat.nextHitEffect = 0;
    level.number = 2;
    level.portalOpen = false;
    level.portalTime = 0;
}

void Game::gameUpdate(float dt) {
    saveMessageTime = ClampValue(saveMessageTime - dt, 0, 4);
    const bool save = canvas.keyPressed(VK_F5), load = canvas.keyPressed(VK_F9);
    const bool savePressed = save && !previousSave, loadPressed = load && !previousLoad;
    previousSave = save; previousLoad = load;
    if (savePressed || loadPressed) {
        saveMessage = loadPressed ? SaveGame::load(*this) : SaveGame::save(*this);
        saveMessageTime = 4;
        previousEscape = canvas.keyPressed(VK_ESCAPE);
        previousEnter = canvas.keyPressed(VK_RETURN);
        previousDebug = canvas.keyPressed(VK_F1);
        previousAoe = canvas.keyPressed(VK_SPACE);
        return; // Save and load only use the last finished game frame
    }
    const bool escape = canvas.keyPressed(VK_ESCAPE);
    const bool enter = canvas.keyPressed(VK_RETURN);
    const bool debug = canvas.keyPressed(VK_F1);
    const bool escapePressed = escape && !previousEscape;
    const bool enterPressed = enter && !previousEnter;
    if (escapePressed) {
        if (session.state == GameState::Menu || session.state == GameState::GameOver
            || session.state == GameState::Victory) running = false;
        else session.togglePause();
    }
    else if (enterPressed) {
        if (session.state == GameState::Menu || session.state == GameState::GameOver
            || session.state == GameState::Victory) {
            startOneGame();
        }
        else if (session.state == GameState::Paused) session.togglePause();
    }
    if (session.state == GameState::Paused && canvas.keyPressed('Q')) running = false;
    if (debug && !previousDebug) showCollider = !showCollider;
    previousEscape = escape;
    previousEnter = enter;
    previousDebug = debug;

    Vector2 input = {
        float(canvas.keyPressed('D')) - float(canvas.keyPressed('A')),
        float(canvas.keyPressed('S')) - float(canvas.keyPressed('W'))
    };
    if (input.x == 0 && input.y == 0) readControllerMove(input);
    // Do not add game time on the frame when the game state changes.
    const float step = (escapePressed || enterPressed) ? 0.0f : dt;
    const float terrainMultiplier = tileMap.isRoad(session.player.position) ? GameplaySettings::get().roadMultiplier : 1.0f;
    session.update(input, step, worldSize, viewport, terrainMultiplier);
    if (session.state == GameState::Playing) {
        tileMap.resolveMovement(session.player, step);
        session.camera.follow(session.player.position, worldSize, viewport);
        if (tileMap.isLava(session.player.position))
            session.player.takeContinuousDamage(GameplaySettings::get().lavaDamagePerSecond * step);
        healthPickups.update(session.player, tileMap, step);
    }
    enemies.update(session, step, worldSize, viewport);
    fightUpdate(step);
    if (session.state == GameState::Playing && level.number == 2
        && session.elapsed >= GameplaySettings::get().firstLevelDuration) {
        session.state = GameState::Victory;
        session.player.velocity = {};
        return;
    }
    if (level.update(session, step)) { goToNextMap(); return; }
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

void Game::fightUpdate(float dt) {
    const bool aoe = canvas.keyPressed(VK_SPACE);
    const bool aoeClicked = aoe && !previousAoe;
    previousAoe = aoe;
    if (session.state != GameState::Playing || dt <= 0) return;
    combat.update(dt);
    playerShots.update(dt);
    enemyShots.update(dt);
    manualShots.update(dt);
    combat.resolve(session.player, enemies, playerShots, enemyShots);
    combat.resolveManual(enemies, manualShots);
    playerShots.recycle(worldSize, selectedMode == CameraMode::Fixed);
    enemyShots.recycle(worldSize, selectedMode == CameraMode::Fixed);
    manualShots.recycle(worldSize, selectedMode == CameraMode::Fixed);
    if (session.player.health <= 0) {
        session.state = GameState::GameOver;
        session.player.velocity = {};
        return;
    }
    combat.collect(session.player);
    if (aoeClicked) combat.fireAoe(enemies, session.player.aoeTargets);
    playerShootCooldown = ClampValue(playerShootCooldown - dt, 0, session.player.attackInterval);
    if (playerShootCooldown <= 0) {
        const Enemy* target = enemies.nearest(session.player.position);
        if (target) {
            Vector2 aim = {target->position.x - session.player.position.x,
                target->position.y - session.player.position.y};
            // Give the bullet a direction when the player and enemy are in the same place.
            if (aim.x == 0 && aim.y == 0) aim = {1, 0};
            playerShots.spawn(session.player.position, aim, GameplaySettings::get().playerProjectile.speed,
                GameplaySettings::get().playerProjectile.lifetime, GameplaySettings::get().playerProjectile.damage, GameplaySettings::get().playerProjectile.radius);
            playerShootCooldown = session.player.attackInterval;
        }
    }
    const float manualInterval = GameplaySettings::get().manualAttackInterval
        * session.player.attackInterval / GameplaySettings::get().attackInterval;
    manualShootCooldown = ClampValue(manualShootCooldown - dt, 0, manualInterval);
    Vector2 aim;
    if (readAttackDirection(canvas, session.camera.worldToScreen(session.player.position), aim)
        && manualShootCooldown <= 0) {
        manualShots.spawn(session.player.position, aim, GameplaySettings::get().manualProjectile.speed,
            GameplaySettings::get().manualProjectile.lifetime, GameplaySettings::get().manualProjectile.damage,
            GameplaySettings::get().manualProjectile.radius);
        manualShootCooldown = manualInterval;
    }
    enemies.shootAtPlayer(session.player.position, dt, enemyShots);
}

void Game::paintBullets() {
    paintBulletPool(canvas, session.camera, playerShots, 255, 245, 110);
    paintBulletPool(canvas, session.camera, enemyShots, 255, 80, 50);
    for (unsigned int i = 0; i < GameConfig::MaxManualProjectiles; ++i) {
        const Projectile& shot = manualShots.at(i);
        if (shot.active) enemySprites.drawManualBomb(canvas, session.camera.worldToScreen(shot.position));
    }
}

void Game::paintMonsters() {
    for (unsigned int i = 0; i < GameConfig::MaxEnemies; ++i) {
        const Enemy& enemy = enemies.at(i);
        if (!enemy.active && !enemy.dying) continue;
        const EnemyDefinition& stats = EnemyManager::definition(enemy.type);
        const Vector2 screen = session.camera.worldToScreen(enemy.position);
        enemySprites.draw(canvas, enemy, screen, session.player.position);
        if (!enemy.active) continue;
        const int height = stats.imageHeight;
        const int left = int(screen.x) - int(stats.radius), top = int(screen.y) - height/2;
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

void Game::paintHero() {
    if (session.player.damageFeedback > 0 && int(session.player.damageAnimationTime * 24) % 2 == 1) return;
    const Vector2 screen = session.camera.worldToScreen(session.player.position);
    constexpr int height = 48;
    const int width = int(playerImage.width * height / playerImage.height);
    const int left = int(screen.x) - width / 2;
    const int top = int(screen.y) - height / 2;
    // Make the player picture bigger but keep its original shape.
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

void Game::paintFight() {
    for (unsigned int i = 0; i < GameConfig::MaxHealthPickups; ++i) {
        const HealthPickup& pickup = healthPickups.at(i);
        if (pickup.active) enemySprites.drawHealthPickup(canvas, session.camera.worldToScreen(pickup.position));
    }
    for (const HitEffect& hit : combat.hitEffects)
        if (hit.remaining > 0) enemySprites.drawBombHit(canvas, session.camera.worldToScreen(hit.position));
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
        // Keep the hit place so the effect is still correct after an enemy is removed.
        enemySprites.drawAoe(canvas, screen, CombatConfig::FeedbackTime - combat.feedbackTime);
    }
}

void Game::paintInformation() {
    const int panelHeight = showCollider ? 230 : 158;
    // Put the information box at bottom when the player is near the top.
    const int panelY = session.state != GameState::Menu
        && session.camera.worldToScreen(session.player.position).y < panelHeight + 24
        ? int(canvas.getHeight()) - panelHeight : 0;
    const unsigned char background[] = {20, 24, 32};
    constexpr unsigned int alpha = 204; // The background can still be seen a little.
    for (int y = 0; y < panelHeight; ++y)
        for (unsigned int x = 0; x < canvas.getWidth(); ++x) {
            auto* pixel = canvas.backBuffer() + ((panelY + y) * canvas.getWidth() + x) * 3;
            for (int channel = 0; channel < 3; ++channel)
                pixel[channel] = static_cast<unsigned char>((background[channel] * alpha
                    + pixel[channel] * (255 - alpha)) / 255);
        }
    if (session.state == GameState::Menu) {
        Hud::text(canvas, 16, panelY + 12, "BALATRONT - ENTER TO START / ESC TO QUIT");
        Hud::text(canvas, 16, panelY + 36, "WASD MOVE / AUTO FIRE - USE ENGLISH INPUT");
        char instructions[96];
        std::snprintf(instructions, sizeof(instructions), "LEVEL 1: SURVIVE %.0f SECONDS IN THE FIXED MAP",
            GameplaySettings::get().firstLevelDuration);
        Hud::text(canvas, 16, panelY + 60, instructions);
        Hud::text(canvas, 16, panelY + 84, "ENTER THE CENTRE PORTAL TO REACH LEVEL 2");
    } else {
        char status[128];
        const bool onRoad = tileMap.isRoad(session.player.position);
        const bool onLava = tileMap.isLava(session.player.position);
        std::snprintf(status, sizeof(status), "HP %.0f  X %.1f  Y %.1f  TIME %.1f  FPS %.0f  %s SPD %.0f",
            session.player.health, session.player.position.x, session.player.position.y, session.elapsed, fps,
            onLava ? "LAVA" : onRoad ? "ROAD" : "GRASS",
            session.player.speed * (onRoad ? GameplaySettings::get().roadMultiplier : 1.0f));
        Hud::text(canvas, 16, panelY + 12, status);
        Hud::text(canvas, 16, panelY + 36, session.state == GameState::Paused
            ? "PAUSED - ESC/ENTER RESUME / F5 SAVE / F9 LOAD / Q QUIT"
            : session.state == GameState::GameOver ? "GAME OVER - ENTER RESTART / ESC QUIT"
            : session.state == GameState::Victory ? "VICTORY - ENTER RESTART / ESC QUIT"
            : "WASD MOVE / MOUSE OR RIGHT STICK BOMB / SPACE AOE / ESC PAUSE");
        const float manualInterval = GameplaySettings::get().manualAttackInterval
            * session.player.attackInterval / GameplaySettings::get().attackInterval;
        std::snprintf(status, sizeof(status), "AOE %.1f S N %u  SHOT %.3f S  BOMB %.3f S  KILLS %u",
            combat.aoeCooldown, session.player.aoeTargets, session.player.attackInterval,
            manualInterval, combat.kills);
        Hud::text(canvas, 16, panelY + 60, status);
        Hud::text(canvas, 16, panelY + 84, combat.upgradeFeedback > 0
            ? (combat.lastUpgradeSpeed ? "FAST DEAL COLLECTED" : "FULL HOUSE COLLECTED")
            : "PICK UP F FOR FIRE RATE / N FOR AOE TARGETS");
        if (level.number == 2)
            std::snprintf(status, sizeof(status), "LEVEL 2 - SURVIVE %.1f SECONDS - SEED %u",
                ClampValue(GameplaySettings::get().firstLevelDuration - session.elapsed, 0,
                    GameplaySettings::get().firstLevelDuration), tileMap.procedural.seed);
        else if (level.portalOpen)
            std::snprintf(status, sizeof(status), "LEVEL 1 - PORTAL OPEN AT %.0f %.0f",
                level.portalPosition.x, level.portalPosition.y);
        else std::snprintf(status, sizeof(status), "LEVEL 1 - PORTAL IN %.1f SECONDS",
            ClampValue(GameplaySettings::get().firstLevelDuration - session.elapsed, 0, GameplaySettings::get().firstLevelDuration));
        Hud::text(canvas, 16, panelY + 108, status);
        if (showCollider) {
            std::snprintf(status, sizeof(status), "NPC %u/256  SPAWN %.2f  IN %u  OUT %u  SKIP %u",
                enemies.activeCount(), EnemyManager::spawnInterval(session.elapsed), enemies.insideCount,
                enemies.outsideCount, enemies.capacitySkips + enemies.locationSkips);
            Hud::text(canvas, 16, panelY + 156, status);
            const auto& stats = GameplaySettings::get().enemies;
            std::snprintf(status, sizeof(status), "G %u HP%.0f  S %u HP%.0f  B %u HP%.0f  T %u HP%.0f",
                enemies.typeCounts[0], stats[0].health, enemies.typeCounts[1], stats[1].health,
                enemies.typeCounts[2], stats[2].health, enemies.typeCounts[3], stats[3].health);
            Hud::text(canvas, 16, panelY + 180, status);
            std::snprintf(status, sizeof(status), "SHOTS PLAYER %u/%u  ENEMY %u/%u  SKIP %u",
                playerShots.activeCount(), GameConfig::MaxPlayerProjectiles,
                enemyShots.activeCount(), GameConfig::MaxEnemyProjectiles,
                playerShots.skippedCount() + enemyShots.skippedCount());
            Hud::text(canvas, 16, panelY + 204, status);
        }
    }
    Hud::text(canvas, 16, panelY + 132, saveMessageTime > 0 ? saveMessage : "F5 SAVE / F9 LOAD" );
}

unsigned int Game::countPoint() const {
    return combat.kills * 10 + combat.upgradesCollected * 100
        + static_cast<unsigned int>(session.player.health + 0.5f) * 5;
}

const char* Game::getLevelLetter() const {
    const unsigned int value = countPoint();
    return value >= 5000 ? "SSS" : value >= 3000 ? "SS" : value >= 1500 ? "S" : "A";
}

void Game::paintFinish() {
    if (session.state != GameState::Victory && session.state != GameState::GameOver) return;
    const int left = int(canvas.getWidth()) / 2 - 260;
    const int top = int(canvas.getHeight()) / 2 - 105;
    for (int y = top; y < top + 210; ++y)
        for (int x = left; x < left + 520; ++x) {
            auto* pixel = canvas.backBuffer() + (y * canvas.getWidth() + x) * 3;
            for (int channel = 0; channel < 3; ++channel)
                pixel[channel] = static_cast<unsigned char>(pixel[channel] * 25 / 255);
        }
    char text[96];
    writeMiddleText(canvas, top + 24, session.state == GameState::Victory ? "YOU WIN!!" : "YOU LOST...");
    std::snprintf(text, sizeof(text), "SCORE %u", countPoint());
    writeMiddleText(canvas, top + 60, text);
    std::snprintf(text, sizeof(text), "KILLS %u  BUFFS %u  HP %.0f",
        combat.kills, combat.upgradesCollected, session.player.health);
    writeMiddleText(canvas, top + 88, text);
    if (session.state == GameState::Victory) {
        std::snprintf(text, sizeof(text), "GRADE %s", getLevelLetter());
        writeMiddleText(canvas, top + 120, text);
    }
    writeMiddleText(canvas, top + 164, "ENTER RESTART / ESC QUIT");
}

void Game::paintAll() {
    canvas.clear();
    tileMap.draw(canvas, session.camera);
    if (session.state != GameState::Menu) {
        if (level.portalOpen) {
            const Vector2 screen = session.camera.worldToScreen(level.portalPosition);
            enemySprites.drawPortal(canvas, screen, level.portalTime);
            Hud::text(canvas, int(screen.x) - 35, int(screen.y) - 48, "PORTAL");
        }
        paintMonsters();
        paintHero();
        paintBullets();
        paintFight();
    }
    paintInformation();
    paintFinish();
}
