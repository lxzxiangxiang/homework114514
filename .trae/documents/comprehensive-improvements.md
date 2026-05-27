# 综合优化计划

## 变更概述

| 需求 | 涉及文件 | 说明 |
|------|----------|------|
| 1. HUD 随摄像机缩放 | UIManager, GameView | 去掉 ItemIgnoresTransformations，HUD 比例随 zoom |
| 2. 效果显示持续时间 | GameView, UIManager | `"Speed(3.2s)"` 格式 |
| 3. 缩放缓动 | GameView | zoom 也加 lerp 平滑 |
| 4. SkillBall 按类型图标 | SkillBall.cpp | 5 种技能各有独特图标 |
| 5. Hazard 暗色球+图标 | Hazard.cpp | 深色底色+清晰图标 |
| 6. Magnet 磁力实现 | GameScene, Ball | 激活时自动吸引附近 Food |
| 7. 去除合并冷却 | Ball.h/.cpp, GameScene | 删除 mergeTimer 所有逻辑 |
| 8. 降低引力倍率+增加初速 | GameScene, Ball | 引力衰减, split 加弹出速度 |

---

## 详细设计

### Step 1: HUD 去掉 ItemIgnoresTransformations
- `UIManager::createHUDItem()`: 删除 `setFlag(ItemIgnoresTransformations)`
- `updateHUD()`: 位置改为固定场景坐标 `(margin, margin)`，让 view transform 自然缩放
  - 每帧 `margin` 要根据 zoom 缩放：`effectiveMargin = margin / zoom`，使 HUD 在屏幕上视觉大小不变
  - 实际上直接设 pos(margin, margin) 并去掉 flag，HUD 就会随缩放变化

> **简化为**：直接 `setPos(margin, margin)`，不做 viewport 坐标映射，让缩放自然生效。

### Step 2: 效果加入持续时间
- `GameView::advanceGame()` 遍历球体时，收集 `"Speed(3.2s)"` 等含计时的文本：
  ```cpp
  if (ball->skill != SkillType::None) {
      QString s = skillName + QString("(%1s)").arg(ball->skillTimer, 0, 'f', 1);
      effectParts.append(s);
  }
  ```
- `UIManager::updateHUD()` 签名不变，接收已格式化好的 effects 字符串。

### Step 3: 缩放缓动
- `GameView.h`: 添加 `qreal m_currentZoom = 1.5f;`
- `GameView::updateCamera()`: 
  ```cpp
  qreal targetZoom = ...; // 现有计算
  m_currentZoom += (targetZoom - m_currentZoom) * 0.08f;
  scale(m_currentZoom, m_currentZoom);
  ```

### Step 4: SkillBall 按类型着色+图标

| 技能 | 颜色 | 图标 | 绘制方式 |
|------|------|------|----------|
| Speed | 亮黄 (255,220,0) | ⚡ 闪电 | 圆 + 折线闪电 |
| Shield | 亮青 (0,200,255) | 🛡 盾牌 | 圆 + 描边盾牌轮廓 |
| Grow | 亮紫 (180,0,255) | ↑ 箭头 | 圆 + 向上箭头 |
| Invisible | 亮灰 (180,180,180) | 👁 眼 | 圆 + 眼睛椭圆 |
| Magnet | 亮橙 (255,150,0) | 🧲 U磁铁 | 圆 + U形+横条 |

构造函数中根据 `skillType` 设置 `m_color`（不再随机 HSV），绘制按类型。

### Step 5: Hazard 改为暗色球+图标

当前已基本符合，只需：
- 增大半径：50 → 60
- 确保颜色更暗：
  - Bomb: 暗红 (80, 0, 0)
  - Trap: 暗棕 (60, 30, 0)
  - Poison: 暗绿 (0, 60, 0)
- 图标白色/亮色高对比度

### Step 6: Magnet 磁力实现

在 `GameScene::updateGame()` 的脚步 6（球体 update）之后，或 `checkCollisions` 之前，添加 Magnet 逻辑：

```cpp
// Magnet: 吸引附近 Food
for (Ball* ball : allBalls) {
    if (!ball->isAlive() || ball->skill != SkillType::Magnet || ball->skillTimer <= 0) continue;
    for (Food* food : foods) {
        if (!food->isAlive()) continue;
        qreal dx = ball->x() - food->x();
        qreal dy = ball->y() - food->y();
        qreal dist = std::sqrt(dx*dx + dy*dy);
        if (dist < ball->radius() * 4.0 && dist > 1e-6) {
            qreal force = 200.0 * dt / dist;
            food->setPos(food->x() + dx * force, food->y() + dy * force);
        }
    }
}
```

### Step 7: 去除合并冷却

删除以下 `mergeTimer` 相关代码：
- `Ball.h`: 删除 `mergeTimer` 成员
- `Ball::split()`: 删除 `newBall->mergeTimer = 1.5f` 和 `mergeTimer = 1.5f`
- `Ball::update()`: 删除 `if (mergeTimer > 0) mergeTimer -= dt;`
- `GameScene::checkCollisions()` L303: `&& mergeTimer <= 0` 条件删除
- `GameScene::applyAttraction()`: 删除 `500.0 / (mergeTimer + ...)` 项

### Step 8: 降低引力倍率 + 增加初速度

- `applyAttraction()` 引力公式：`50 + 0.001*dist²` → `15 + 0.0003*dist²`
- split 分裂球加弹出速度：在 `Ball::split()` 结尾设置 `newBall->vx = direction.x() * 150`、`newBall->vy = direction.y() * 150`

### Step 9: 编译验证
