#pragma once
#include "Combat.h"
#include "TileMap.h"

struct HealthPickup {
    bool active = false;
    Vector2 position;
    float remaining = 0;
};

class HealthPickupManager {
    friend class SaveGame;
public:
    void reset(std::uint32_t seed) {
        for (HealthPickup& pickup : pickups) pickup = HealthPickup{};
        spawnTimer = GameplaySettings::get().healthPickupInterval;
        randomState = seed ^ 0x71b4a239u;
    }
    void update(Player& player, const TileMap& map, float dt) {
        if (dt <= 0) return;
        for (HealthPickup& pickup : pickups) {
            if (!pickup.active) continue;
            pickup.remaining -= dt;
            if (pickup.remaining <= 0) { pickup.active = false; continue; }
            float fraction;
            if (CircleSweep(player.previousPosition, player.position, pickup.position, pickup.position,
                player.radius + GameplaySettings::get().healthPickupRadius, fraction)) {
                player.health = ClampValue(player.health + GameplaySettings::get().healthPickupHeal,
                    0, GameplaySettings::get().health);
                pickup.active = false;
            }
        }
        spawnTimer -= dt;
        if (spawnTimer > 0) return;
        spawnTimer = GameplaySettings::get().healthPickupInterval;
        spawn(player.position, map);
    }
    const HealthPickup& at(unsigned int index) const { return pickups[index]; }
private:
    HealthPickup pickups[GameConfig::MaxHealthPickups] = {};
    float spawnTimer = 0;
    std::uint32_t randomState = 1;

    float random01() {
        randomState = randomState * 1664525u + 1013904223u;
        return float(randomState >> 8) / 16777216.0f;
    }
    void spawn(Vector2 player, const TileMap& map) {
        HealthPickup* free = nullptr;
        for (HealthPickup& pickup : pickups) if (!pickup.active) { free = &pickup; break; }
        if (!free) return;
        for (unsigned int attempt = 0; attempt < 12; ++attempt) {
            const float angle = random01() * 6.283185307f;
            const float distance = std::sqrt(random01()) * GameplaySettings::get().healthPickupSpawnRadius;
            const Vector2 position = {player.x + std::cos(angle) * distance,
                player.y + std::sin(angle) * distance};
            if (map.isBlocked(position, GameplaySettings::get().healthPickupRadius) || map.isLava(position)) continue;
            *free = {true, position, GameplaySettings::get().healthPickupLifetime};
            return;
        }
    }
};