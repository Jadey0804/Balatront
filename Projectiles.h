#pragma once
#include "Gameplay.h"

namespace ProjectileConfig {
    constexpr float PlayerInterval = 0.35f;
    constexpr float PlayerSpeed = 480.0f;
    constexpr float PlayerLifetime = 4.0f;
    constexpr float PlayerDamage = 20.0f;
    constexpr float PlayerRadius = 3.0f;
    constexpr float EnemyInterval = 1.8f;
    constexpr float EnemySpeed = 240.0f;
    constexpr float EnemyLifetime = 6.0f;
    constexpr float EnemyDamage = 12.0f;
    constexpr float EnemyRadius = 4.0f;
    constexpr float WorldMargin = 64.0f;
}

struct Projectile {
    bool active = false;
    Vector2 position;
    Vector2 velocity;
    float remainingLife = 0;
    float damage = 0;
    float radius = 0;
    Vector2 previousPosition;
    float motionFraction = 1;
};

// One implementation, two independent fixed arrays. No per-shot allocations.
template<unsigned int Capacity>
class ProjectilePool {
public:
    void reset() {
        for (Projectile& shot : shots) shot = Projectile{};
        searchStart = active = skipped = 0;
    }

    bool spawn(Vector2 position, Vector2 direction, float speed, float lifetime, float damage, float radius) {
        const float lengthSquared = direction.x * direction.x + direction.y * direction.y;
        if (lengthSquared <= 0) return false;
        if (active == Capacity) {
            ++skipped;
            GAME_DEBUG_LOG("Projectile pool full; shot skipped");
            return false;
        }
        const float factor = speed / std::sqrt(lengthSquared);
        for (unsigned int offset = 0; offset < Capacity; ++offset) {
            const unsigned int index = (searchStart + offset) % Capacity;
            if (shots[index].active) continue;
            shots[index] = {true, position, {direction.x * factor, direction.y * factor}, lifetime, damage, radius};
            shots[index].previousPosition = position;
            searchStart = (index + 1) % Capacity;
            ++active;
            return true;
        }
        return false;
    }

    void update(float dt) {
        for (unsigned int i = 0; i < Capacity; ++i) {
            Projectile& shot = shots[i];
            if (!shot.active) continue;
            shot.previousPosition = shot.position;
            const float travelTime = ClampValue(dt, 0, shot.remainingLife);
            shot.motionFraction = dt > 0 ? travelTime / dt : 0;
            shot.position.x += shot.velocity.x * travelTime;
            shot.position.y += shot.velocity.y * travelTime;
            shot.remainingLife -= dt;
        }
    }

    // Resolve the final segment before lifetime/world-bound reclamation.
    void recycle(Vector2 worldSize, bool fixedWorld = true) {
        for (unsigned int i = 0; i < Capacity; ++i) {
            const Projectile& shot = shots[i];
            if (!shot.active) continue;
            const float margin = ProjectileConfig::WorldMargin;
            // Recycle against WORLD bounds, never against the moving camera.
            if (shot.remainingLife <= 0 || (fixedWorld && (shot.position.x < -margin || shot.position.y < -margin
                || shot.position.x > worldSize.x + margin || shot.position.y > worldSize.y + margin)))
                deactivate(i);
        }
    }

    void deactivate(unsigned int index) {
        if (index >= Capacity || !shots[index].active) return;
        shots[index].active = false;
        --active;
    }

    const Projectile& at(unsigned int index) const {
        GAME_ASSERT(index < Capacity);
        return shots[index];
    }
    unsigned int activeCount() const { return active; }
    unsigned int skippedCount() const { return skipped; }

private:
    Projectile shots[Capacity] = {};
    unsigned int searchStart = 0;
    unsigned int active = 0;
    unsigned int skipped = 0;
};

using PlayerProjectilePool = ProjectilePool<GameConfig::MaxPlayerProjectiles>;
using EnemyProjectilePool = ProjectilePool<GameConfig::MaxEnemyProjectiles>;
