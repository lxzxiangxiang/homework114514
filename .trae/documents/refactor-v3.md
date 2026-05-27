# AgarClone_Qt 重构计划 v3

**审查**: Lint 0 条（已清理） | 当前文件: 11 .h + 9 .cpp (UIManager/SkillBall/Hazard 已删除)

---

## 类层次

```
Entity (m_mass, radius()=√(mass/π))
├── Ball (isPlayer, aiLevel, aiId, effect, lastDx/Dy)
│   ├── AIBall    (aiId, AIState)
│   └── PlayerBall (无额外)
├── ResBall
│   ├── Food      (静态, alive)
│   └── EjectBall (vx/vy, lifetime, alive)
└── EffectBall    (type, lifetime, 合并 SkillBall+Hazard)
```

---

## 执行步骤（每步可编译验证）

### S0: 修正当前编译错误

| 文件 | 问题 | 修复 |
|------|------|------|
| `Ball.h` | `speed()` 未声明 | 加 `qreal speed() const;` |
| `EjectBall.h` | `EntityType` 不存在 | 删 `entityType()` override |
| `Food.h` | `EntityType` 不存在 | 删 `entityType()` override |
| `GameScene.cpp` | `EntityType::Ball` 不存在 | 改为 `dynamic_cast<Ball*>(e)` 检查 |

### S1: AI 数量按 ai_id

| 文件 | 修改 |
|------|------|
| `GameScene.cpp` | `removeDeadEntities` 后用 `QSet<int>` 收集存活 ID，只补缺失的 ID |

### S2: 同 ai_id 球共享效果

| 文件 | 修改 |
|------|------|
| `Ball.cpp` `split()` | 复制 `effect/effectTimer/growOriginalMass` 到新球 |
| `Ball.h/.cpp` `applyEffect()` | 新增参数 `const QList<Ball*>& allBalls`，遍历同步同ai_id球 |

### S3: ResBall 抽象类

| 操作 | 文件 |
|------|------|
| 新建 | `ResBall.h` — 抽象基类: `bool isAlive()`, `void setAlive()`, `virtual void update(dt)=0` |
| 修改 | `Food.h` — `class Food : public ResBall` |
| 修改 | `EjectBall.h` — `class EjectBall : public ResBall` |

### S4: GameScene 列表类型简化

| 旧 | 新 |
|----|-----|
| `QList<SkillBall*> skillBalls` + `QList<Hazard*> hazards` | `QList<EffectBall*> effectBalls` |

### S5: CMakeLists.txt 更新

删除 `UIManager/SkillBall/Hazard` 引用，添加 `EffectBall`。

---

## 修改文件清单

| 文件 | 操作 |
|------|------|
| `Ball.h/cpp` | speed() 声明 + applyEffect 同步参数 |
| `EjectBall.h/cpp` | 删 entityType + 继承 ResBall |
| `Food.h/cpp` | 删 entityType + 继承 ResBall |
| `GameScene.h/cpp` | dynamic_cast 替代 entityType + ai_id 计数 + 列表名 effectBalls |
| `ResBall.h` | **新建** 抽象基类 |
| `CMakeLists.txt` | 加 ResBall/EffectBall，移除旧文件 |