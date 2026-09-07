#pragma once
#include <cmath>
#include "GameConfig.h"

// World positions refer to object centres. Screen positions are derived only
// during rendering; no image dimensions are stored in movement/camera logic.
struct Vector2 {
    float x = 0.0f;
    float y = 0.0f;
};

inline float ClampValue(float value, float low, float high) {
    return value < low ? low : (value > high ? high : value);
}

struct Player {
    Vector2 position;
    Vector2 previousPosition;
    Vector2 velocity;
    float health = 100.0f;
    float speed = 180.0f;
    float radius = 14.0f;
    float invulnerability = 0;
    float attackInterval = 0.35f;
    unsigned int aoeTargets = 3;

    void takeDamage(float damage) {
        if (invulnerability > 0 || health <= 0) return;
        health = ClampValue(health - damage, 0, 100);
        invulnerability = 0.35f;
    }

    void update(Vector2 input, float dt, Vector2 worldSize) {
        previousPosition = position;
        invulnerability = ClampValue(invulnerability - dt, 0, 0.35f);
        const float length = std::sqrt(input.x * input.x + input.y * input.y);
        // Preserve analog input magnitude; cap diagonal keyboard input to one.
        if (length > 1.0f) { input.x /= length; input.y /= length; }
        velocity = {input.x * speed, input.y * speed};
        position.x = ClampValue(position.x + velocity.x * dt, radius, worldSize.x - radius);
        position.y = ClampValue(position.y + velocity.y * dt, radius, worldSize.y - radius);
    }
};

enum class CameraMode { Fixed, Infinite };

struct Camera {
    Vector2 position;
    CameraMode mode = CameraMode::Fixed;

    void follow(Vector2 target, Vector2 worldSize, Vector2 viewport) {
        position = {target.x - viewport.x * 0.5f, target.y - viewport.y * 0.5f};
        if (mode == CameraMode::Fixed) {
            const float maxX = worldSize.x > viewport.x ? worldSize.x - viewport.x : 0.0f;
            const float maxY = worldSize.y > viewport.y ? worldSize.y - viewport.y : 0.0f;
            position.x = ClampValue(position.x, 0.0f, maxX);
            position.y = ClampValue(position.y, 0.0f, maxY);
        }
    }

    Vector2 worldToScreen(Vector2 world) const {
        return {world.x - position.x, world.y - position.y};
    }
};

// The same state gate is used by runtime and the movement/pause tests.
struct PlaySession {
    Player player;
    Camera camera;
    GameState state = GameState::Menu;
    float elapsed = 0.0f;

    void start(Vector2 worldSize, Vector2 viewport) {
        player = Player{};
        player.position = {worldSize.x * 0.5f, worldSize.y * 0.5f};
        player.previousPosition = player.position;
        elapsed = 0.0f;
        state = GameState::Playing;
        camera.follow(player.position, worldSize, viewport);
    }

    void togglePause() {
        if (state == GameState::Playing) { state = GameState::Paused; player.velocity = {}; }
        else if (state == GameState::Paused) state = GameState::Playing;
    }

    void update(Vector2 input, float dt, Vector2 worldSize, Vector2 viewport) {
        if (state != GameState::Playing) return;
        player.update(input, dt, worldSize);
        elapsed += dt;
        camera.follow(player.position, worldSize, viewport);
    }
};
