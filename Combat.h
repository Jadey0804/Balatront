#pragma once
#include "Enemies.h"

namespace CombatConfig {
    constexpr float AoeDamage = 40;
    constexpr float AoeCooldown = 12;
    constexpr float FeedbackTime = 0.4f;
    constexpr float MinAttackInterval = 0.08f;
    constexpr unsigned int MaxPickups = 64;
    constexpr unsigned int KillsPerPickup = 5;
    constexpr float PickupRadius = 18;
}

struct UpgradePickup {
    bool active = false;
    Vector2 position;
    bool attackSpeed = true;
};

// Relative motion reduces two moving circles to a segment against a circle.
// Returns the first contact fraction, including initial overlap and tangency.
inline bool CircleSweep(Vector2 from, Vector2 to, Vector2 targetFrom,
    Vector2 targetTo, float radius, float& fraction) {
    const Vector2 offset = {from.x - targetFrom.x, from.y - targetFrom.y};
    const Vector2 motion = {to.x - from.x - (targetTo.x - targetFrom.x),
        to.y - from.y - (targetTo.y - targetFrom.y)};
    const float c = offset.x * offset.x + offset.y * offset.y - radius * radius;
    if (c <= 0) { fraction = 0; return true; }
    const float a = motion.x * motion.x + motion.y * motion.y;
    if (a == 0) return false;
    const float b = offset.x * motion.x + offset.y * motion.y;
    const float discriminant = b * b - a * c;
    if (discriminant < 0) return false;
    fraction = (-b - std::sqrt(discriminant)) / a;
    return fraction >= 0 && fraction <= 1;
}

struct Combat {
    float aoeCooldown = 0;
    float feedbackTime = 0;
    Vector2 targetPositions[GameConfig::MaxAoeTargets] = {};
    unsigned int targetCount = 0;
    unsigned int kills = 0;
    UpgradePickup pickups[CombatConfig::MaxPickups] = {};
    unsigned int skippedPickups = 0;
    float upgradeFeedback = 0;
    bool lastUpgradeSpeed = true;

    void update(float dt) {
        aoeCooldown = ClampValue(aoeCooldown - dt, 0, CombatConfig::AoeCooldown);
        feedbackTime = ClampValue(feedbackTime - dt, 0, CombatConfig::FeedbackTime);
        upgradeFeedback = ClampValue(upgradeFeedback - dt, 0, 2);
    }

    void damageEnemy(EnemyManager& enemies, unsigned int index, float amount) {
        if (!enemies.damage(index, amount)) return;
        ++kills;
        if (kills % CombatConfig::KillsPerPickup != 0) return;
        for (UpgradePickup& pickup : pickups) {
            if (pickup.active) continue;
            pickup = {true, enemies.at(index).position,
                (kills / CombatConfig::KillsPerPickup) % 2 == 1};
            return;
        }
        ++skippedPickups;
        GAME_DEBUG_LOG("Upgrade pickup pool full; drop skipped");
    }

    void collect(Player& player) {
        for (UpgradePickup& pickup : pickups) {
            if (!pickup.active) continue;
            float fraction;
            if (!CircleSweep(player.previousPosition, player.position, pickup.position,
                pickup.position, player.radius + CombatConfig::PickupRadius, fraction)) continue;
            if (pickup.attackSpeed)
                player.attackInterval = ClampValue(player.attackInterval * 0.85f,
                    CombatConfig::MinAttackInterval, player.attackInterval);
            else if (player.aoeTargets < GameConfig::MaxAoeTargets) ++player.aoeTargets;
            lastUpgradeSpeed = pickup.attackSpeed;
            upgradeFeedback = 2;
            pickup.active = false;
        }
    }

    void fireAoe(EnemyManager& enemies, unsigned int requested) {
        if (aoeCooldown > 0) return;
        unsigned int indices[GameConfig::MaxAoeTargets] = {};
        bool selected[GameConfig::MaxEnemies] = {};
        targetCount = 0;
        // Select the complete set before applying damage. Equal HP uses slot order.
        while (targetCount < requested && targetCount < GameConfig::MaxAoeTargets) {
            int best = -1;
            for (unsigned int i = 0; i < GameConfig::MaxEnemies; ++i) {
                const Enemy& enemy = enemies.at(i);
                if (!enemy.active || selected[i]) continue;
                if (best < 0 || enemy.health > enemies.at(best).health) best = int(i);
            }
            if (best < 0) break;
            selected[best] = true;
            indices[targetCount] = static_cast<unsigned int>(best);
            targetPositions[targetCount++] = enemies.at(best).position;
        }
        if (targetCount == 0) return;
        for (unsigned int i = 0; i < targetCount; ++i)
            damageEnemy(enemies, indices[i], CombatConfig::AoeDamage);
        aoeCooldown = CombatConfig::AoeCooldown;
        feedbackTime = CombatConfig::FeedbackTime;
    }

    void resolve(Player& player, EnemyManager& enemies,
        PlayerProjectilePool& playerShots, EnemyProjectilePool& enemyShots) {
        for (unsigned int i = 0; i < GameConfig::MaxPlayerProjectiles; ++i) {
            const Projectile& shot = playerShots.at(i);
            if (!shot.active) continue;
            int closest = -1;
            float first = 2;
            for (unsigned int j = 0; j < GameConfig::MaxEnemies; ++j) {
                const Enemy& enemy = enemies.at(j);
                if (!enemy.active) continue;
                const Vector2 targetEnd = {
                    enemy.previousPosition.x + (enemy.position.x - enemy.previousPosition.x) * shot.motionFraction,
                    enemy.previousPosition.y + (enemy.position.y - enemy.previousPosition.y) * shot.motionFraction};
                float fraction;
                if (CircleSweep(shot.previousPosition, shot.position, enemy.previousPosition,
                    targetEnd, shot.radius + EnemyManager::definition(enemy.type).radius, fraction)
                    && fraction < first) { first = fraction; closest = int(j); }
            }
            if (closest >= 0) {
                damageEnemy(enemies, static_cast<unsigned int>(closest), shot.damage);
                playerShots.deactivate(i);
            }
        }
        for (unsigned int i = 0; i < GameConfig::MaxEnemyProjectiles; ++i) {
            const Projectile& shot = enemyShots.at(i);
            if (!shot.active) continue;
            const Vector2 targetEnd = {
                player.previousPosition.x + (player.position.x - player.previousPosition.x) * shot.motionFraction,
                player.previousPosition.y + (player.position.y - player.previousPosition.y) * shot.motionFraction};
            float fraction;
            if (CircleSweep(shot.previousPosition, shot.position, player.previousPosition,
                targetEnd, shot.radius + player.radius, fraction)) {
                player.takeDamage(shot.damage);
                enemyShots.deactivate(i); // Invulnerability also consumes the colliding shot.
            }
        }
        for (unsigned int i = 0; i < GameConfig::MaxEnemies; ++i) {
            const Enemy& enemy = enemies.at(i);
            if (!enemy.active) continue;
            const EnemyDefinition& stats = EnemyManager::definition(enemy.type);
            float fraction;
            if (CircleSweep(player.previousPosition, player.position, enemy.previousPosition,
                enemy.position, player.radius + stats.radius, fraction)) player.takeDamage(stats.contactDamage);
        }
    }
};
