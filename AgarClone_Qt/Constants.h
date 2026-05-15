#pragma once

#include <cstdint>

enum class SkillType : uint8_t {
    None = 0,
    Speed,
    Shield,
    Grow,
    Invisible,
    Magnet
};

enum class DebuffType : uint8_t {
    None = 0,
    Bomb,
    Trap,
    Poison
};

namespace GameConstants {

inline constexpr int WINDOW_WIDTH  = 1280;
inline constexpr int WINDOW_HEIGHT = 720;

inline constexpr int MAP_WIDTH  = 5000;
inline constexpr int MAP_HEIGHT = 5000;

inline constexpr int FRAME_INTERVAL_MS = 16;

inline constexpr float BASE_SPEED = 300.0f;

inline constexpr float MIN_RADIUS = 10.0f;
inline constexpr float MAX_RADIUS = 300.0f;

inline constexpr float EAT_RATIO = 1.1f;

inline constexpr float SPLIT_THRESHOLD  = 18.0f;
inline constexpr float EJECT_THRESHOLD  = 25.0f;

inline constexpr float FOOD_RADIUS_MIN = 3.0f;
inline constexpr float FOOD_RADIUS_MAX = 8.0f;

inline constexpr float SKILLBALL_RADIUS = 12.0f;

inline constexpr float EJECTBALL_RADIUS = 8.0f;

inline constexpr float HAZARD_RADIUS = 50.0f;

inline constexpr int MAX_FOOD_COUNT     = 500;
inline constexpr int MAX_HAZARD_COUNT   = 10;
inline constexpr int MAX_SKILLBALL_COUNT = 20;

// Skill durations in seconds
namespace SkillDuration {
    inline constexpr float SPEED     = 5.0f;
    inline constexpr float SHIELD    = 3.0f;
    inline constexpr float GROW      = 4.0f;
    inline constexpr float INVISIBLE = 6.0f;
    inline constexpr float MAGNET    = 5.0f;
}

// Hazard / debuff effects
namespace HazardEffect {
    // Bomb: instant -15% radius
    inline constexpr float BOMB_RADIUS_RATIO = 0.85f;

    // Trap: speed x0.5 for 3 seconds
    inline constexpr float TRAP_SPEED_MULTIPLIER = 0.5f;
    inline constexpr float TRAP_DURATION         = 3.0f;

    // Poison: -2 radius per second for 4 seconds
    inline constexpr float POISON_RADIUS_PER_SEC = 2.0f;
    inline constexpr float POISON_DURATION       = 4.0f;
}

// AI level parameters
namespace AI {

    namespace Level1 {
        inline constexpr float REACTION_TIME  = 0.5f;
        inline constexpr float DECISION_TIME  = 0.4f;
        inline constexpr float TURN_RATE      = 2.0f;
        inline constexpr bool  CAN_SPLIT      = false;
        inline constexpr bool  AGGRESSIVE     = false;
    }

    namespace Level2 {
        inline constexpr float REACTION_TIME  = 0.3f;
        inline constexpr float DECISION_TIME  = 0.2f;
        inline constexpr float TURN_RATE      = 3.0f;
        inline constexpr bool  CAN_SPLIT      = true;
        inline constexpr bool  AGGRESSIVE     = false;
    }

    namespace Level3 {
        inline constexpr float REACTION_TIME  = 0.15f;
        inline constexpr float DECISION_TIME  = 0.1f;
        inline constexpr float TURN_RATE      = 5.0f;
        inline constexpr bool  CAN_SPLIT      = true;
        inline constexpr bool  AGGRESSIVE     = true;
    }

} // namespace AI

inline constexpr int SPATIAL_GRID_CELL_SIZE = 200;

inline constexpr float CAMERA_ZOOM_MIN = 0.5f;
inline constexpr float CAMERA_ZOOM_MAX = 1.5f;

inline constexpr float VICTORY_TOTAL_RADIUS = 2000.0f;

} // namespace GameConstants
