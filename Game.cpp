#include "Game.h"
#include "Hud.h"
#include <cstdio>
#include <fstream>
#include <iostream>

namespace {
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
    canvas.create(GameConfig::WindowWidth, GameConfig::WindowHeight, "Balatront - Phase 1");
    if (!loadImage(landscape, "Resources/landscape.png") || !loadImage(playerImage, "Resources/L.png"))
        return false;
    worldSize = {float(landscape.width), float(landscape.height)};
    // This phase uses the supplied 1344x1344 scene, not the later 80x60 tile map.
    if (worldSize.x < viewport.x || worldSize.y < viewport.y) {
        std::cerr << "The fixed test scene must cover the viewport.\n"; return false;
    }
    session.camera.follow({worldSize.x * 0.5f, worldSize.y * 0.5f}, worldSize, viewport);
    return true;
}

int Game::run() {
    if (!initialize()) return 1;
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
                << " | state " << int(session.state) << std::endl;
            measuredSeconds = 0.0;
            frames = 0;
        }
    }
    return 0;
}

void Game::update(float dt) {
    const bool escape = canvas.keyPressed(VK_ESCAPE);
    const bool enter = canvas.keyPressed(VK_RETURN);
    const bool debug = canvas.keyPressed(VK_F1);
    const bool escapePressed = escape && !previousEscape;
    const bool enterPressed = enter && !previousEnter;
    if (escapePressed) {
        if (session.state == GameState::Menu) running = false;
        else session.togglePause();
    }
    else if (enterPressed) {
        if (session.state == GameState::Menu) session.start(worldSize, viewport);
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
    session.update(input, (escapePressed || enterPressed) ? 0.0f : dt, worldSize, viewport);
}

void Game::drawPlayer() {
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

void Game::drawHud() {
    constexpr int panelHeight = 62;
    // Move the overlay out of the way when the player reaches the top edge.
    const int panelY = session.state != GameState::Menu
        && session.camera.worldToScreen(session.player.position).y < panelHeight + 24
        ? int(canvas.getHeight()) - panelHeight : 0;
    for (int y = 0; y < panelHeight; ++y)
        for (unsigned int x = 0; x < canvas.getWidth(); ++x) canvas.draw(x, panelY + y, 20, 24, 32);
    if (session.state == GameState::Menu) {
        Hud::text(canvas, 16, panelY + 12, "BALATRONT - ENTER TO START / ESC TO QUIT");
        Hud::text(canvas, 16, panelY + 36, "WASD TO MOVE - USE ENGLISH KEYBOARD INPUT");
    } else {
        char status[128];
        std::snprintf(status, sizeof(status), "HP %.0f  X %.1f  Y %.1f  TIME %.1f  FPS %.0f",
            session.player.health, session.player.position.x, session.player.position.y, session.elapsed, fps);
        Hud::text(canvas, 16, panelY + 12, status);
        Hud::text(canvas, 16, panelY + 36, session.state == GameState::Paused
            ? "PAUSED - ESC OR ENTER TO RESUME / Q TO QUIT" : "PLAYING - WASD MOVE / ESC PAUSE / F1 COLLIDER");
    }
}

void Game::render() {
    canvas.clear();
    const int cameraX = int(session.camera.position.x);
    const int cameraY = int(session.camera.position.y);
    for (unsigned int y = 0; y < canvas.getHeight(); ++y)
        for (unsigned int x = 0; x < canvas.getWidth(); ++x)
            canvas.draw(x, y, landscape.atUnchecked(cameraX + x, cameraY + y));
    if (session.state != GameState::Menu) drawPlayer();
    drawHud();
}
