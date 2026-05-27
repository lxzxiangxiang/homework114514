# HUD 精简优化计划

## 变更

| 当前 HUD（9行） | 新 HUD（6行） |
|-----------------|--------------|
| Score | Score |
| Time | Time |
| Mass | **删除** |
| Radius | Radius |
| AI Count | AI Count |
| Split | **删除** |
| Skill | → 合并为 "Effects" |
| Debuff | → 合并为 "Effects" |
| Invincible | → 合并为 "Effects" |

效果合并逻辑：`"Effects: " + (技能列表 | 减益列表 | 无敌信息)，无效果时显示 "Effects: None"`

## 影响文件

### 1. UIManager.h
- 删除成员: `m_hudMass`, `m_hudSplitStatus`, `m_hudSkill`, `m_hudDebuff`, `m_hudInvincible`
- 新增成员: `m_hudEffects`
- `updateHUD()` 签名简化: 去掉 `totalMass`, `canSplit`, `skillInfo`, `debuffInfo`, `invincibleInfo`
- 新增: `updateHUD(qreal score, qreal survivalTime, qreal avgRadius, int aiCount, const QString& effects)`

### 2. UIManager.cpp
- `createHUDItems()`: 只创建 5 项 (Score/Time/Radius/AI/Effects)
- `showMenu()`/`hideAll()`: 更新隐藏列表
- `updateHUD()`: 更新为 5 行定位

### 3. GameView.cpp
- `advanceGame()`: 合并 skillParts/debuffParts/invincibleInfo 为一个 effects 字符串
- 调用 `updateHUD(score, time, avgRadius, aiCount, effects)`

### 4. 编译验证
