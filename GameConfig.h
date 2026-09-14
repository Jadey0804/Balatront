#pragma once

#include <cassert>
#include <iostream>

// Course header internals are exempt from the student-code STL restriction.
namespace GameConfig {
    constexpr unsigned int WindowWidth = 1024;
    constexpr unsigned int WindowHeight = 768;
    constexpr unsigned int MaxEnemies = 256;
    constexpr float InfiniteReclaimDistance = 2048.0f;
    constexpr unsigned int MaxPlayerProjectiles = 512;
    constexpr unsigned int MaxEnemyProjectiles = 256;
    constexpr unsigned int MaxManualProjectiles = 128;
    constexpr unsigned int MaxHealthPickups = 16;
    constexpr unsigned int MaxMapWidth = 80;
    constexpr unsigned int MaxMapHeight = 60;
    constexpr unsigned int TileSize = 32;

    constexpr unsigned int CardCount = 8;
    constexpr unsigned int MaxOwnedCards = 16;
    // Provisional storage ceiling; gameplay balancing belongs to phase 4.
    constexpr unsigned int MaxAoeTargets = MaxEnemies;
}

// Phase 0 declares the vocabulary only. Transitions belong to later phases.
enum class GameState { Menu, Playing, Paused, Shop, GameOver, Victory };

// Assertions diagnose programmer errors only. Capacity exhaustion must return
// failure at the call site in every configuration, never rely on an assertion.
#ifndef NDEBUG
#define GAME_ASSERT(condition) assert(condition)
#define GAME_DEBUG_LOG(message) do { std::clog << "[Debug] " << message << '\n'; } while (false)
#else
#define GAME_ASSERT(condition) ((void)0)
#define GAME_DEBUG_LOG(message) ((void)0)
#endif
