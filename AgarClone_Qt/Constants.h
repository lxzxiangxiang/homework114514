#pragma once

#include <cstdint>

// ============================================================================
// EffectType — 统一技能/减益/危险物类型
// ============================================================================
enum class EffectType : uint8_t {
    None = 0,
    // 正面效果
    Speed,
    Shield,
    Grow,
    Invisible,
    Magnet,
    // 负面效果
    Bomb,
    Trap,
    Poison,
};

inline bool isBuff(EffectType e)  { return e >= EffectType::Speed && e <= EffectType::Magnet; }
inline bool isDebuff(EffectType e) { return e >= EffectType::Bomb  && e <= EffectType::Poison; }

namespace GameConstants {

// ==========================================================================
// Window
// ==========================================================================
namespace Window {
    inline constexpr int WIDTH  = 1280;
    inline constexpr int HEIGHT = 720;
}

// ==========================================================================
// World
// ==========================================================================
namespace World {
    inline constexpr int MAP_WIDTH  = 5000;
    inline constexpr int MAP_HEIGHT = 5000;
    inline constexpr float MIN_RADIUS = 10.0f;
    inline constexpr float MAX_RADIUS = 300.0f;  // 假设屏幕容纳上限
}

// ==========================================================================
// Loop
// ==========================================================================
namespace Loop {
    inline constexpr int FRAME_INTERVAL_MS = 16;
}

// ==========================================================================
// Ball
// ==========================================================================
namespace Ball {
    using namespace World;
    inline constexpr float BASE_SPEED = 300.0f;   // 公式: speed = BASE × √(MIN_RADIUS / radius())
    inline constexpr float EAT_RATIO = 1.1f;      // 公式: r_eater > r_prey × 1.1 才可吞食
    inline constexpr float SPLIT_THRESHOLD = 18.0f; // 可分裂的最小半径
    inline constexpr float EJECT_THRESHOLD = 25.0f; // 可吐孢的最小半径
    inline constexpr float SPLIT_MASS_RETAIN = 0.5f; // 分裂保留质量比例
    inline constexpr float SPLIT_RANDOM_ANGLE = 0.26f; // 分裂方向随机偏差 (±15°)

    // 速度调节
    inline constexpr float SPEED_MULTIPLIER = 1.5f;  // Speed 技能加速
    inline constexpr float TRAP_SPEED_MULTIPLIER = 0.5f; // Trap 减速

    // Grow 技能
    namespace Grow {
        inline constexpr float RADIUS_MULTIPLIER = 1.3f;  // 公式: r' = r × 1.3
        // 质量乘数 = 1.3² = 1.69
    }

    // Bomb debuff
    namespace Bomb {
        inline constexpr float RADIUS_RATIO = 0.85f;   // 爆炸后半径比例
    }

    // Poison debuff
    namespace Poison {
        inline constexpr float RADIUS_PER_SEC = 2.0f;  // 每秒减少半径
    }
}

// ==========================================================================
// Effect 持续时长
// ==========================================================================
namespace EffectDuration {
    inline constexpr float SPEED     = 5.0f;
    inline constexpr float SHIELD    = 3.0f;
    inline constexpr float GROW      = 4.0f;
    inline constexpr float INVISIBLE = 6.0f;
    inline constexpr float MAGNET    = 5.0f;
    inline constexpr float TRAP      = 3.0f;
    inline constexpr float POISON    = 4.0f;
}

// ==========================================================================
// Entity 半径
// ==========================================================================
namespace EntityRadius {
    inline constexpr float FOOD_MIN     = 3.0f;
    inline constexpr float FOOD_MAX     = 8.0f;
    inline constexpr float EJECTBALL    = 8.0f;
    inline constexpr float EFFECTBALL   = 12.0f;
    inline constexpr float HAZARD       = 60.0f;
}

// ==========================================================================
// Spawning
// ==========================================================================
namespace Spawning {
    inline constexpr int MAX_FOOD       = 500;
    inline constexpr int MAX_EFFECT     = 50;
    inline constexpr int MAX_HAZARD     = 20;
    inline constexpr int AIBALL_COUNT   = 50;
    inline constexpr int MAX_BALLS_PER_AI = 15;
}

// ==========================================================================
// Physics
// ==========================================================================
namespace Physics {
    inline constexpr float ATTRACTION_BASE = 30.0f;            // 同源球吸引力基数
    inline constexpr float ATTRACTION_DIST_FACTOR = 0.002f;   // 距离系数
    inline constexpr float ATTRACTION_DIST_EXPONENT = 2.5f;   // 距离指数
    inline constexpr float ATTRACTION_TIME_FACTOR = 0.5f;     // 时间增量系数(每秒)
    inline constexpr float MAGNET_RANGE_MULTIPLIER = 4.0f;  // Magnet 技能: 吸引力范围 = r ×4
    inline constexpr float MAGNET_FORCE = 200.0f;           // Magnet 单位力
    inline constexpr float SPLIT_ANIM_DURATION = 0.25f;     // 分裂缓动动画时长(秒)
}

// ==========================================================================
// AI
// ==========================================================================
namespace AI {
    namespace Level1 {
        inline constexpr float REACTION_TIME  = 0.5f;
        inline constexpr float DECISION_TIME  = 0.4f;
        inline constexpr float TURN_RATE      = 2.0f;
    }
    namespace Level2 {
        inline constexpr float REACTION_TIME  = 0.3f;
        inline constexpr float DECISION_TIME  = 0.2f;
        inline constexpr float TURN_RATE      = 3.0f;
    }
    namespace Level3 {
        inline constexpr float REACTION_TIME  = 0.15f;
        inline constexpr float DECISION_TIME  = 0.1f;
        inline constexpr float TURN_RATE      = 5.0f;
    }

    inline constexpr int THREAT_RANGE_MULTIPLIER = 6; // 威胁感知范围 = r ×6
    inline constexpr float SPLIT_DIST_NORMAL  = 4.0f;
    inline constexpr float SPLIT_DIST_AGGRESSIVE = 5.0f;
    inline constexpr float PREY_VALUE_MULTIPLIER = 0.9f;
    inline constexpr float TARGET_SKIP_CORNER_RANGE = 8;
    inline constexpr float PREY_MIN_CORNER_DIST = 3;
}

// ==========================================================================
// Camera
// ==========================================================================
namespace Camera {
    inline constexpr float ZOOM_MIN  = 0.2f;
    inline constexpr float ZOOM_MAX  = 2.0f;
    inline constexpr float ZOOM_LERP = 0.15f;
    inline constexpr float CENTER_LERP = 0.2f;
}

// ==========================================================================
// Gameplay
// ==========================================================================
namespace Gameplay {
    inline constexpr float VICTORY_TOTAL_RADIUS = 500.0f;
    inline constexpr int SPLIT_DELAY_MS = 100;  // 分裂防连发
}

// ==========================================================================
// HUD
// ==========================================================================
namespace HUD {
    inline constexpr int MARGIN = 10;
    inline constexpr int LINE_HEIGHT = 20;
    inline constexpr int MAX_WIDTH = 300;
    inline constexpr int FONT_SIZE = 12;
}

} // namespace GameConstants
