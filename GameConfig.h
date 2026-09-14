#pragma once

#include <cassert>
#include <iostream>

// The course header can use its own standard library code.
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

    constexpr unsigned int MaxAoeTargets = MaxEnemies;
}

enum class GameState { Menu, Playing, Paused, GameOver, Victory };

// Assertions only show coding mistakes when using Debug mode.
// A full array still needs normal failure handling in every build mode.
#ifndef NDEBUG
#define GAME_ASSERT(condition) assert(condition)
#define GAME_DEBUG_LOG(message) do { std::clog << "[Debug] " << message << '\n'; } while (false)
#else
#define GAME_ASSERT(condition) ((void)0)
#define GAME_DEBUG_LOG(message) ((void)0)
#endif
