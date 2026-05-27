# AgarClone_Qt 可读性与文件职责优化计划

## 审查概要

- **Lint**: 11 条 DEP-7（已知 qreal/float 类型不匹配，不改）
- **6 Agent**: 合并去重后 **15 项** >80 置信度发现

---

## 修复清单

### P0: 死代码删除

| 文件 | 内容 |
|------|------|
| `Entity.h` L5 | 删除 `#include <QPointF>` |
| `UIManager.h` L8 | 删除 `#include <QTimer>` |
| `Constants.h` L108 | 删除死常量 `SPATIAL_GRID_CELL_SIZE` |
| `GameScene.h` L50 | 删除 `class QPainter` 前向声明（已在 cpp 包含） |
| `GameScene.h` | 删除 `drawGrid()` 声明 |
| `GameScene.cpp` | 删除 `drawGrid()` 空实现 |
| `Ball.h` L96 | 删除 `m_vx, m_vy` 未使用成员 |

### P1: 重复代码提取

**`GameScene.cpp`** — sameOwner 逻辑在 checkCollisions 和 applyAttraction 中重复：

```cpp
bool sameOwner = (ball1->isPlayer && ball2->isPlayer)
              || (ball1->aiId > 0 && ball1->aiId == ball2->aiId);
```

修复：提取为私有静态方法 `static bool sameOwner(const Ball* a, const Ball* b)`

### P2: 命名一致性

| 文件 | 修改 |
|------|------|
| `Ball.h` L34 | `m_growOriginalRadius` → `growOriginalRadius` |
| `Ball.cpp` | 同步改名 |

### P3: Constants.h 新增常量

```cpp
inline constexpr float ATTRACTION_BASE = 15.0f;
inline constexpr float ATTRACTION_DIST_FACTOR = 0.0003f;
inline constexpr float SPLIT_RANDOM_ANGLE = 0.26f;
```

### P4: GameScene::updateGame() 拆分

133 行拆分为：

```cpp
void movePlayerBalls(qreal dt);
void processSplitEject();
void updateAIBalls(QList<Ball*>& allBalls, qreal dt);
void updateAllTimers(const QList<Ball*>& allBalls, qreal dt);
void updateMagnetEffect(const QList<Ball*>& allBalls, qreal dt);
void updateProjectilesAndSpecials(qreal dt);
void spawnAndMaintain(qreal dt);
```

### P5: 注释修正

| 文件 | 修改 |
|------|------|
| `EjectBall.cpp` | 修正速度注释 `8` → `200` |
| `Ball.cpp` | Bomb debuff 添加即时效果注释 |

---

## 修改文件清单

| 文件 | 修改 |
|------|------|
| `Entity.h` | 删除 `#include <QPointF>` |
| `UIManager.h` | 删除 `#include <QTimer>` |
| `Constants.h` | 删除 `SPATIAL_GRID_CELL_SIZE` + 新增 3 个常量 |
| `GameScene.h` | 删除 `class QPainter` + `drawGrid()` + 新增拆分方法声明 |
| `GameScene.cpp` | 删除 `drawGrid()` + 拆分 `updateGame()` + 提取 `sameOwner()` + 用常量替换魔法数字 |
| `Ball.h` | 删除 `m_vx/m_vy` + 改 `growOriginalRadius` |
| `Ball.cpp` | Bomb 注释 + 魔法数字改常量 + 改名同步 |
| `EjectBall.cpp` | 修正过时注释 |
