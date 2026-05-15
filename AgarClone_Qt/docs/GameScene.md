# GameScene.cpp — 游戏场景(GameScene)实现

## 文件概述
GameScene 是游戏的核心引擎，管理所有实体的生成、更新、碰撞检测和移除。每帧执行完整的游戏循环。

## 涉及类
- `GameScene : public QGraphicsScene` — 游戏场景管理器

## 方法说明

### `GameScene::GameScene(QObject* parent)`
构造函数。设置场景尺寸为 MAP_WIDTH × MAP_HEIGHT，生成初始 100 个豆子和 5 个 AI 球体。

### `void GameScene::updateGame(qreal dt)`
主游戏循环（每 16ms 执行一次），按顺序执行 9 个步骤：

1. **移动玩家球体** — 遍历 `playerBalls`，调用 `move()` 朝输入方向移动
2. **处理分裂/吐孢** — 根据 `wantSplit`/`wantEject` 标志位触发操作
3. **更新 AI 球体** — 通过 `AIController::updateAI()` 驱动所有 AI
4. **更新抛射物** — 调用 `eb->update(dt)`
5. **技能球/危险物生成计时器** — 技能球每 ~5s、危险物每 ~8s 概率生成，有数量上限
6. **同源球体吸引力** — `applyAttraction(dt)`（预留）
7. **碰撞检测** — `checkCollisions()`（预留）
8. **移除死亡实体并补充豆子** — 维持豆子数量 = MAX_FOOD_COUNT
9. **累加生存时间**

### `void GameScene::spawnFood(int count)`
在随机地图位置生成指定数量豆子。

### `void GameScene::spawnSkillBall()`
在随机位置生成一个技能球（随机 5 种技能之一）。

### `void GameScene::spawnHazard()`
在随机位置生成一个危险物（随机 3 种类型之一）。

### `void GameScene::spawnAIBall()`
生成一个 AI 球体：随机颜色、半径 15~60、等级 1~3。

### `void GameScene::checkCollisions()`
碰撞检测骨架。预留 TODO：
- Ball ↔ Food（吞食豆子）
- Ball ↔ SkillBall（获得技能）
- Ball ↔ Hazard（受减益）
- Ball ↔ EjectBall（吞食孢子）
- Ball ↔ Ball（大球吞小球，同源合并）

### `void GameScene::applyAttraction(qreal dt)`
同源球体吸引力骨架。预留 TODO。

### `void GameScene::removeDeadEntities()`
使用模板 lambda 逆向遍历所有 6 个实体列表，移除并 `delete` 已死亡实体。
