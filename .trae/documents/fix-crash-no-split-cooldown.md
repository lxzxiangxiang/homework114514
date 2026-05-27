# 崩溃修复 + 去掉分裂冷却和分裂无敌 计划

## Bug 诊断

### 🔴 崩溃原因：AI 分裂时在遍历 aiBalls 的循环中 append

**位置**: [GameScene.cpp:L78-L88](file:///d:/code/project/0.0.1/AgarClone_Qt/GameScene.cpp#L78-L88)

```cpp
for (Ball* ai : aiBalls) {          // 遍历 aiBalls
    ...
    if (ai->pendingSplitBall) {
        aiBalls.append(...);        // ❌ 在遍历过程中修改同一列表！
    }
}
```

`QList` 的 `append()` 可能触发内存重分配，使 range-for 的迭代器失效 → 崩溃。

**修复**: 仿照玩家分裂的做法，遍历前先拷贝：
```cpp
QList<Ball*> currentAIBalls = aiBalls;
for (Ball* ai : currentAIBalls) {
```

---

### 用户要求：去掉分裂冷却 + 分裂无敌

| 当前 | 改为 |
|------|------|
| `Ball::split()` 检查 `splitTimer > 0` 返回 nullptr | 删除检查，随时可分裂 |
| `Ball::split()` 设置 `newBall->splitTimer = 1.5f` | 删除 |
| `Ball::split()` 设置 `newBall->invincibleTimer = 3.0f` | 删除 |
| `Ball::split()` 设置 `splitTimer = 1.5f` | 删除 |
| `AIController::updateAI()` 检查 `splitTimer <= 0` | 删除检查 |
| `AIController::updateAI()` 设置 `ai->splitTimer = 1.5f` | 删除 |

---

## 实施步骤

### Step 1: 修复崩溃 — AI 遍历拷贝
- `GameScene.cpp` L78: `for (Ball* ai : aiBalls)` → `QList<Ball*> currentAIBalls = aiBalls; for (Ball* ai : currentAIBalls)`

### Step 2: Ball::split() 去掉冷却和无敌
- 删除 `if (... || splitTimer > 0) return nullptr` 中的 `splitTimer > 0` 检查
- 删除 `newBall->splitTimer = 1.5f`
- 删除 `newBall->invincibleTimer = 3.0f`
- 删除 `splitTimer = 1.5f`（自身的冷却）
- 保留 `newBall->mergeTimer = 1.5f` 和 `mergeTimer = 1.5f`（合并冷却仍需，防止刚分裂立刻合并）

### Step 3: AIController 去掉 splitTimer 检查
- 删除 `ai->splitTimer <= 0` 条件
- 删除 `ai->splitTimer = 1.5f` 赋值

### Step 4: 编译验证
