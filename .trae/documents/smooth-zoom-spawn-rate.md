# 缩放缓动 + HUD 间距 + 道具刷新 计划

## 1. 缩放改回缓动（去掉阶跃式）

**位置**: [GameView.cpp L296-L312](file:///d:/code/project/0.0.1/AgarClone_Qt/GameView.cpp#L296-L312)

**当前**: 6 级阶梯 `{1.5, 1.2, 1.0, 0.8, 0.6, 0.5}`，取 ≤ targetZoom 的第一个值

**改为**: 去掉阶梯逻辑，直接使用连续 targetZoom：
```cpp
qreal zoom = targetZoom;
```

删除整个 `zoomLevels` 数组和 for 循环。

---

## 2. HUD 间距修复

**位置**: [UIManager.cpp L325-L327](file:///d:/code/project/0.0.1/AgarClone_Qt/UIManager.cpp#L325-L327)

**问题**: `ItemIgnoresTransformations` 下 item 的 `pos()` 按场景坐标定位，但渲染时忽略 view 变换，导致位置由 `pos - view_scene_origin` 决定。`mapToScene(0,0)` 虽然理论正确，但每帧重新计算可能因 `centerOn` 的浮点精度累积导致微小漂移。

**修复**: `mapToScene(0,0)` 改为直接从 `centerOn` 目标反算，避免浮点累积：
```cpp
// hudOrigin = center - viewport半尺寸（与updateCamera中centerOn的语义一致）
QPointF center = m_view->mapToScene(m_view->viewport()->rect().center());
qreal w = m_view->viewport()->width();
qreal h = m_view->viewport()->height();
QPointF hudOrigin(center.x() - w / 2.0, center.y() - h / 2.0);
```

同时增大间距：`lineHeight = 18.0` → `lineHeight = 20.0`，`margin = 10.0` → `margin = 12.0`。

---

## 3. 增加道具刷新频率

### 当前参数
| 道具 | 生成间隔 | 上限 | 初始生成计时器 |
|------|----------|------|---------------|
| 技能球 | 5~10s | 20 | 0 |
| 危险物 | 8~15s | 10 | 0 |

### 改为
| 道具 | 生成间隔 | 上限 |
|------|----------|------|
| 技能球 | 2~4s | 30 |
| 危险物 | 3~6s | 20 |

### 修改位置
- [Constants.h L51-L52](file:///d:/code/project/0.0.1/AgarClone_Qt/Constants.h#L51-L52): `MAX_SKILLBALL_COUNT = 20→30`, `MAX_HAZARD_COUNT = 10→20`
- [GameScene.cpp L115](file:///d:/code/project/0.0.1/AgarClone_Qt/GameScene.cpp#L115): `5.0f + bounded(5.0)` → `2.0f + bounded(2.0)`
- [GameScene.cpp L122](file:///d:/code/project/0.0.1/AgarClone_Qt/GameScene.cpp#L122): `8.0f + bounded(7.0)` → `3.0f + bounded(3.0)`

---

## 实施步骤

### Step 1: 缩放改回缓动
- `GameView::updateCamera()` 去掉 zoomLevels 阶梯逻辑，改为 `zoom = targetZoom`

### Step 2: HUD 间距修复
- `UIManager::updateHUD()` 用 centerOn 反算 hudOrigin
- lineHeight 18→20，margin 10→12

### Step 3: 道具刷新频率
- `Constants.h` 修改 MAX_SKILLBALL_COUNT / MAX_HAZARD_COUNT
- `GameScene.cpp` 修改生成间隔

### Step 4: 编译验证
