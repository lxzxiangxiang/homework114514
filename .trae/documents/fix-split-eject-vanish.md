# 分裂/吐孢消失诊断与修复计划

## 诊断

### 分裂 Bug 🔴

**原因 1**: 玩家没按 WASD 时 `playerInputDirection = (0,0)` → `split((0,0))` → 偏移量 `(0,0)` → 新球和母球完全重叠。

**原因 2**: 重叠 + 同 owner → 同帧 `checkCollisions()` 检测到同源接触 → 大球吃小球 → 两球等半径则前者吃后者 → 新球瞬间消失。

**原因 3**: offset = `(newRadius + newRadius*0.1)` ≈ `1.1 × newRadius`，但接触需要 `dist ≤ r1+r2 = 2×newRadius`。1.1 < 2 → 只要方向向量有效，两球也几乎接触，merge 判定极易触发。

### 吐孢 Bug 🟡

**原因**: `EjectBall::EjectBall` 中 `vx = dx * 8.0`，`update()` 中 `setPos(pos + vx*dt)` 且 `dt=0.016`。每帧位移 = `8 × 0.016 = 0.128 像素`。且 `vx *= 0.98` 指数衰减，20 秒总位移仅 ~6 像素。孢子几乎原地不动，被母球立刻吞回。

---

## 修复

### Step 1: split() 方向后备 + 增大偏移 + 合并豁免

**Ball::split()**:
```cpp
// 方向后备
QPointF dir(direction);
if (std::abs(dir.x()) < 1e-6 && std::abs(dir.y()) < 1e-6) {
    dir = QPointF(lastDx, lastDy);
    if (std::abs(dir.x()) < 1e-6 && std::abs(dir.y()) < 1e-6)
        dir = QPointF(1, 0);
}
// 偏移：至少间距 > r1+r2 防止立即合并
qreal offsetDist = newRadius * 3.0;
QPointF offsetDir(dir.x() / len, dir.y() / len);
newBall->setPos(pos() + offsetDir * offsetDist);
```

### Step 2: checkCollisions 同源豁免新分裂球

同源合并加条件：球心距 > 半径和×0.5 才合并（即不是完全重叠才合并），或更简单：在 split 的偏移足够大（3×半径）后就不需要额外保护。

**简化方案**: 偏移 `newRadius * 3.0` > `r1 + r2 = 2*newRadius`，已超过接触距离，同帧不会 merge。

### Step 3: EjectBall 速度 ×25

- `EjectBall::EjectBall`: `vx = dx * 200.0`（原来 ×8，太小）
- 每帧位移 = `200 × 0.016 = 3.2 像素`，20 秒可飞约 2000 像素

---

## 实施步骤

### Step 1: Ball::split() — 方向后备 + 大偏移
- 方向(0,0) 后备到 lastDx/lastDy 或 (1,0)
- offset = `normalized(dir) * newRadius * 3.0`

### Step 2: EjectBall 初速度 8→200
- `EjectBall.cpp`: `dx * 8.0` → `dx * 200.0`

### Step 3: 编译验证
