# AIController.cpp — AI 控制器(AIController)实现

## 文件概述
AI 控制器管理所有非玩家球体的自主行为。实现三级 AI 决策系统：巡逻、猎食、躲避、分裂，以及平滑转向。

## 涉及类/结构体
- `AIController::AIState` — AI 状态结构体（方向、决策计时器、巡逻目标）
- `AIController` — 静态方法类，维护 AI 状态映射表

## 辅助函数

### `clampLength(vec, maxLen)`
限制向量长度不超过 maxLen。

### `length(vec)`
计算向量长度（欧几里得距离）。

### `normalized(vec)`
向量归一化（长度 < 1e-6 时返回零向量）。

## 方法说明

### `void AIController::updateAI(Ball* ai, const QList<Ball*>& allBalls, const QList<Food*>& foods, qreal dt)`
AI 主决策函数。每帧调用一次。

**决策流程：**

1. **空指针/存活检查** — `!ai || !ai->isAlive()` 则返回

2. **决策计时器** — 递减 `decisionTimer`；归零时根据 AI 等级重置：
   - Level 1: 0.4s
   - Level 2: 0.2s
   - Level 3: 0.1s

3. **威胁检测** — 遍历所有球体，找 `radius > 自身×1.1(EAT_RATIO)` 且在 6×半径范围内的对手，朝反方向逃离（多个威胁矢量叠加）

4. **猎物检测** — 找 `radius < 自身×0.9` 的球体，按 `value = radius / (dist + 1)` 评分，朝最高分猎物追击

5. **巡逻** — 无威胁且无猎物时，在地图范围内随机生成巡逻点

6. **分裂决策（Level ≥ 2）** — 预留，待实现

7. **平滑转向** — 使用 `atan2` 计算当前/目标方向角度差，以 `TURN_RATE × dt` 为最大步长做平滑插值：
   - Level 1: 2.0 rad/s
   - Level 2: 3.0 rad/s
   - Level 3: 5.0 rad/s

8. **执行移动** — `ai->move(direction.x, direction.y, dt)`

### `void AIController::resetState(Ball* ai)`
清除指定 AI 的状态（AI 死亡时调用）。
