# AI 大球无法吞吃小球 — Shield 护盾导致

**根因**: `spawnAIBall()` 中每个 AI 出生带 3 秒 Shield：
```cpp
ai->effect = EffectType::Shield;
ai->effectTimer = 3.0f;
```
`hasShield()` 保护被吃方。前 3 秒所有 AI 互有护盾 → 谁也不吃谁 → 用户看到的"大球无法吞小球"。

"没有无敌效果" = 可视化有（白光闪烁），但用户期望的"无敌"含义不同。

**修复**: 移除外壳护盾，AI 出生即进入正常捕食逻辑。

| 文件 | 修改 |
|------|------|
| `GameScene.cpp` L236-L237 | 删除 `ai->effect = EffectType::Shield;` 和 `ai->effectTimer = 3.0f;` |

关于"有新生的 AI"——这是移除死亡 AI 后自动补充（`removeDeadEntities` → `spawnAIBall`），属于正常行为，无需修改。