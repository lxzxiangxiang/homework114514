# UIManager.cpp — UI 管理器(UIManager)实现

## 文件概述
UIManager 管理游戏的所有界面元素：主菜单、暂停遮罩、游戏结束/胜利界面，以及游戏进行中的 HUD 信息显示。

## 涉及类
- `UIManager : public QObject` — UI 管理器

## 界面系统
| 界面 | 调用方法 | 可见元素 |
|------|----------|----------|
| 主菜单 | `showMenu()` | 半透明黑背景 + 标题 + 操作说明 |
| 暂停 | `showPause()` | 半透明遮罩 + "PAUSED" + 提示 |
| 游戏结束 | `showGameOver(score, time)` | 红色 "GAME OVER" + 分数/时间 |
| 胜利 | `showVictory(score, time)` | 绿色 "VICTORY!" + 分数/时间 |
| HUD | `updateHUD(...)` | 9 项实时数据 |

## HUD 显示项
| 字段 | 内容 |
|------|------|
| Score | 分数 |
| Time | 生存时间 (MM:SS 格式) |
| Mass | 总质量 |
| Radius | 平均半径 |
| AI Count | 剩余 AI 数量 |
| Split | 分裂状态 (Ready / Cooldown) |
| Skill | 当前技能 |
| Debuff | 当前负面效果 |
| Invincible | 无敌状态 |

## 方法说明

### `UIManager::UIManager(QGraphicsScene* scene, QObject* parent)`
构造函数。存储场景指针，调用 `createHUDItems()` 和 `createMenuItems()`。

### `void UIManager::createHUDItems()`
创建 9 个 `QGraphicsTextItem`（白色、Arial 12、z=100），默认隐藏。存储在左上角待 `updateHUD()` 定位显示。

### `void UIManager::createMenuItems()`
创建主菜单元素：
- 半透明黑背景 `QGraphicsRectItem`（alpha=180, z=200）
- 标题 "Agar.io Clone"（Arial 48, Bold）
- 操作说明（WASD/Space/E/ESC/Enter）

### `void UIManager::showMenu()`
显示菜单元素，隐藏暂停/结束/胜利界面及 HUD。

### `void UIManager::showPause()`
惰性创建暂停遮罩（半透明黑 + "PAUSED" + "ESC 继续 / M 回菜单"），z=250。

### `void UIManager::showGameOver(int score, int survivalTime)`
创建红色 "GAME OVER" 遮罩（z=300），显示最终分数和 MM:SS 格式的生存时间，以及重新开始/回菜单提示。

### `void UIManager::showVictory(int score, int survivalTime)`
创建绿色 "VICTORY!" 遮罩（z=300），格式同 showGameOver。

### `void UIManager::hideAll()`
隐藏所有界面元素。

### `void UIManager::updateHUD(...)`
更新 9 个 HUD 文本项并垂直排列：
- 左边距 10px，行高 18px
- 从左上角依次排列 9 行
- 设置文本内容后 `setVisible(true)`
