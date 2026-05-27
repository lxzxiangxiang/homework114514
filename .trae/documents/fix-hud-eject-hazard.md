# HUD位置 + 分裂吐孢质量逻辑 + Debuff圆球图标 修复计划

## Bug 1 🔴 HUD 绘制位置有误

**位置**: [UIManager.cpp:L32-L41](file:///d:/code/project/0.0.1/AgarClone_Qt/UIManager.cpp#L32-L41)

**原因**: 上一次去掉了 `ItemIgnoresTransformations`，导致 HUD 的 `pos(10, 10)` 是场景坐标。当摄像机移动到 (2500, 2500) 时，HUD 远在视口之外。

**修复**: 
1. `createHUDItem()` 恢复 `setFlag(ItemIgnoresTransformations)` — 此时 item 坐标即为视口像素坐标
2. `updateHUD()` 中位置保持 `pos(margin, margin + lineHeight*N)`，去掉了 viewport 映射代码

---

## Bug 2 🟡 吐孢/分裂 质量减少逻辑不对

### 2a. 吐孢

**当前**: `setRadius(m_radius - 3)` — 硬编码减 3 半径，与质量无关

**原因**: 半径 25 时减 3 很多（面积从 625π→484π，-141π），半径 100 时减 3 微不足道（面积从 10000π→9409π，-591π）。EjectBall 固定半径 8（面积 64π），吐孢应该转移等于 EjectBall 面积的质量。

**修复**: `setRadius(sqrt(m_radius² - EJECTBALL_RADIUS²))` — parent 面积 = 原面积 - 64π

### 2b. 分裂

**当前**: `m_radius / sqrt(2)` — 质量平分（每个子球 50%），数学上已正确。但注释写"新球半径 = r/√2"不够清晰。

**修复**: 在 Constants.h 添加 `SPLIT_MASS_RETAIN = 0.5f`，split 中：
```cpp
qreal newRadius = m_radius * std::sqrt(GameConstants::SPLIT_MASS_RETAIN);
```
（等同于 `r/√2`，但语义清晰表示"保留一半质量"）

---

## Bug 3 🟡 Debuff 形状改为圆球+图标

**位置**: [Hazard.cpp:L51-L115](file:///d:/code/project/0.0.1/AgarClone_Qt/Hazard.cpp#L51-L115)

**当前**: Bomb=X标记, Trap=六角星多边形, Poison=骷髅头贝塞尔曲线

**改为**: 统一为暗色圆球 + 白色简单图标，与 SkillBall 风格一致：

| Debuff | 圆球颜色 | 图标 | 绘制 |
|--------|----------|------|------|
| Bomb | 暗红 (80,0,0) | 💣 叉号 | 圆+白色X |
| Trap | 暗棕 (60,30,0) | ⚠ 叹号 | 圆+竖线+点 |
| Poison | 暗绿 (0,60,0) | ☠ 骷髅简化 | 圆+白色T形+两点 |

---

## 实施步骤

### Step 1: HUD 修复
- `UIManager::createHUDItem()`: 恢复 `setFlag(ItemIgnoresTransformations)`
- `updateHUD()` 位置保持 `pos(margin, margin + lineHeight*N)`（已是视口像素坐标）

### Step 2: 吐孢质量 + Constants
- `Constants.h`: 添加 `SPLIT_MASS_RETAIN = 0.5f`
- `Ball::eject()`: `setRadius(sqrt(m_radius² - EJECTBALL_RADIUS²))`
- `Ball::split()`: 用 `SPLIT_MASS_RETAIN` 常量

### Step 3: Hazard 圆球+图标
- `Hazard::paint()`: 重写为圆球+简单图标

### Step 4: 编译验证
