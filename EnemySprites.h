#pragma once
#include "Enemies.h"
#include "GamesEngineeringBase.h"
#include <fstream>
#include <cstdio>
#include <cstring>

struct SpriteClip {
    GamesEngineeringBase::Image image;
    unsigned int width = 0, height = 0, frames = 0;
    float fps = 0, scale = 1;
    bool loop = false;
    float duration() const { return fps > 0 ? frames / fps : 0.15f; }
};

class EnemySprites {
public:
    static constexpr unsigned int MaxClips = 24;
    EnemySprites() {
        for (auto& type : lookup) for (int& index : type) index = -1;
    }
    bool load(const char* path) {
        std::ifstream file(path);
        if (!file) return fail("Cannot open Sprites/sprites.txt");
        char line[2048];
        unsigned int lineNumber = 0;
        while (file.getline(line, sizeof(line))) {
            ++lineNumber;
            const char* first = line;
            while (*first == ' ' || *first == '\t' || *first == '\r') ++first;
            if (!*first || *first == '#') continue;
            char type[32], action[32], imagePath[512], extra;
            unsigned int w, h, frames;
            float fps, scale;
            int loop;
            const int fields = sscanf_s(first, "%31s %31s \"%511[^\"]\" %u %u %u %f %f %d %c",
                type, unsigned(sizeof(type)), action, unsigned(sizeof(action)), imagePath, unsigned(sizeof(imagePath)),
                &w, &h, &frames, &fps, &scale, &loop, &extra, 1u);
            if (fields != 9 || count == MaxClips || !w || !h || !frames || frames > 1024
                || !std::isfinite(fps) || !std::isfinite(scale) || scale <= 0 || scale > 16
                || fps < 0 || (frames > 1 && fps <= 0) || (loop != 0 && loop != 1)) {
                std::cerr << "Invalid sprite row " << lineNumber << '\n'; return fail("Invalid sprite configuration");
            }
            int group = -1, state = -1;
            if (!std::strcmp(type,"goblin")) group = 0;
            else if (!std::strcmp(type,"sprinter")) group = 1;
            else if (!std::strcmp(type,"brute")) group = 2;
            else if (!std::strcmp(type,"turretBody")) group = 3;
            else if (!std::strcmp(type,"turretPipe")) group = 4;
            else if (!std::strcmp(type,"portal")) group = 5;
            else if (!std::strcmp(type,"aoe")) group = 6;
            else if (!std::strcmp(type,"manualBomb")) group = 7;
            else if (!std::strcmp(type,"healthPickup")) group = 8;
            else if (!std::strcmp(type,"bombHit")) group = 9;
            if (!std::strcmp(action,"walk") || !std::strcmp(action,"fly") || !std::strcmp(action,"idle")) state = 0;
            else if (!std::strcmp(action,"attack")) state = 1;
            else if (!std::strcmp(action,"hurt")) state = 2;
            else if (!std::strcmp(action,"death")) state = 3;
            if (group < 0 || state < 0 || lookup[group][state] >= 0) return fail("Unknown or duplicate sprite action");
            char fullPath[560];
            std::snprintf(fullPath, sizeof(fullPath), "Resources/%s", imagePath);
            std::ifstream imageFile(fullPath, std::ios::binary);
            if (!imageFile) { std::cerr << fullPath << '\n'; return fail("Missing sprite image"); }
            imageFile.close();
            SpriteClip& clip = clips[count];
            if (!clip.image.load(fullPath)) {
                std::cerr << "Sprite row " << lineNumber << ": " << fullPath << '\n';
                std::snprintf(detail, sizeof(detail), "Cannot decode %s %s - use RGB or RGBA PNG", type, action);
                return fail(detail);
            }
            if (clip.image.width % w != 0 || clip.image.height % h != 0
                || (clip.image.width / w) * (clip.image.height / h) != frames) {
                std::cerr << "Sprite row " << lineNumber << ": " << fullPath << " actual "
                    << clip.image.width << 'x' << clip.image.height << ", configured frame " << w << 'x' << h
                    << " count " << frames << '\n';
                std::snprintf(detail, sizeof(detail), "Frame mismatch: %s %s - see console", type, action);
                return fail(detail);
            }
            clip.width = w; clip.height = h; clip.frames = frames;
            clip.fps = fps; clip.scale = scale; clip.loop = loop != 0;
            lookup[group][state] = int(count++);
        }
        if (!file.eof()) return fail("Sprite row is too long or unreadable");
        for (int group = 0; group < 10; ++group)
            if (lookup[group][0] < 0) return fail("Missing movement or idle sprite");
        return true;
    }
    const char* error() const { return errorMessage; }
    void drawAoe(GamesEngineeringBase::Window& canvas, Vector2 screen, float time) const {
        drawClip(canvas, clips[lookup[6][0]], screen, time, false, 0, false);
    }
    void drawManualBomb(GamesEngineeringBase::Window& canvas, Vector2 screen) const {
        drawClip(canvas, clips[lookup[7][0]], screen, 0, false, 0, false);
    }
    void drawHealthPickup(GamesEngineeringBase::Window& canvas, Vector2 screen) const {
        drawClip(canvas, clips[lookup[8][0]], screen, 0, false, 0, false);
    }
    void drawBombHit(GamesEngineeringBase::Window& canvas, Vector2 screen) const {
        drawClip(canvas, clips[lookup[9][0]], screen, 0, false, 0, false);
    }
    void drawPortal(GamesEngineeringBase::Window& canvas, Vector2 screen, float time) const {
        drawClip(canvas, clips[lookup[5][0]], screen, time, false, 0, false);
    }
    void configure(EnemyManager& enemies) const {
        for (unsigned int type = 0; type < EnemyConfig::TypeCount; ++type)
            for (unsigned int action = 0; action < 4; ++action) {
                const int index = lookup[type][action];
                enemies.animationDurations[type][action] = index < 0 ? 0 : clips[index].duration();
            }
    }
    void draw(GamesEngineeringBase::Window& canvas, const Enemy& enemy, Vector2 screen, Vector2 target) const {
        const int group = static_cast<int>(enemy.type);
        const int action = static_cast<int>(enemy.animation);
        const int index = lookup[group][action] >= 0 ? lookup[group][action] : lookup[group][0];
        bool flip = enemy.faceLeft && group != 3;
        // Brute's walk sheet faces left by default, unlike the other clips.
        if (enemy.type == EnemyType::Brute && index == lookup[group][0]) flip = !flip;
        drawClip(canvas, clips[index], screen, enemy.animationTime, flip, 0,
            enemy.active && enemy.hitFlash > 0 && lookup[group][2] < 0);
        if (group == 3 && enemy.active) {
            // The supplied pipe points down; its 16x16 canvas centre is the mounting pivot.
            const float angle = std::atan2(target.y-enemy.position.y, target.x-enemy.position.x) - 1.570796327f;
            drawClip(canvas, clips[lookup[4][0]], screen, 0, false, angle, false);
        }
    }
private:
    SpriteClip clips[MaxClips];
    int lookup[10][4];
    unsigned int count = 0;
    const char* errorMessage = "Sprite loading failed";
    char detail[128] = {};
    bool fail(const char* text) { errorMessage = text; std::cerr << text << '\n'; return false; }
    static void drawClip(GamesEngineeringBase::Window& canvas, const SpriteClip& clip, Vector2 centre,
        float time, bool flip, float angle, bool flash) {
        const float frameTime = clip.fps > 0 ? time * clip.fps : 0;
        const unsigned int frame = clip.loop ? unsigned(std::fmod(frameTime, float(clip.frames)))
            : unsigned(ClampValue(frameTime, 0, float(clip.frames-1)));
        const float c = std::cos(angle), s = std::sin(angle);
        const float halfW = clip.width * clip.scale * 0.5f, halfH = clip.height * clip.scale * 0.5f;
        const float extentX = std::fabs(c)*halfW + std::fabs(s)*halfH;
        const float extentY = std::fabs(s)*halfW + std::fabs(c)*halfH;
        const int left = int(ClampValue(std::floor(centre.x-extentX), 0, float(canvas.getWidth())));
        const int right = int(ClampValue(std::ceil(centre.x+extentX), 0, float(canvas.getWidth())));
        const int top = int(ClampValue(std::floor(centre.y-extentY), 0, float(canvas.getHeight())));
        const int bottom = int(ClampValue(std::ceil(centre.y+extentY), 0, float(canvas.getHeight())));
        for (int y = top; y < bottom; ++y)
            for (int x = left; x < right; ++x) {
                const float dx = x+0.5f-centre.x, dy = y+0.5f-centre.y;
                int sx = int(std::floor((c*dx+s*dy)/clip.scale + clip.width*0.5f));
                int sy = int(std::floor((-s*dx+c*dy)/clip.scale + clip.height*0.5f));
                if (sx < 0 || sy < 0 || sx >= int(clip.width) || sy >= int(clip.height)) continue;
                if (flip) sx = int(clip.width)-1-sx;
                const unsigned int columns = clip.image.width / clip.width;
                sx += (frame % columns) * clip.width;
                sy += (frame / columns) * clip.height;
                const unsigned int alpha = clip.image.alphaAtUnchecked(sx,sy);
                if (!alpha) continue;
                const auto* source = clip.image.atUnchecked(sx,sy);
                auto* destination = canvas.backBuffer() + (y*canvas.getWidth()+x)*3;
                for (int channel=0; channel<3; ++channel)
                    destination[channel] = static_cast<unsigned char>(((flash ? 255 : source[channel])*alpha
                        + destination[channel]*(255-alpha))/255);
            }
    }
};
