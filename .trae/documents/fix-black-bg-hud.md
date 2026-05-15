
# 背景黑色 + HUD 显示修复计划

## Bug 诊断

### Bug 1: 背景不是黑色
**位置**: [GameView.cpp:L14-L40](file:///d:/code/project/0.0.1/AgarClone_Qt/GameView.cpp#L14-L40)

**原因**: `QGraphicsView` 默认背景是白色/灰色，没有显式设置。

**修复**: 在构造函数中添加 `setBackgroundBrush(Qt::black)`

---

### Bug 2: HUD 不显示

**位置**: [UIManager.cpp:L376-L395](file:///d:/code/project/0.0.1/AgarClone_Qt/UIManager.cpp#L376-L395)

**原因**: `ItemIgnoresTransformations` 的作用机制是：item 渲染时不受 view 的缩放/旋转影响，但 **item 的 `pos()` 仍然位于场景坐标系中**。当摄像机 `centerOn(2500, 2500)` 时，viewport 左上角对应的场景坐标大约是 `(2500 - 640/zoom, 2500 - 360/zoom)`，而 HUD 固定设置在 `pos(10, 10)`，远在可视区域之外。

**修复**: 在 `updateHUD()` 中，动态计算 HUD 原点。通过 `mapToScene(0, 0)` 获取视口左上角在场景中的坐标，把 HUD 项定位在那里：

```cpp
QPointF hudOrigin = m_view->mapToScene(0, 0);
m_hudScore->setPos(hudOrigin.x() + margin, hudOrigin.y() + margin);
```

因此需要给 `UIManager` 传入 `QGraphicsView*` 指针，并在 `updateHUD()` 中使用。

---

## 修复步骤

### Step 1: 背景黑色
- `GameView` 构造函数添加 `setBackgroundBrush(Qt::black)`

### Step 2: HUD 动态定位
- `UIManager.h`: 添加 `QGraphicsView* m_view` 成员
- `UIManager.h`: 构造函数增加 `QGraphicsView* view` 参数
- `UIManager.cpp`: 存储 `m_view` 指针
- `UIManager.cpp::updateHUD()`: 用 `m_view->mapToScene(0, 0)` 作为 HUD 原点
- `GameView.cpp`: 创建 `UIManager` 时传入 `this`

### Step 3: 编译验证
