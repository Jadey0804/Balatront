#pragma once
#include "Game.h"
#include <cstdint>
#include <cstdio>
#include <fstream>

// Save normal values one by one and do not save memory address.
class SaveGame {
    struct Snapshot {
        PlaySession session;
        LevelFlow level;
        EnemyManager enemies;
        PlayerProjectilePool playerShots;
        EnemyProjectilePool enemyShots;
        ManualProjectilePool manualShots;
        Combat combat;
        HealthPickupManager healthPickups;
        float shootCooldown = 0, manualShootCooldown = 0;
        std::uint32_t mapSeed = 1, nextSeed = 1;
        unsigned int width = 0, height = 0;
        unsigned char cells[GameConfig::MaxMapWidth * GameConfig::MaxMapHeight] = {};
    };
    // Check every basic value before putting it back into the game.
    class Archive {
        std::istream* input;
        std::ostream* output;
    public:
        bool valid = true;
        std::uint32_t hash = 2166136261u;
        Archive(std::istream* in, std::ostream* out) : input(in), output(out) {}
        void bytes(void* data, unsigned int size) {
            if (!valid) return;
            if (input) valid = bool(input->read(static_cast<char*>(data), size));
            else valid = bool(output->write(static_cast<const char*>(data), size));
            if (!valid) return;
            const unsigned char* p = static_cast<const unsigned char*>(data);
            for (unsigned int i = 0; i < size; ++i) { hash ^= p[i]; hash *= 16777619u; }
        }
        void value(unsigned int& v) { static_assert(sizeof(v) == 4); bytes(&v, 4); }
        void value(float& v) {
            static_assert(sizeof(v) == 4);
            bytes(&v, 4);
            valid = valid && std::isfinite(v) && std::fabs(v) <= 10000000.0f;
        }
        void value(bool& v) {
            unsigned int n = v ? 1u : 0u; value(n);
            if (n > 1) valid = false;
            if (valid) v = n != 0;
        }
        template<class T> void enumeration(T& v, unsigned int count) {
            unsigned int n = static_cast<unsigned int>(v); value(n);
            if (n >= count) valid = false;
            if (valid) v = static_cast<T>(n);
        }
        void vector(Vector2& v) { value(v.x); value(v.y); }
        void constant(unsigned int expected) {
            unsigned int n = expected; value(n); if (n != expected) valid = false;
        }
    };
private:
    template<unsigned int Capacity>
    static void projectiles(Archive& a, ProjectilePool<Capacity>& pool) {
        a.constant(Capacity);
        a.value(pool.searchStart); a.value(pool.active); a.value(pool.skipped);
        unsigned int active = 0;
        for (Projectile& p : pool.shots) {
            a.value(p.active); a.vector(p.position); a.vector(p.velocity);
            a.value(p.remainingLife); a.value(p.damage); a.value(p.radius);
            a.vector(p.previousPosition); a.value(p.motionFraction);
            if (p.active) {
                ++active;
                if (p.remainingLife <= 0 || p.damage < 0 || p.radius <= 0 || p.radius > 128
                    || p.motionFraction < 0 || p.motionFraction > 1) a.valid = false;
            }
        }
        if (pool.searchStart >= Capacity || pool.active != active) a.valid = false;
    }
    static void serialize(Archive& a, Snapshot& s) {
        a.constant(0x42545356u); a.constant(4); // This number tells which save file format is used.
        a.enumeration(s.session.state, 5); a.value(s.session.elapsed);
        a.vector(s.session.camera.position); a.enumeration(s.session.camera.mode, 2);
        Player& p = s.session.player;
        a.vector(p.position); a.vector(p.previousPosition); a.vector(p.velocity);
        a.value(p.health); a.value(p.speed); a.value(p.radius); a.value(p.invulnerability);
        a.value(p.damageFeedback); a.value(p.damageAnimationTime);
        a.value(p.attackInterval); a.value(p.aoeTargets);
        a.value(s.shootCooldown); a.value(s.manualShootCooldown);
        a.value(s.level.number); a.value(s.level.portalOpen); a.value(s.level.portalTime);
        a.vector(s.level.portalPosition); a.value(s.mapSeed); a.value(s.nextSeed);
        a.value(s.width); a.value(s.height);
        if (!s.width || !s.height || s.width > GameConfig::MaxMapWidth || s.height > GameConfig::MaxMapHeight) {
            a.valid = false; return;
        }
        for (unsigned int i = 0; i < s.width * s.height; ++i) {
            unsigned int tile = s.cells[i]; a.value(tile);
            if (tile >= TileMap::ImageCount) a.valid = false;
            if (a.valid) s.cells[i] = static_cast<unsigned char>(tile);
        }
        EnemyManager& m = s.enemies;
        a.constant(GameConfig::MaxEnemies);
        a.value(m.accumulator); a.value(m.randomState);
        a.value(m.insideCount); a.value(m.outsideCount); a.value(m.capacitySkips); a.value(m.locationSkips);
        for (unsigned int& count : m.typeCounts) a.value(count);
        for (Enemy& e : m.enemies) {
            a.value(e.active); a.enumeration(e.type, EnemyConfig::TypeCount);
            a.vector(e.position); a.value(e.health); a.value(e.spawnedInside);
            a.value(e.shootCooldown); a.vector(e.previousPosition); a.value(e.hitFlash);
            a.value(e.dying); a.value(e.faceLeft); a.enumeration(e.animation, 4); a.value(e.animationTime);
            if ((e.active && (e.dying || e.health <= 0)) || e.health < 0 || e.hitFlash < 0
                || e.animationTime < 0 || e.shootCooldown < 0) a.valid = false;
        }
        projectiles(a, s.playerShots); projectiles(a, s.enemyShots); projectiles(a, s.manualShots);
        Combat& c = s.combat;
        a.value(c.aoeCooldown); a.value(c.feedbackTime); a.value(c.targetCount);
        a.value(c.kills); a.value(c.upgradesCollected);
        a.constant(GameConfig::MaxAoeTargets);
        for (Vector2& v : c.targetPositions) a.vector(v);
        a.constant(CombatConfig::MaxPickups);
        for (UpgradePickup& pickup : c.pickups) {
            a.value(pickup.active); a.vector(pickup.position); a.value(pickup.attackSpeed);
        }
        a.value(c.skippedPickups); a.value(c.upgradeFeedback); a.value(c.lastUpgradeSpeed);
        a.constant(CombatConfig::MaxHitEffects);
        for (HitEffect& hit : c.hitEffects) { a.vector(hit.position); a.value(hit.remaining); }
        a.value(c.nextHitEffect);
        HealthPickupManager& h = s.healthPickups;
        a.constant(GameConfig::MaxHealthPickups);
        for (HealthPickup& pickup : h.pickups) {
            a.value(pickup.active); a.vector(pickup.position); a.value(pickup.remaining);
            if (pickup.active && pickup.remaining <= 0) a.valid = false;
        }
        a.value(h.spawnTimer); a.value(h.randomState);
        if ((s.session.state != GameState::Playing && s.session.state != GameState::Paused)
            || s.session.elapsed < 0 || p.health <= 0 || p.health > GameplaySettings::get().health
            || p.speed < 0 || p.speed > 2000 || p.radius <= 0 || p.radius > 16 || p.invulnerability < 0
            || p.damageFeedback < 0 || p.damageAnimationTime < 0
            || p.attackInterval < 0.01f || p.attackInterval > 60 || !p.aoeTargets
            || p.aoeTargets > GameplaySettings::get().maxTargets || s.shootCooldown < 0 || s.manualShootCooldown < 0
            || (s.level.number != 1 && s.level.number != 2) || s.level.portalTime < 0
            || (s.level.number == 1) != (s.session.camera.mode == CameraMode::Fixed)
            || (s.level.number == 2 && s.level.portalOpen)
            || m.accumulator < 0 || c.aoeCooldown < 0 || c.feedbackTime < 0
            || c.targetCount > GameConfig::MaxAoeTargets || c.upgradeFeedback < 0
            || c.nextHitEffect >= CombatConfig::MaxHitEffects || h.spawnTimer < 0) a.valid = false;
        if (s.level.number == 1 && (p.position.x < p.radius || p.position.y < p.radius
            || p.position.x > s.width * GameConfig::TileSize - p.radius
            || p.position.y > s.height * GameConfig::TileSize - p.radius)) a.valid = false;
    }
public:
    static const char* save(Game& game) {
        if (game.session.state != GameState::Playing && game.session.state != GameState::Paused)
            return "SAVE FAILED - START A GAME FIRST";
        Snapshot* s = new Snapshot;
        s->session = game.session; s->level = game.level; s->enemies = game.enemies;
        s->playerShots = game.playerShots; s->enemyShots = game.enemyShots; s->manualShots = game.manualShots;
        s->combat = game.combat; s->healthPickups = game.healthPickups;
        s->shootCooldown = game.playerShootCooldown; s->manualShootCooldown = game.manualShootCooldown;
        s->mapSeed = game.tileMap.procedural.seed; s->nextSeed = game.nextMapSeed;
        s->width = game.tileMap.width; s->height = game.tileMap.height;
        for (unsigned int i = 0; i < s->width * s->height; ++i) s->cells[i] = game.tileMap.cells[i];
        std::ofstream file("savegame.tmp", std::ios::binary | std::ios::trunc);
        Archive a(nullptr, &file);
        serialize(a, *s);
        const std::uint32_t checksum = a.hash;
        file.write(reinterpret_cast<const char*>(&checksum), sizeof(checksum));
        file.flush(); bool good = a.valid && bool(file); file.close(); good = good && !file.fail();
        delete s;
        if (!good) { std::remove("savegame.tmp"); return "SAVE FAILED - WRITE OR STATE ERROR"; }
        // Keep the old save first so a stopped writing will not destroy it.
        std::ifstream existing("savegame.dat", std::ios::binary);
        const bool hadSave = bool(existing); existing.close();
        if (hadSave) {
            std::remove("savegame.bak");
            if (std::rename("savegame.dat", "savegame.bak") != 0) return "SAVE FAILED - FILE IN USE";
        }
        if (std::rename("savegame.tmp", "savegame.dat") != 0) {
            if (hadSave) std::rename("savegame.bak", "savegame.dat");
            return "SAVE FAILED - CANNOT REPLACE FILE";
        }
        return "SAVED";
    }
    static const char* load(Game& game) {
        std::ifstream file("savegame.dat", std::ios::binary);
        if (!file) return "LOAD FAILED - NO SAVE FILE";
        Snapshot* s = new Snapshot;
        Archive a(&file, nullptr); serialize(a, *s);
        std::uint32_t checksum = 0;
        file.read(reinterpret_cast<char*>(&checksum), sizeof(checksum));
        const bool good = a.valid && bool(file) && checksum == a.hash
            && file.peek() == std::char_traits<char>::eof();
        if (!good) {
            delete s;
            return "LOAD FAILED - INVALID SAVE OR VERSION";
        }
        game.session = s->session; game.level = s->level;
        game.enemies = s->enemies; game.enemySprites.configure(game.enemies);
        game.playerShots = s->playerShots; game.enemyShots = s->enemyShots; game.manualShots = s->manualShots;
        game.combat = s->combat; game.healthPickups = s->healthPickups;
        game.playerShootCooldown = s->shootCooldown; game.manualShootCooldown = s->manualShootCooldown;
        game.tileMap.width = s->width; game.tileMap.height = s->height;
        for (unsigned int i = 0; i < s->width * s->height; ++i) game.tileMap.cells[i] = s->cells[i];
        game.tileMap.procedural.seed = s->mapSeed; game.nextMapSeed = s->nextSeed;
        game.worldSize = game.tileMap.size();
        game.selectedMode = game.tileMap.mode = game.session.camera.mode;
        game.session.state = GameState::Paused;
        delete s;
        return "LOADED - ESC OR ENTER TO RESUME";
    }
};
