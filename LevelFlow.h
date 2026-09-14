#pragma once
#include "Combat.h"

struct LevelFlow {
    unsigned int number = 1;
    bool portalOpen = false;
    float portalTime = 0;
    Vector2 portalPosition;

    void reset(Vector2 fixedWorldSize) {
        number = 1;
        portalOpen = false;
        portalTime = 0;
        portalPosition = {fixedWorldSize.x * 0.5f, fixedWorldSize.y * 0.5f};
    }
    bool update(const PlaySession& session, float dt) {
        if (number != 1 || session.state != GameState::Playing || dt <= 0) return false;
        if (!portalOpen) {
            if (session.elapsed >= GameplaySettings::get().firstLevelDuration) portalOpen = true;
            return false; // Never count movement from before the portal appeared.
        }
        portalTime += dt;
        float fraction;
        return CircleSweep(session.player.previousPosition, session.player.position, portalPosition,
            portalPosition, session.player.radius + GameplaySettings::get().portalRadius, fraction);
    }
};
