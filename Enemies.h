#pragma once
#include "Gameplay.h"
#include "Projectiles.h"
#include <cstdint>

enum class EnemyType { Grunt, Sprinter, Brute, Turret, Count };
enum class EnemyAnimation { Move, Attack, Hurt, Death, Count };

struct EnemyDefinition {
    float health;
    float speed;
    float contactDamage;
    float radius;
    int imageHeight;
    unsigned char red, green, blue;
    const char* label;
};

namespace EnemyConfig {
    constexpr unsigned int TypeCount = static_cast<unsigned int>(EnemyType::Count);
    constexpr unsigned int SpawnAttempts = 32;
    constexpr float SafeDistance = 160.0f;
    constexpr float OutsideMin = 16.0f;
    constexpr float OutsideMax = 64.0f;
    // Shared definitions: behaviour differs through data, not four update loops.
    constexpr EnemyDefinition Appearance[TypeCount] = {
        {0, 0, 0, 0, 40, 255, 110, 100, "G"},
        {0, 0, 0, 0, 30, 255, 225, 70, "S"},
        {0, 0, 0, 0, 60, 190, 100, 255, "B"},
        {0, 0, 0, 0, 44, 70, 210, 255, "T"}
    };
}

struct Enemy {
    bool active = false;
    EnemyType type = EnemyType::Grunt;
    Vector2 position;
    float health = 0;
    bool spawnedInside = false;
    float shootCooldown = GameplaySettings::get().turretInterval;
    Vector2 previousPosition;
    float hitFlash = 0;
    bool dying = false;
    bool faceLeft = false;
    EnemyAnimation animation = EnemyAnimation::Move;
    float animationTime = 0;
};

class EnemyManager {
public:
    float animationDurations[EnemyConfig::TypeCount][4] = {};
    void reset(std::uint32_t seed = 1) {
        for (Enemy& enemy : enemies) enemy = Enemy{};
        randomState = seed;
        accumulator = 0;
        insideCount = outsideCount = capacitySkips = locationSkips = 0;
        for (unsigned int& count : typeCounts) count = 0;
    }

    static float spawnInterval(float elapsed) {
        return elapsed < 30 ? 1.60f : elapsed < 60 ? 1.20f : elapsed < 90 ? 0.85f : 0.55f;
    }

    int spawn(EnemyType type, Vector2 position, bool inside) {
        if (static_cast<unsigned int>(type) >= EnemyConfig::TypeCount) return -1;
        for (unsigned int i = 0; i < GameConfig::MaxEnemies; ++i) {
            if (!enemies[i].active && !enemies[i].dying) {
                enemies[i] = {true, type, position, definition(type).health, inside};
                enemies[i].previousPosition = position;
                ++typeCounts[static_cast<unsigned int>(type)];
                if (inside) ++insideCount; else ++outsideCount;
                return int(i);
            }
        }
        ++capacitySkips;
        GAME_DEBUG_LOG("Enemy pool full; spawn skipped");
        return -1;
    }

    void update(const PlaySession& session, float dt, Vector2 worldSize, Vector2 viewport) {
        if (session.state != GameState::Playing || dt <= 0) return;
        for (Enemy& enemy : enemies) {
            if (enemy.dying) {
                enemy.animationTime += dt;
                if (enemy.animationTime >= animationDurations[static_cast<unsigned int>(enemy.type)][3]) enemy.dying = false;
                continue;
            }
            if (!enemy.active) continue;
            enemy.animationTime += dt;
            if (enemy.animation != EnemyAnimation::Move && enemy.animationTime >=
                animationDurations[static_cast<unsigned int>(enemy.type)][static_cast<unsigned int>(enemy.animation)]) {
                enemy.animation = EnemyAnimation::Move;
                enemy.animationTime = 0;
            }
            enemy.previousPosition = enemy.position;
            enemy.hitFlash = ClampValue(enemy.hitFlash - dt, 0, 0.15f);
            const EnemyDefinition& stats = definition(enemy.type);
            const Vector2 delta = {session.player.position.x - enemy.position.x, session.player.position.y - enemy.position.y};
            const float distance = std::sqrt(delta.x * delta.x + delta.y * delta.y);
            if (delta.x != 0) enemy.faceLeft = delta.x < 0;
            if (stats.speed > 0 && distance <= stats.radius + session.player.radius
                && enemy.animation == EnemyAnimation::Move
                && animationDurations[static_cast<unsigned int>(enemy.type)][1] > 0) {
                enemy.animation = EnemyAnimation::Attack;
                enemy.animationTime = 0;
            }
            if (session.camera.mode == CameraMode::Infinite && distance > GameConfig::InfiniteReclaimDistance) {
                enemy.active = false;
                continue; // Distant despawn is not a kill and grants no rewards.
            }
            if (distance > 0 && stats.speed > 0) {
                const float travel = ClampValue(stats.speed * dt, 0, distance);
                enemy.position.x += delta.x / distance * travel;
                enemy.position.y += delta.y / distance * travel;
            }
        }
        accumulator += dt;
        const float interval = spawnInterval(session.elapsed);
        while (accumulator >= interval) {
            accumulator -= interval;
            const EnemyType type = chooseType(session.elapsed);
            const bool inside = random01() < 0.30f;
            Vector2 point;
            if (spawnPoint(session, viewport, worldSize, definition(type).radius, inside, point))
                spawn(type, point, inside);
            else ++locationSkips;
        }
    }

    void shootAtPlayer(Vector2 target, float dt, EnemyProjectilePool& projectiles) {
        for (Enemy& enemy : enemies) {
            if (!enemy.active || enemy.type != EnemyType::Turret) continue;
            enemy.shootCooldown -= dt;
            if (enemy.shootCooldown > 0) continue;
            const Vector2 direction = {target.x - enemy.position.x, target.y - enemy.position.y};
            projectiles.spawn(enemy.position, direction, GameplaySettings::get().enemyProjectile.speed,
                GameplaySettings::get().enemyProjectile.lifetime, GameplaySettings::get().enemyProjectile.damage, GameplaySettings::get().enemyProjectile.radius);
            // No burst of overdue shots after a stalled frame, including a full pool.
            enemy.shootCooldown = GameplaySettings::get().turretInterval;
        }
    }

    const Enemy* nearest(Vector2 position) const {
        const Enemy* closest = nullptr;
        float bestDistance = 0;
        for (const Enemy& enemy : enemies) {
            if (!enemy.active) continue;
            const float dx = enemy.position.x - position.x, dy = enemy.position.y - position.y;
            const float distance = dx*dx + dy*dy;
            if (!closest || distance < bestDistance) {
                closest = &enemy;
                bestDistance = distance;
            }
        }
        return closest;
    }

    bool damage(unsigned int index, float amount) {
        if (index >= GameConfig::MaxEnemies || !enemies[index].active) return false;
        Enemy& enemy = enemies[index];
        enemy.health = ClampValue(enemy.health - amount, 0, definition(enemy.type).health);
        enemy.hitFlash = 0.15f;
        if (animationDurations[static_cast<unsigned int>(enemy.type)][2] > 0) {
            enemy.animation = EnemyAnimation::Hurt;
            enemy.animationTime = 0;
        }
        if (enemy.health > 0) return false;
        enemy.active = false;
        enemy.dying = animationDurations[static_cast<unsigned int>(enemy.type)][3] > 0;
        enemy.animation = EnemyAnimation::Death;
        enemy.animationTime = 0;
        return true;
    }

    const Enemy& at(unsigned int index) const {
        GAME_ASSERT(index < GameConfig::MaxEnemies);
        return enemies[index];
    }
    static EnemyDefinition definition(EnemyType type) {
        GAME_ASSERT(static_cast<unsigned int>(type) < EnemyConfig::TypeCount);
        const unsigned int index = static_cast<unsigned int>(type);
        EnemyDefinition result = EnemyConfig::Appearance[index];
        const EnemyStats& stats = GameplaySettings::get().enemies[index];
        result.health = stats.health; result.speed = stats.speed;
        result.contactDamage = stats.contactDamage; result.radius = stats.radius;
        return result;
    }
    unsigned int activeCount() const {
        unsigned int count = 0;
        for (const Enemy& enemy : enemies) if (enemy.active) ++count;
        return count;
    }
    unsigned int insideCount = 0, outsideCount = 0;
    unsigned int capacitySkips = 0, locationSkips = 0;
    unsigned int typeCounts[EnemyConfig::TypeCount] = {};

private:
    Enemy enemies[GameConfig::MaxEnemies] = {};
    float accumulator = 0;
    std::uint32_t randomState = 1;

    float random01() {
        // Explicit reproducible generator state, suitable for later save data.
        randomState = randomState * 1664525u + 1013904223u;
        return float(randomState >> 8) / 16777216.0f;
    }

    EnemyType chooseType(float elapsed) {
        const float roll = random01() * 100;
        if (elapsed < 30) return roll < 70 ? EnemyType::Grunt : roll < 90 ? EnemyType::Sprinter : EnemyType::Brute;
        if (elapsed < 60) return roll < 50 ? EnemyType::Grunt : roll < 75 ? EnemyType::Sprinter : roll < 95 ? EnemyType::Brute : EnemyType::Turret;
        if (elapsed < 90) return roll < 40 ? EnemyType::Grunt : roll < 65 ? EnemyType::Sprinter : roll < 90 ? EnemyType::Brute : EnemyType::Turret;
        return roll < 30 ? EnemyType::Grunt : roll < 60 ? EnemyType::Sprinter : roll < 85 ? EnemyType::Brute : EnemyType::Turret;
    }

    bool spawnPoint(const PlaySession& session, Vector2 viewport, Vector2 worldSize,
        float radius, bool inside, Vector2& point) {
        const Vector2 camera = session.camera.position;
        for (unsigned int attempt = 0; attempt < EnemyConfig::SpawnAttempts; ++attempt) {
            point = {camera.x + radius + random01() * (viewport.x - 2 * radius),
                     camera.y + radius + random01() * (viewport.y - 2 * radius)};
            if (!inside) {
                const float gap = EnemyConfig::OutsideMin + random01() * (EnemyConfig::OutsideMax - EnemyConfig::OutsideMin);
                const int edge = int(random01() * 4);
                if (edge == 0) point.x = camera.x - radius - gap;
                else if (edge == 1) point.x = camera.x + viewport.x + radius + gap;
                else if (edge == 2) point.y = camera.y - radius - gap;
                else point.y = camera.y + viewport.y + radius + gap;
            }
            if (session.camera.mode == CameraMode::Fixed && (point.x < radius || point.y < radius
                || point.x > worldSize.x-radius || point.y > worldSize.y-radius)) continue;
            const float dx = point.x - session.player.position.x, dy = point.y - session.player.position.y;
            if (dx*dx + dy*dy < EnemyConfig::SafeDistance * EnemyConfig::SafeDistance) continue;
            return true;
        }
        // No valid screen-edge position: skip, never silently change spawn type.
        return false;
    }
};
