# 修复"球体数量无上限"计划

## 问题描述
玩家和 AI 通过反复分裂可以产生无限数量的球体：
- 玩家分裂([GameScene.cpp:L46-L56](file:///d:/code/project/0.0.1/AgarClone_Qt/GameScene.cpp#L46-L56))：无上限，`addPlayerBall` 直接 append
- AI 分裂([GameScene.cpp:L83-L87](file:///d:/code/project/0.0.1/AgarClone_Qt/GameScene.cpp#L83-L87))：无上限，`aiBalls.append` 直接添加
- AI 补充([GameScene.cpp:L160-L164](file:///d:/code/project/0.0.1/AgarClone_Qt/GameScene.cpp#L160-L164))：只维护下限 `AIBALL_COUNT_INIT=20`，不限制上限

球体过多会导致碰撞检测 O(n²) 开销剧增、HUD 遍历变慢、最终可能 OOM 或性能崩溃。

## 修复方案

### 步骤 1: Constants.h — 添加上限常量

在 `AIBALL_COUNT_INIT` 常量旁添加两个上限常量：

```cpp
inline constexpr int AIBALL_COUNT_INIT  = 20;
inline constexpr int MAX_AI_BALLS       = 60;   // AI球体总上限
inline constexpr int MAX_PLAYER_BALLS   = 16;   // 玩家球体上限
```

### 步骤 2: GameScene.cpp — 玩家分裂上限

在分裂循环开头检查 `playerBalls.size()`，超限则停止分裂：

```cpp
if (wantSplit) {
    QList<Ball*> currentPlayerBalls = playerBalls;
    for (Ball* ball : currentPlayerBalls) {
        if (playerBalls.size() >= GameConstants::MAX_PLAYER_BALLS) break;
        if (ball->isAlive() && ball->radius() >= GameConstants::SPLIT_THRESHOLD) {
            Ball* newBall = ball->split(playerInputDirection);
            if (newBall) {
                addPlayerBall(newBall);
            }
        }
    }
    wantSplit = false;
}
```

### 步骤 3: GameScene.cpp — AI 分裂上限

在 AI 分裂球体添加处检查 `aiBalls.size()`，超限则释放而非添加：

```cpp
if (ai->pendingSplitBall) {
    if (aiBalls.size() < GameConstants::MAX_AI_BALLS) {
        addItem(ai->pendingSplitBall);
        aiBalls.append(ai->pendingSplitBall);
    } else {
        delete ai->pendingSplitBall;
    }
    ai->pendingSplitBall = nullptr;
}
```

### 步骤 4: GameScene.cpp — AI 补充也检查上限

使 AI 补充逻辑同时受上限约束：

```cpp
int aliveAi = 0;
for (Ball* ai : aiBalls) { if (ai->isAlive()) aliveAi++; }
for (int i = aliveAi; i < GameConstants::AIBALL_COUNT_INIT && aiBalls.size() < GameConstants::MAX_AI_BALLS; ++i) {
    spawnAIBall();
}
```

## 修改文件清单

| 文件 | 修改内容 |
|------|----------|
| `Constants.h` | 添加 `MAX_AI_BALLS`(60) 和 `MAX_PLAYER_BALLS`(16) |
| `GameScene.cpp` L47 | 玩家分裂循环增加 `MAX_PLAYER_BALLS` 上限检查 |
| `GameScene.cpp` L83-L87 | AI 分裂球体增加 `MAX_AI_BALLS` 上限检查 |
| `GameScene.cpp` L162 | AI 补充循环增加 `MAX_AI_BALLS` 上限检查 |