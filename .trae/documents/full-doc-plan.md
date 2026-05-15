# 全流程文档计划（运行时调用链为主）

## 目标
聚焦游戏从**启动→运行→结束**全生命周期的完整调用链，说明每一步触发条件、调用哪些方法、数据如何流转。其他内容简略。

## 文档结构

### 1. 项目概览（极简）
- 技术栈一句话，目录结构一张表

### 2. 启动流程
- `main()` → `QApplication` → `GameView()` → `setScene()` → `UIManager()` → `returnToMenu()` → `showMenu()`
- 此时屏幕：黑色背景 + 半透明菜单 + "Agar.io Clone" + 按键提示

### 3. 开始游戏
- 按 Enter → `keyPressEvent(Enter)` → `startGame()`
- `startGame()` 内部：删除旧 scene → `new GameScene()` → `new UIManager()` → 清空按键 → State=Playing → 启动 QTimer(16ms)
- `GameScene()` 构造函数：`setSceneRect(5000×5000)` → `spawnFood(100)` → `spawnAIBall()×5` → 创建红色玩家球体(半径15, 地图中心) → `playerBalls.append()`

### 4. 主游戏循环（每帧 16ms）
- QTimer::timeout → `advanceGame()`
- **若非 Playing 状态**：直接 return
- `advanceGame()` 按顺序：
  1. `processPlayerInput()` — 从 `m_keysPressed` 读 WASD，归一化方向向量，Space→wantSplit, E→wantEject
  2. `m_gameScene->updateGame(0.016)` — 见步骤5
  3. 计算玩家总半径 → 判定胜负（全灭→gameOver, ≥2000→victory）
  4. `updateCamera()` — 质量加权中心 + lerp 平滑 + 动态 zoom
  5. 聚合 HUD 数据 → `m_uiManager->updateHUD(...)`

### 5. updateGame(dt) 内部 13 步
每一步说明触发条件、遍历哪些列表、调用什么方法：

1. **移动玩家球体** — 遍历 `playerBalls` → `ball->move(inputDir, dt)`
   - `move()`: `speed = 300 × sqrt(10/radius)` → vx/vy → `setPos()` → clamp 边界 → Trap 减速
2. **分裂** — 若 `wantSplit` → 遍历 `playerBalls` → `ball->split(direction)`
   - `split()`: 检查半径≥18 + splitTimer≤0 → 自身半径←r/√2 → new Ball(同属性) → 偏移位置 → splitTimer=mergeTimer=1.5s → newBall 无敌3s → 返回指针 → `addPlayerBall()`
3. **吐孢** — 若 `wantEject` → 遍历 `playerBalls` → `ball->eject()`
   - `eject()`: 检查半径≥25 → 自身半径-3 → `new EjectBall(自身位置, 同色, lastDx, lastDy)` → 返回指针 → `addEjectBall()`
4. **合并球体列表** — `allBalls = playerBalls + aiBalls`
5. **AI 更新** — 遍历 `aiBalls` → `AIController::updateAI(ai, allBalls, foods, dt)` — 见步骤6
6. **抛射物更新** — 遍历 `ejectBalls` → `eb->update(dt)` — 生命周期递减、移动、速度衰减×0.98
7. **技能球计时器** — `m_skillSpawnTimer -= dt` → 到期生成 → `spawnSkillBall()`
8. **危险物计时器** — `m_hazardSpawnTimer -= dt` → 到期生成 → `spawnHazard()`
9. **同源吸引力** — `applyAttraction(dt)` → 遍历同 owner 球对 → 吸引力公式 → 相向移动 — 见步骤8
10. **碰撞检测** — `checkCollisions()` — 见步骤7
11. **移除死亡实体** — `removeDeadEntities()` → lambda 逆向遍历 6 个列表 → `removeItem` + `delete` + `removeAt`
12. **补豆子** — 维持数量 = MAX_FOOD_COUNT = 500
13. **生存时间** — `survivalTime += dt`

### 6. AIController::updateAI() 决策流程
- 每帧递减 `decisionTimer`（Level1:0.4s, Level2:0.2s, Level3:0.1s）
- 到期后：
  1. **威胁检测** — 找 `radius > self×1.1` 且 `dist < self×6` 的球 → 叠加逃离向量
  2. **猎物检测** — 找 `radius < self×0.9` 的球 → 评分 `value=radius/(dist+1)` → 追最高分
  3. **巡逻** — 无威胁无猎物 → 随机 `[0, MAP_WIDTH]` 巡逻点
  4. **分裂决策(Level≥2)** — 逃生(威胁距<self×3)或猎食(猎物距<self×(4或5) 且 self>prey×1.5) → `ai->split(dir)` → 结果存 `pendingSplitBall`
- **平滑转向** — atan2 角度差 → clamp(最大步长=TurnRate×dt) → 新方向
- `ai->move(dir, dt)`
- GameScene 在 AI 循环后检查 `pendingSplitBall` → 注册到 `aiBalls`

### 7. checkCollisions() 5 种碰撞
每种碰撞的触发条件、判定公式、调用链：

1. **Ball↔Food** — `dist² ≤ (rB+rF)²` 且 `rB > rF×1.1` → `ball->eat(food)` → 玩家 `score+=1`
2. **Ball↔SkillBall** — `dist² ≤ (rB+rS)²` → `ball->applySkill(sb->type)` → `sb->onEaten()`
3. **Ball↔Hazard** — 护盾跳过 → `dist² ≤ (rB+rH)²` → `ball->applyDebuff(hazard->type)` → `hazard->onEaten()`
   - Bomb: `setRadius(r×0.85)` 瞬时
   - Trap: debuff=Trap, timer=3s → move() 中 `speed×0.5`
   - Poison: debuff=Poison, timer=4s → update() 中 `radius -= 2×dt`
4. **Ball↔EjectBall** — `dist² ≤ (rB+rE)²` 且 `rB > rE×1.1` → `ball->eat(eb)` → 玩家 `score+=0.5`
5. **Ball↔Ball** — 用 SpatialGrid 查 3×3 邻域 → `dist² ≤ (r1+r2)²`：
   - 同源(`isPlayer`同 或 `aiId`相同>0) + mergeTimer≤0 → 大球 `eat()` 小球
   - 非同源 + `r大 > r小×1.1` + 小无护盾 → 大球 `eat()` 小球 → 玩家 `score += 被吃球半径×0.5`

### 8. applyAttraction() 合并力
- 遍历同 owner 球体对（`isPlayer` 同 或 `aiId` 相同>0)
- `splitTimer>0` 跳过
- 吸引力 = `50 + 0.001×dist² + 500/(mergeTimer总和+1)`
- 两球各向对方移动 `attraction×dt/2`
- 合并靠靠拢后的 checkCollisions 处理

### 9. 暂停/继续
- ESC → `keyPressEvent(Esc)` → `pauseGame()` → State=Paused → `m_gameTimer->stop()` → `showPause()` (半透明遮罩 + "PAUSED")
- ESC → `keyPressEvent(Esc)` → `resumeGame()` → State=Playing → `m_gameTimer->start()` → `hideAll()`

### 10. 游戏结束（失败）
- 触发：`playerBalls` 全部死亡 或 总半径≤0（`advanceGame()` 中检测）
- `gameOver()` → State=GameOver → stop timer → `resetTransform()` + `centerOn()` → `showGameOver(score, time)` (红色遮罩 + 分数时间)

### 11. 胜利
- 触发：总半径 ≥ 2000（`advanceGame()` 中检测）
- `victory()` → State=Victory → stop timer → `resetTransform()` + `centerOn()` → `showVictory(score, time)` (绿色遮罩)

### 12. 返回菜单 + 重新开始
- M → `returnToMenu()` → State=Menu → stop timer → `resetTransform()` + `centerOn()` → `showMenu()`
- Enter → `startGame()` → 删除旧场景 → 新建
