# 分裂/吐孢随机误差

**项目**: `D:\code\project\0.0.1\AgarClone_Qt`

---

## 现状确认

- `QGraphicsView::drawForeground()` painter 已是视口坐标系，当前 HUD 绘制代码无需修改，之前多虑了。
- 已确认 [GameView.cpp:L186-L212](file:///D:/code/project/0.0.1/AgarClone_Qt/GameView.cpp#L186-L212) 代码正确。

---

## 问题: 分裂/吐孢加入方向及距离的随机误差

### 当前

[Ball.cpp:L54-L69](file:///D:/code/project/0.0.1/AgarClone_Qt/Ball.cpp#L54-L69) `split()`：
- 角度有 `±0.26` 弧度随机偏差（`splitEjectDir`）
- 距离固定: `offsetDist = newRadius * 2.0 + 30.0` — 无抖动

[Ball.cpp:L72-L81](file:///D:/code/project/0.0.1/AgarClone_Qt/Ball.cpp#L72-L81) `eject()`：
- 构造 EjectBall 时方向已有角度偏差
- 速度固定（`dx * 200.0` 在构造中），无抖动

### 修复

**Ball.cpp `split()`** — 距离加 ±20% 随机：

```cpp
qreal offsetDist = (newRadius * 2.0 + 30.0)
    * (1.0 + (QRandomGenerator::global()->generateDouble() - 0.5) * 0.4);
```

**Ball.cpp `eject()`** — 构造 EjectBall 时速度加 ±15% 随机：

```cpp
qreal speedJitter = 1.0 + (QRandomGenerator::global()->generateDouble() - 0.5) * 0.3;
return new EjectBall(pos(), m_color, dir.x() * speedJitter, dir.y() * speedJitter);
```

---

## 修改文件清单

| 文件 | 行 | 修改 |
|------|-----|------|
| `Ball.cpp` | L66 | `split()`: offsetDist 乘 `1.0 ± 0.2` 随机因子 |
| `Ball.cpp` | L80-L81 | `eject()`: 速度乘 `1.0 ± 0.15` 随机因子 |