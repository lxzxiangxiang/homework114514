# 地图上大型圆形物体 — AI 球体起始半径过大

**问题**: 那些"看着很大"的是 **AI Ball**。`spawnAIBall()` 中：
```cpp
qreal radius = 15.0 + QRandomGenerator::global()->bounded(46.0);  // [15, 61)
```
20 个 AI 球半径在 15~61 之间（大部分 30~50），对比玩家起始 15、食物 3~9，AI 球在视觉上压倒性地大。

**修复**:

| 常量 | 旧 | 新 | 说明 |
|------|-----|-----|------|
| AI 起始半径 | `15 + random(46)` → [15,61) | `12 + random(10)` → [12,22) | AI 球与玩家同量级 |

| 文件 | 行 | 修改 |
|------|-----|------|
| `GameScene.cpp` | L231 | `qreal radius = 15.0 + QRandomGenerator::global()->bounded(46.0);` → `qreal radius = 12.0 + QRandomGenerator::global()->bounded(10.0);` |