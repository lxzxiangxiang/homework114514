# 地图上静止大圆 — AI 球体半径过大 + 移动过慢

**根因**: 两项叠加造成"满地图静止大圆"

### 1. 数量多 (20个) + 体积大 (半径 15~61)

[GameScene.cpp:L23](file:///D:/code/project/0.0.1/AgarClone_Qt/GameScene.cpp#L23): 构造时一次性生成 20 个 AI。
[GameScene.cpp:L231](file:///D:/code/project/0.0.1/AgarClone_Qt/GameScene.cpp#L231): 每个 AI 半径 `15 + random(46)` → [15, 61)，大部分 30~50。

对比玩家起始半径 **15**，AI 球体是玩家的 2~4 倍，视觉上极其突出。

### 2. 大球移速极慢

[Ball.cpp:L20](file:///D:/code/project/0.0.1/AgarClone_Qt/Ball.cpp#L20): `speed = 300 × √(10/r)`。半径 50 → 速度 134 px/s → **每帧 2px** → 肉眼几乎不可见。

## 修复

| 项目 | 旧 | 新 | 说明 |
|------|-----|-----|------|
| AI 起始半径 | `15 + random(46)` → [15,61) | `10 + random(8)` → [10,18) | 与玩家 15 同量级 |
| AI 数量 | `AIBALL_COUNT = 20` | `10` | 地图视觉稀疏 |

| 文件 | 行 | 修改 |
|------|-----|------|
| `GameScene.cpp` | L231 | `qreal radius = 15.0 + bounded(46)` → `10.0 + bounded(8)` |
| `Constants.h` | L115 | `AIBALL_COUNT 20` → `10` |