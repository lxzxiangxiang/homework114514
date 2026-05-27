# 修复"运行一段时间后崩溃"计划

## 问题概述
游戏在运行一段时间后会崩溃。由于 Qt 6.11 商业许可证问题无法编译运行，本文档通过静态代码分析排查潜在崩溃原因。

---

## 静态分析发现的所有问题

### 🔴 BUG 1: sameOwner 合并后死球继续参与碰撞处理

**文件**: `GameScene.cpp` → `checkCollisions()` 函数 (L327-332)

**问题**: 当两个同源球体合并时（如 `ball2->eat(ball1)`），`ball1` 被标记为死亡，但内层循环**没有 `break`**，继续处理 `nearby` 列表中的下一个实体。此时 `ball1` 已经死亡，但仍会参与后续的碰撞判定：

```cpp
if (sameOwner) {
    if (ball1->radius() >= ball2->radius()) {
        ball1->eat(ball2);
    } else {
        ball2->eat(ball1);   // ball1 死亡，但没有 break！
    }
}
// 内层循环继续，死球 ball1 可能与 ball3 再次合并
```

**影响**: 死亡球体可能继续吞食其他活球，导致活球被标记为死亡，链式反应。虽然指针仍然有效（未被 delete），但这会导致不可预期的状态——被死球吞食的活球会丢失质量。

**修复**: `ball2->eat(ball1)` 后添加 `break`，与之前 D-001 修复保持一致。

---

### 🔴 BUG 2: Grow 技能期间吃到的质量会在技能结束时丢失

**文件**: `Ball.cpp` → `applySkill()` (L103-104) + `update()` (L149-152)

**问题**: Grow 技能激活时，`m_growOriginalRadius` 保存的是激活前的半径。但如果在 Grow 期间球体吃到了食物或其他球体，半径会增加。当 Grow 结束时，半径被**恢复**到激活前的值，**丢失了期间吃到的所有质量**：

```cpp
// applySkill: 保存原始半径（50），扩大 1.3 倍（65）
m_growOriginalRadius = m_radius;
setRadius(m_radius * 1.3f);

// eat() 被调用：半径从 65 变成 68
// ...

// update: Grow 到期
if (skill == SkillType::Grow && m_growOriginalRadius > 0) {
    setRadius(m_growOriginalRadius);  // 恢复到 50，丢失了 eat 得来的质量！
}
```

**修复**: Grow 到期时，如果当前半径大于 `m_growOriginalRadius * 1.3`（说明期间吃到了东西），应按比例恢复而非直接重置为原始半径。

---

### 🟡 BUG 3: AI 分裂球体在创建帧内不被处理

**文件**: `GameScene.cpp` → `updateGame()` (L73-148)

**问题**: `allBalls` 列表在 AI 更新循环之前构建。AI 分裂产生的新球体被添加到 `aiBalls`，但**不在 `allBalls` 列表**中。因此分裂球体在当前帧内：
- 不会获得计时器更新（技能/减益计时）
- 不会参与引力吸引
- 不会与食物/技能/危险物碰撞
- 不会被其他球体检测到碰撞（不在空间网格中）

```cpp
QList<Ball*> allBalls;
allBalls.append(playerBalls);
allBalls.append(aiBalls);  // ← 此时还没有分裂球体

// AI 更新循环中分裂的球体只加到 aiBalls，没加到 allBalls
for (Ball* ai : currentAIBalls) {
    AIController::updateAI(ai, allBalls, dt);
    if (ai->pendingSplitBall) {
        addItem(ai->pendingSplitBall);
        aiBalls.append(ai->pendingSplitBall);  // ← 只加到这里
    }
}

// allBalls 不包含新分裂的球体
applyAttraction(allBalls, dt);
checkCollisions(allBalls);
```

**影响**: 分裂球体在创建帧内"隐身"一帧，虽然不会直接崩溃，但可能造成瞬间碰撞穿透。

**修复**: AI 分裂球体也加入 `allBalls` 列表。

---

### 🟡 BUG 4: `applyAttraction` 修改位置后空间网格不同步

**文件**: `GameScene.cpp` → `applyAttraction()` (L349-380)

**问题**: `applyAttraction` 通过 `setPos` 移动同源球体使其相互靠近，但空间网格是在之后的 `checkCollisions` 开头才重建的。这意味着：
- `applyAttraction` 后的位置变化不反映在网格中（但网格还没被使用，所以当前没影响）
- 如果将来在 `applyAttraction` 和 `checkCollisions` 之间插入使用网格的代码，会有不一致问题

**当前状态**: 由于 `checkCollisions` 开头会 `clear()` + 重建网格，所以目前没有实际 bug。但代码脆弱，无需修改。

---

### 🟢 BUG 5: `AIController::s_states` 使用 `Ball*` 作为 key 可能残留悬空指针

**文件**: `AIController.cpp` (L10) + `GameScene.cpp` → `removeDeadEntities()` (L396-399)

**问题**: `s_states` 是 `QHash<Ball*, AIState>`。当 Ball 被 delete 后，如果 hash 中仍有该指针的条目，则存在悬空指针。

**当前处理**: `removeDeadEntities()` 在 delete 之前调用 `AIController::resetState(ai)` 清除条目。逻辑正确，但依赖调用顺序，较为脆弱。当前不需要修改。

---

## 可能崩溃的根本原因分析

经过全面审查，**代码逻辑层面没有明显的 use-after-free 或 double-free**。`removeDeadEntities()` 的正向遍历 + 反向删除模式是正确的。AIController 状态清理在 delete 之前完成。

最可能的崩溃原因是以下之一：

1. **BUG 1**（sameOwner 合并链式反应）导致大量球体在单帧内死亡，`removeDeadEntities` 批量 delete 大量 QGraphicsItem，可能触发 Qt 内部的 BSP 树重平衡导致栈溢出或断言失败。

2. **Qt 内部 BSP 树问题**: 每帧有 `addItem`（新生实体）+ `removeItem`+`delete`（死亡实体）+ `setRect`/`setPos`（半径/位置变化触发的 `prepareGeometryChange`）。在长时间运行后，BSP 树的增删改操作累积可能导致内部状态不一致。

3. **内存碎片**: 每帧 new/delete 多个 QGraphicsItem 对象（AI 分裂、食物重生等），长时间运行后内存碎片化可能导致分配失败。

---

## 修复计划

### 步骤 1: 修复 sameOwner 合并缺少 break (BUG 1)

**文件**: `GameScene.cpp` → `checkCollisions()`

在 sameOwner 合并中，`ball2->eat(ball1)` 后添加 `break`：

```cpp
if (sameOwner) {
    if (ball1->radius() >= ball2->radius()) {
        ball1->eat(ball2);
    } else {
        ball2->eat(ball1);
        break;  // ← 新增: 防止死球继续参与碰撞
    }
}
```

### 步骤 2: 修复 Grow 技能吃质量丢失 (BUG 2)

**文件**: `Ball.cpp` → `update()`

Grow 到期时，如果当前半径大于 `m_growOriginalRadius * 1.3`（期间吃到了东西），按比例缩小而非直接重置：

```cpp
if (skill == SkillType::Grow && m_growOriginalRadius > 0) {
    qreal expectedRadius = m_growOriginalRadius * 1.3;
    if (m_radius > expectedRadius) {
        setRadius(m_radius / 1.3);  // 保留额外质量
    } else {
        setRadius(m_growOriginalRadius);
    }
    m_growOriginalRadius = 0;
}
```

### 步骤 3: AI 分裂球体加入 allBalls (BUG 3)

**文件**: `GameScene.cpp` → `updateGame()`

AI 分裂球体添加到场景时，同步加入 `allBalls` 列表：

```cpp
if (ai->pendingSplitBall) {
    addItem(ai->pendingSplitBall);
    aiBalls.append(ai->pendingSplitBall);
    allBalls.append(ai->pendingSplitBall);  // ← 新增
    ai->pendingSplitBall = nullptr;
}
```

### 步骤 4: 编译验证

解决 Qt 许可证问题后编译验证所有修改。

---

## 修改文件清单

| 文件 | 修改内容 |
|------|----------|
| `GameScene.cpp` | BUG 1: sameOwner 合并后 break; BUG 3: AI 分裂球体加入 allBalls |
| `Ball.cpp` | BUG 2: Grow 技能到期保留额外质量 |