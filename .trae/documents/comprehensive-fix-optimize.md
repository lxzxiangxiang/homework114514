# 综合修复+优化计划

## 变更清单

### 1. 分裂/吐孢方向+位置 🔴

| 项 | 修复 |
|----|------|
| 方向 | `lastDx/lastDy`（上次运动方向），零则取(1,0) |
| 随机误差 | 方向角 ±15°（`±0.26rad`） |
| 位置 | 方向 × (2r + 30) 偏移，out of contact range |

- `Ball::split()` + `Ball::eject()` 均适用

### 2. 出生数量

| 项 | 当前 | 改为 |
|----|------|------|
| 初始 Food | 100 | 200 |
| AI 初始 | 15 | 20 (AIBALL_COUNT_INIT) |
| AI 最小补位 | 无 | aiBalls<20 时自动 spawn |

### 3. 出生护盾（无敌与护盾统一为 Shield）

用户反馈：无敌和护盾效果重复，统一为护盾。

- **Ball.h**: 删除 `invincibleTimer` 成员，`isInvincible()` → 改为 `return hasShield()`
- **GameScene**: 玩家出生时 `player->skill = Shield; player->skillTimer = 3.0f;`
- **GameScene::spawnAIBall()**: 同上
- **Ball::update()**: 删除 `invincibleTimer` 递减
- **Ball::paint()**: 删除 isInvincible() 闪烁分支（Shield 已有蓝色外圈）
- **GameView.cpp**: 删除效果列表中 "Invincible/无敌" 的收集逻辑（护盾已在 skill 中）

### 4. HUD 加分裂提示

新 6 行布局：`分数 | 时间 | 半径 | AI | 效果 | 分裂`

- `UIManager.h/.cpp`: 新增 `m_hudSplit`
- `updateHUD` 签名加 `bool canSplit`
- `GameView.cpp`: 遍历时检测 `SPLIT_THRESHOLD`

### 5. 全文本中文化

| English | 中文 |
|---------|------|
| Score/Time/Radius/AI/Effects/Split | 分数/时间/半径/AI/效果/分裂 |
| Speed/Shield/Grow/Invisible/Magnet | 加速/护盾/巨大/隐身/磁力 |
| Bomb/Trap/Poison | 炸弹/陷阱/中毒 |
| None | 无 |
| Ready / - | 可 / 不可 |
| "Agar.io Clone" | "球球大乱斗" |
| Menu keyboard hints | 中文 |
| PAUSED / GAME OVER / VICTORY! | 已暂停 / 游戏结束 / 胜利! |
| All UI strings | 全中文化 |

### 6. 优化清理

- **Ball.h**: 删除 `vx/vy` 成员（move() 内局部变量已足够）
- **Ball.h**: 删除 `invincibleTimer`
- **Ball.cpp**: 删除 `isInvincible()` 闪烁 paint 分支
- **Ball::eject()**: 加方向后备+随机误差+距离

---

## 实施步骤 (7步)

### Step 1: Ball::split/eject 方向+位置
### Step 2: 出生数量 (Constants, GameScene)
### Step 3: 无敌→护盾统一 (Ball.h/cpp, GameScene, GameView)
### Step 4: HUD 分裂行 (UIManager.h/cpp, GameView.cpp)
### Step 5: 全中文 (UIManager.cpp, GameView.cpp)
### Step 6: 代码清理 (Ball.h 删 vx/vy/invincibleTimer)
### Step 7: 编译验证
