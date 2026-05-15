# GameView.cpp — 游戏视图(GameView)实现

## 文件概述
GameView 是游戏的主窗口和控制器。管理键盘输入、摄像机控制、游戏状态机和 HUD 数据聚合。

## 涉及类
- `GameView : public QGraphicsView` — 游戏视图 & 状态机

## 状态机（State 枚举）
| 状态 | 说明 | 触发 |
|------|------|------|
| Menu | 主菜单 | 启动时 / 按 M |
| Playing | 游戏中 | 按 Enter |
| Paused | 暂停 | 按 ESC |
| GameOver | 游戏结束 | 玩家全灭 |
| Victory | 胜利 | 总半径 ≥ 2000 |

## 方法说明

### `GameView::GameView(QWidget* parent)`
构造函数。
- 创建 `GameScene` 和 `UIManager`
- 设置窗口标题 "Agar.io Clone"、大小 1280×720
- 禁用抗锯齿和滚动条
- 创建 16ms QTimer，连接 `advanceGame()`
- 调用 `returnToMenu()` 显示主菜单

### `GameView::~GameView()`
停止计时器。

### `void GameView::advanceGame()`
每帧回调（16ms）。
1. 非 Playing 状态直接返回
2. 处理玩家输入 → `processPlayerInput()`
3. 更新游戏 → `m_gameScene->updateGame(0.016)`
4. 计算玩家总半径，判定胜负
5. 更新摄像机 → `updateCamera()`
6. 聚合 HUD 数据（技能/减益/无敌信息）→ `m_uiManager->updateHUD(...)`

### `void GameView::keyPressEvent(QKeyEvent*)`
状态机键盘处理：
- **Menu**: Enter → `startGame()`
- **Playing**: ESC → `pauseGame()`；WASD/Space/E → 记录到 `m_keysPressed`
- **Paused**: ESC → `resumeGame()`；M → `returnToMenu()`
- **GameOver/Victory**: Enter → `startGame()`；M → `returnToMenu()`

### `void GameView::keyReleaseEvent(QKeyEvent*)`
从 `m_keysPressed` 移除释放的按键。

### `void GameView::startGame()`
重建 GameScene 和 UIManager，清空按键状态，设 Playing，启动计时器。

### `void GameView::pauseGame() / resumeGame()`
暂停/恢复：切换状态，停止/启动计时器，显示/隐藏暂停 UI。

### `void GameView::gameOver() / victory()`
停止计时器，显示对应 UI（分数 + 生存时间）。

### `void GameView::returnToMenu()`
设置 Menu 状态，停止计时器，显示主菜单。

### `void GameView::updateCamera()`
动态摄像机系统。

**算法：**
1. 计算玩家球体质量加权中心点（半径作为权重）
2. 使用 `lerp(0.1)` 平滑插值当前位置到目标中心
3. 动态缩放：`zoom = CAMERA_ZOOM_MAX × (MIN_RADIUS / maxRadius)`
4. zoom 限制在 [0.5, 1.5] 范围
5. `resetTransform()` + `scale(zoom, zoom)` + `centerOn(smoothCenter)`

### `void GameView::processPlayerInput()`
从 `m_keysPressed` 读取 WASD/方向键，计算归一化方向向量。Space/E 设置分裂/吐孢意图。
