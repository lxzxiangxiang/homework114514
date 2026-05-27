# AgarClone_Qt 全量重构计划

## 审查结果

- **Lint**: 12 DEP-7 qBound（不改）
- **6 Agent**: 类层次、质量重构、Constants、Effect合并、UIManager、可读性建议

---

## 最终类层次

```
Entity (m_mass, radius()=√(mass/π))
├── Ball
│   ├── AIBall    (aiId, AIState 内嵌)
│   └── PlayerBall
├── ResBall
│   ├── Food      (静态)
│   └── EjectBall (含 vx/vy)
└── EffectBall    (合并 SkillBall+Hazard)
```

## 核心改动

### 1. Entity.h: 半径 → 质量
- `m_radius` → `m_mass`
- `radius()` 改为 `√(m_mass/π)`
- `setMass(qreal)` 替代 `setRadius()`
- `mass()` getter

### 2. EffectType 合并（Skill+Debuff+Hazard → 一个 enum）
```cpp
enum class EffectType : uint8_t {
    None, Speed, Shield, Grow, Invisible, Magnet, Bomb, Trap, Poison
};
```
Ball 四字段 `skill/debuff/skillTimer/debuffTimer` → 两字段 `m_effect/m_effectTimer`

### 3. 同 ai_id 共享效果
- `split()` 中复制效果给新球
- `applyEffect()` 遍历同 aiId 球体同步

### 4. AI 数量按 ai_id
- `removeDeadEntities()` 后检查存活 ID 集合
- 只补缺失的 ID，不按球体总数

### 5. GameScene: 6 列表 → 5 列表
- `playerBalls: QList<PlayerBall*>`
- `aiBalls: QList<AIBall*>`
- `resources: QList<ResBall*>` (Food+EjectBall)
- `effects: QList<EffectBall*>` (SkillBall+Hazard)

### 6. Constants.h 按模块 namespace
```
GameConstants::Window / World / Ball / Physics / Spawning / AI / Camera / HUD
```
每个常量带公式注释

### 7. UIManager → GameView
- 7 个 QGraphicsItem 指针 + 方法搬入 GameView
- 删除 UIManager.h/cpp

---

## 文件操作

| 操作 | 文件 |
|------|------|
| 修改 | Entity.h, Ball.h, Ball.cpp, GameScene.h/cpp, GameView.h/cpp, Constants.h, AIController.h/cpp, Food.cpp, EjectBall.h/cpp, CMakeLists.txt |
| 新建 | AIBall.h, PlayerBall.h, ResBall.h, EffectBall.h, EffectBall.cpp |
| 删除 | UIManager.h/cpp, SkillBall.h/cpp, Hazard.h/cpp |

## 执行顺序

| 阶段 | 内容 |
|------|------|
| S1 | Constants.h 模块化重写 |
| S2 | Entity.h m_mass 改造 |
| S3 | EffectType 合并 + EffectBall 新建 |
| S4 | Ball 拆 AIBall/PlayerBall + AIState 迁移 |
| S5 | ResBall 抽象 + Food/EjectBall 改造 |
| S6 | GameScene 列表类型更新 + ai_id 计数 |
| S7 | UIManager → GameView 合并 |
| S8 | CMakeLists.txt + 编译验证 |