#pragma once
#include "GamesEngineeringBase.h"
#include "Gameplay.h"
#include "ProceduralMap.h"
#include <fstream>
#include <iomanip>
#include <cstring>
#include <cstdio>

class TileMap {
    friend class SaveGame;
public:
    static constexpr unsigned int ImageCount = 25;

    bool load(const char* path) {
        std::ifstream file(path);
        if (!file) return fail("Cannot open Resources/tiles.txt");
        unsigned int tileWidth = 0, tileHeight = 0, layer = 0;
        if (!field(file, "tileswide", width) || !field(file, "tileshigh", height)
            || !field(file, "tilewidth", tileWidth) || !field(file, "tileheight", tileHeight)
            || !field(file, "layer", layer)) return fail("Invalid map header");
        if (width == 0 || width > GameConfig::MaxMapWidth || height == 0 || height > GameConfig::MaxMapHeight
            || tileWidth != GameConfig::TileSize || tileHeight != GameConfig::TileSize || layer != 0)
            return fail("Unsupported map dimensions, tile size or layer");
        for (unsigned int i = 0; i < width * height; ++i) {
            int id = -1;
            if (!(file >> id) || id < 0 || id >= int(ImageCount)) return fail("Missing or invalid tile ID");
            cells[i] = static_cast<unsigned char>(id);
            file >> std::ws;
            if (file.peek() == ',') file.get();
            else if (i + 1 < width * height) return fail("Missing comma between tile IDs");
        }
        file >> std::ws;
        if (file.peek() != std::char_traits<char>::eof()) return fail("Unexpected extra map data");
        for (unsigned int i = 0; i < ImageCount; ++i) {
            char imagePath[64];
            std::snprintf(imagePath, sizeof(imagePath), "Resources/%u.png", i);
            std::ifstream imageFile(imagePath, std::ios::binary);
            if (!imageFile) return fail("Missing tile image in Resources");
            imageFile.close();
            if (!images[i].load(imagePath) || images[i].width != GameConfig::TileSize
                || images[i].height != GameConfig::TileSize) return fail("Tile images must be 32 by 32");
        }
        return true;
    }

    Vector2 size() const { return {float(width * GameConfig::TileSize), float(height * GameConfig::TileSize)}; }
    const char* error() const { return errorMessage; }
    CameraMode mode = CameraMode::Fixed;
    ProceduralMap procedural;

    bool isRoad(Vector2 position) const {
        const int tileSize = int(GameConfig::TileSize);
        const int col = int(std::floor(position.x / tileSize));
        const int row = int(std::floor(position.y / tileSize));
        if (mode == CameraMode::Fixed && (col < 0 || row < 0 || col >= int(width) || row >= int(height))) return false;
        const int x = int(std::floor(position.x)) - col * tileSize;
        const int y = int(std::floor(position.y)) - row * tileSize;
        const unsigned int id = mode == CameraMode::Infinite
            ? procedural.appearance(col, row).images[(y >= tileSize/2 ? 2 : 0) + (x >= tileSize/2 ? 1 : 0)]
            : tileAt(col, row);
        if (id == 0 || id == 24 || (id >= 14 && id <= 22)) return false;
        // Road pixels have more red and grass pixels have more green.
        // Read the same picture pixel that is shown on the screen.
        const unsigned char* pixel = images[id].atUnchecked(x, y);
        return pixel[0] > pixel[1] && pixel[0] > pixel[2];
    }

    bool isLava(Vector2 position) const {
        const int col = int(std::floor(position.x / GameConfig::TileSize));
        const int row = int(std::floor(position.y / GameConfig::TileSize));
        if (mode == CameraMode::Fixed && (col < 0 || row < 0 || col >= int(width) || row >= int(height))) return false;
        return tileAt(col, row) == 24;
    }

    bool isBlocked(Vector2 position, float radius) const { return blocked(position, radius); }

    bool findSpawn(float radius, Vector2& position) {
        position = mode == CameraMode::Infinite ? Vector2{16, 16} : Vector2{size().x * 0.5f, size().y * 0.5f};
        return placeOnLand(radius, position);
    }

    bool placeOnLand(float radius, Vector2& position) {
        if (!blocked(position, radius)) return true;
        const Vector2 centre = position;
        float nearest = -1;
        const int originX = int(std::floor(position.x / GameConfig::TileSize));
        const int originY = int(std::floor(position.y / GameConfig::TileSize));
        const int firstX = mode == CameraMode::Infinite ? originX - ProceduralMap::RegionSize : 0;
        const int firstY = mode == CameraMode::Infinite ? originY - ProceduralMap::RegionSize : 0;
        const int lastX = mode == CameraMode::Infinite ? originX + ProceduralMap::RegionSize : int(width) - 1;
        const int lastY = mode == CameraMode::Infinite ? originY + ProceduralMap::RegionSize : int(height) - 1;
        for (int row = firstY; row <= lastY; ++row)
            for (int col = firstX; col <= lastX; ++col) {
                const Vector2 candidate = {(col + 0.5f) * GameConfig::TileSize, (row + 0.5f) * GameConfig::TileSize};
                if (blocked(candidate, radius)) continue;
                const float dx = candidate.x - centre.x, dy = candidate.y - centre.y;
                const float distance = dx*dx + dy*dy;
                if (nearest < 0 || distance < nearest) { nearest = distance; position = candidate; }
            }
        return nearest >= 0 || fail("Map has no walkable player spawn");
    }

    void resolveMovement(Player& player, float dt) const {
        if (dt <= 0) return;
        const Vector2 desired = player.position;
        const Vector2 delta = {desired.x - player.previousPosition.x, desired.y - player.previousPosition.y};
        const float distance = std::sqrt(delta.x*delta.x + delta.y*delta.y);
       
        // Check two directions separately so the player can move beside water.
        const int steps = 1 + int(distance / (GameConfig::TileSize * 0.25f));
        const Vector2 step = {delta.x / steps, delta.y / steps};
        player.position = player.previousPosition;
        for (int i = 0; i < steps; ++i) {
            Vector2 next = {player.position.x + step.x, player.position.y};
            if (!blocked(next, player.radius)) player.position.x = next.x;
            next = {player.position.x, player.position.y + step.y};
            if (!blocked(next, player.radius)) player.position.y = next.y;
        }
        player.velocity = {(player.position.x - player.previousPosition.x) / dt,
            (player.position.y - player.previousPosition.y) / dt};
    }

    void draw(GamesEngineeringBase::Window& canvas, const Camera& camera) const {
        const int size = int(GameConfig::TileSize);
        const int cameraX = int(std::floor(camera.position.x)), cameraY = int(std::floor(camera.position.y));
        const int firstX = ProceduralMap::floorDivide(cameraX, size), firstY = ProceduralMap::floorDivide(cameraY, size);
        int lastX = ProceduralMap::floorDivide(cameraX + int(canvas.getWidth()) - 1, size);
        int lastY = ProceduralMap::floorDivide(cameraY + int(canvas.getHeight()) - 1, size);
        if (mode == CameraMode::Fixed) {
            lastX = int(ClampValue(float(lastX), 0, float(width - 1)));
            lastY = int(ClampValue(float(lastY), 0, float(height - 1)));
        }
        for (int row = firstY; row <= lastY; ++row)
            for (int col = firstX; col <= lastX; ++col) {
                TerrainTile appearance;
                if (mode == CameraMode::Infinite) appearance = procedural.appearance(col, row);
                else for (unsigned int& id : appearance.images) id = tileAt(col, row);
                const int left = col * size - cameraX, top = row * size - cameraY;
                const int beginX = left < 0 ? -left : 0, beginY = top < 0 ? -top : 0;
                const int endX = int(ClampValue(float(int(canvas.getWidth()) - left), 0, float(size)));
                const int endY = int(ClampValue(float(int(canvas.getHeight()) - top), 0, float(size)));
                for (int y = beginY; y < endY; ++y)
                    for (int x = beginX; x < endX; ++x)
                        canvas.draw(left + x, top + y,
                            images[appearance.images[(y >= size/2 ? 2 : 0) + (x >= size/2 ? 1 : 0)]].atUnchecked(x, y));
            }
    }

private:
    unsigned int width = 0, height = 0;
    unsigned char cells[GameConfig::MaxMapWidth * GameConfig::MaxMapHeight] = {};
    GamesEngineeringBase::Image images[ImageCount];
    const char* errorMessage = "Map loading failed";

    unsigned int tileAt(int col, int row) const {
        if (mode == CameraMode::Infinite) return procedural.tile(col, row);
        GAME_ASSERT(col >= 0 && row >= 0 && col < int(width) && row < int(height));
        return cells[row * width + col];
    }

    bool blocked(Vector2 position, float radius) const {
        const Vector2 bounds = size();
        if (mode == CameraMode::Fixed && (position.x - radius < 0 || position.y - radius < 0
            || position.x + radius > bounds.x || position.y + radius > bounds.y)) return true;
        const int tileSize = int(GameConfig::TileSize);
        const int firstX = int(std::floor((position.x - radius) / tileSize));
        const int firstY = int(std::floor((position.y - radius) / tileSize));
        int lastX = int(std::floor((position.x + radius) / tileSize));
        int lastY = int(std::floor((position.y + radius) / tileSize));
        if (mode == CameraMode::Fixed) {
            lastX = int(ClampValue(float(lastX), 0, float(width - 1)));
            lastY = int(ClampValue(float(lastY), 0, float(height - 1)));
        }
        for (int row = firstY; row <= lastY; ++row)
            for (int col = firstX; col <= lastX; ++col) {
                const unsigned int id = tileAt(col, row);
                // Picture numbers 14 to 22 are water and the water edge.
                if (id < 14 || id > 22) continue;
                const float x = ClampValue(position.x, float(col * tileSize), float((col + 1) * tileSize));
                const float y = ClampValue(position.y, float(row * tileSize), float((row + 1) * tileSize));
                const float dx = position.x - x, dy = position.y - y;
                if (dx*dx + dy*dy < radius*radius) return true;
            }
        return false;
    }

    bool fail(const char* message) { errorMessage = message; std::cerr << message << '\n'; return false; }
    static bool field(std::ifstream& file, const char* expected, unsigned int& value) {
        char name[32] = {};
        return bool(file >> std::setw(sizeof(name)) >> name >> value) && std::strcmp(name, expected) == 0;
    }
};