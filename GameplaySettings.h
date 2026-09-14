#pragma once
#include "GameConfig.h"
#include <fstream>
#include <cstdio>
#include <cstring>
#include <cmath>

struct EnemyStats { float health=0, speed=0, contactDamage=0, radius=0; };
struct ProjectileStats { float damage=0, speed=0, lifetime=0, radius=0; };

// Loaded once before creating gameplay state. No fallback balancing values in code.
struct GameplaySettings {
    float health=0, speed=0, radius=0, invulnerability=0, attackInterval=0, roadMultiplier=0;
    EnemyStats enemies[4];
    float turretInterval=0;
    ProjectileStats playerProjectile, enemyProjectile, manualProjectile;
    float manualAttackInterval=0;
    float aoeDamage=0, aoeCooldown=0, attackMultiplier=0, minAttackInterval=0, pickupRadius=0;
    unsigned int initialTargets=0, maxTargets=0, killsPerDrop=0, targetsAdded=0;
    float firstLevelDuration=0, portalRadius=0;
    float healthPickupHeal=0, healthPickupInterval=0, healthPickupLifetime=0;
    float healthPickupSpawnRadius=0, healthPickupRadius=0, lavaDamagePerSecond=0;

    static GameplaySettings& data() { static GameplaySettings settings; return settings; }
    static const GameplaySettings& get() { return data(); }
    static bool load(const char* path) {
        GameplaySettings pending;
        struct Field {
            const char* name;
            float* scalar;
            unsigned int* integer;
            float minimum, maximum;
            bool seen=false;
        };
        // Limits protect existing movement, fixed pools and finite arithmetic.
        Field fields[] = {
            {"level1.duration", &pending.firstLevelDuration, nullptr, 1, 3600},
            {"portal.radius", &pending.portalRadius, nullptr, 1, 128},
            {"player.health", &pending.health, nullptr, 1, 1000000},
            {"player.speed", &pending.speed, nullptr, 0, 2000},
            {"player.radius", &pending.radius, nullptr, 1, 16},
            {"player.invulnerability", &pending.invulnerability, nullptr, 0, 60},
            {"player.attack_interval", &pending.attackInterval, nullptr, 0.01f, 60},
            {"player.road_multiplier", &pending.roadMultiplier, nullptr, 1, 10},
            {"goblin.health", &pending.enemies[0].health, nullptr, 1, 1000000},
            {"goblin.speed", &pending.enemies[0].speed, nullptr, 0, 2000},
            {"goblin.contact_damage", &pending.enemies[0].contactDamage, nullptr, 0, 1000000},
            {"goblin.radius", &pending.enemies[0].radius, nullptr, 1, 128},
            {"sprinter.health", &pending.enemies[1].health, nullptr, 1, 1000000},
            {"sprinter.speed", &pending.enemies[1].speed, nullptr, 0, 2000},
            {"sprinter.contact_damage", &pending.enemies[1].contactDamage, nullptr, 0, 1000000},
            {"sprinter.radius", &pending.enemies[1].radius, nullptr, 1, 128},
            {"brute.health", &pending.enemies[2].health, nullptr, 1, 1000000},
            {"brute.speed", &pending.enemies[2].speed, nullptr, 0, 2000},
            {"brute.contact_damage", &pending.enemies[2].contactDamage, nullptr, 0, 1000000},
            {"brute.radius", &pending.enemies[2].radius, nullptr, 1, 128},
            {"turret.health", &pending.enemies[3].health, nullptr, 1, 1000000},
            {"turret.speed", &pending.enemies[3].speed, nullptr, 0, 2000},
            {"turret.contact_damage", &pending.enemies[3].contactDamage, nullptr, 0, 1000000},
            {"turret.radius", &pending.enemies[3].radius, nullptr, 1, 128},
            {"turret.attack_interval", &pending.turretInterval, nullptr, 0.01f, 60},
            {"player_projectile.damage", &pending.playerProjectile.damage, nullptr, 0, 1000000},
            {"player_projectile.speed", &pending.playerProjectile.speed, nullptr, 1, 10000},
            {"player_projectile.lifetime", &pending.playerProjectile.lifetime, nullptr, 0.01f, 120},
            {"player_projectile.radius", &pending.playerProjectile.radius, nullptr, 0.5f, 128},
            {"enemy_projectile.damage", &pending.enemyProjectile.damage, nullptr, 0, 1000000},
            {"enemy_projectile.speed", &pending.enemyProjectile.speed, nullptr, 1, 10000},
            {"enemy_projectile.lifetime", &pending.enemyProjectile.lifetime, nullptr, 0.01f, 120},
            {"enemy_projectile.radius", &pending.enemyProjectile.radius, nullptr, 0.5f, 128},
            {"manual_projectile.damage", &pending.manualProjectile.damage, nullptr, 0, 1000000},
            {"manual_projectile.speed", &pending.manualProjectile.speed, nullptr, 1, 10000},
            {"manual_projectile.lifetime", &pending.manualProjectile.lifetime, nullptr, 0.01f, 120},
            {"manual_projectile.radius", &pending.manualProjectile.radius, nullptr, 0.5f, 128},
            {"manual_projectile.attack_interval", &pending.manualAttackInterval, nullptr, 0.01f, 60},
            {"aoe.damage", &pending.aoeDamage, nullptr, 0, 1000000},
            {"aoe.cooldown", &pending.aoeCooldown, nullptr, 0.01f, 600},
            {"aoe.initial_targets", nullptr, &pending.initialTargets, 1, float(GameConfig::MaxAoeTargets)},
            {"aoe.max_targets", nullptr, &pending.maxTargets, 1, float(GameConfig::MaxAoeTargets)},
            {"upgrade.kills_per_drop", nullptr, &pending.killsPerDrop, 1, 1000000},
            {"upgrade.attack_interval_multiplier", &pending.attackMultiplier, nullptr, 0.01f, 1},
            {"upgrade.min_attack_interval", &pending.minAttackInterval, nullptr, 0.01f, 60},
            {"upgrade.aoe_targets_added", nullptr, &pending.targetsAdded, 1, float(GameConfig::MaxAoeTargets)},
            {"upgrade.pickup_radius", &pending.pickupRadius, nullptr, 1, 512},
            {"health_pickup.heal", &pending.healthPickupHeal, nullptr, 1, 1000000},
            {"health_pickup.spawn_interval", &pending.healthPickupInterval, nullptr, 0.1f, 600},
            {"health_pickup.lifetime", &pending.healthPickupLifetime, nullptr, 0.1f, 600},
            {"health_pickup.spawn_radius", &pending.healthPickupSpawnRadius, nullptr, 1, 5000},
            {"health_pickup.pickup_radius", &pending.healthPickupRadius, nullptr, 1, 128},
            {"lava.damage_per_second", &pending.lavaDamagePerSecond, nullptr, 0, 1000000}
        };
        std::ifstream file(path);
        if (!file) return fail("Cannot open Resources/gameplay.txt");
        char line[256]; unsigned int lineNumber=0;
        while (file.getline(line, sizeof(line))) {
            ++lineNumber;
            char* comment=std::strchr(line, '#'); if (comment) *comment=0;
            char key[80], extra; float value;
            const int count=sscanf_s(line, "%79s %f %c", key, unsigned(sizeof(key)), &value, &extra, 1u);
            if (count <= 0) continue;
            if (count != 2) { std::cerr << "Line " << lineNumber << '\n'; return fail("Expected property and value"); }
            Field* selected=nullptr;
            for (Field& field:fields) if (!std::strcmp(key,field.name)) { selected=&field; break; }
            if (!selected || selected->seen || !std::isfinite(value) || value<selected->minimum
                || value>selected->maximum || (selected->integer && std::floor(value)!=value)) {
                std::snprintf(errorText(),128,"Invalid or duplicate property: %s",key);
                return fail(errorText());
            }
            if (selected->scalar) *selected->scalar=value;
            else *selected->integer=static_cast<unsigned int>(value);
            selected->seen=true;
        }
        if (!file.eof()) return fail("Gameplay configuration line too long");
        for (const Field& field:fields) if (!field.seen) {
            std::snprintf(errorText(),128,"Missing property: %s",field.name); return fail(errorText());
        }
        if (pending.initialTargets>pending.maxTargets || pending.minAttackInterval>pending.attackInterval)
            return fail("Invalid AOE target or attack interval limits");
        data()=pending;
        return true;
    }
    static const char* error() { return errorText(); }
private:
    static char* errorText() { static char text[128]={}; return text; }
    static bool fail(const char* message) {
        if (message != errorText()) std::snprintf(errorText(),128,"%s",message);
        std::cerr << errorText() << '\n'; return false;
    }
};
