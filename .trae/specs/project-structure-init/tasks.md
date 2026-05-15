# Tasks

- [x] Task 1: 创建项目目录与 CMakeLists.txt
  - 创建 `AgarClone_Qt/` 目录
  - 编写 CMakeLists.txt：project(AgarClone)，C++17，Qt6 Core+Widgets，qt_add_executable 列出全部源文件
  - 验证：`cmake -S AgarClone_Qt -B AgarClone_Qt/build` 配置通过

- [x] Task 2: 创建 Constants.h 游戏常量头文件
  - 定义窗口/地图尺寸、帧率、速度公式常量
  - 定义实体属性常量（半径、数量上限、持续时间）
  - 定义技能/危险物参数、AI 等级参数
  - 定义空间网格/摄像机/胜利条件常量

- [x] Task 3: 创建 Entity.h 抽象基类
  - 继承 QGraphicsEllipseItem
  - 声明 EntityType 枚举（Ball/Food/SkillBall/Hazard/EjectBall）
  - 声明位置、半径、颜色、存活状态属性
  - 声明虚方法 update(qreal dt)、onEaten(Entity* eater)
  - 声明纯虚方法 type() const

- [x] Task 4: 创建 Ball.h / Ball.cpp 球体类骨架
  - 继承 Entity，实现 type() 返回 EntityType::Ball
  - 声明所有成员变量（vx, vy, isPlayer, aiLevel, aiId, splitTimer, mergeTimer, invincibleTimer, skill, skillTimer, debuff, debuffTimer, lastDx, lastDy）
  - 声明 SkillType / DebuffType 枚举（在 Ball.h 中或 Constants.h 中）
  - 声明 move/split/eject/eat/applySkill/applyDebuff/isInvincible/hasShield 方法骨架
  - 重写 paint() 方法骨架

- [x] Task 5: 创建 Food.h / Food.cpp 豆子类骨架
  - 继承 Entity，实现 type() 返回 EntityType::Food
  - 实现构造函数：随机半径(3~8)、随机颜色
  - 重写 paint() 绘制实心圆

- [x] Task 6: 创建 SkillBall.h / SkillBall.cpp 技能球类骨架
  - 继承 Entity，实现 type() 返回 EntityType::SkillBall
  - 声明 skillType 成员（SkillType 枚举）
  - 声明 lifetime 倒计时
  - 重写 paint() 绘制脉冲光效

- [x] Task 7: 创建 Hazard.h / Hazard.cpp 危险物类骨架
  - 继承 Entity，实现 type() 返回 EntityType::Hazard
  - 声明 hazardType 成员（HazardType 枚举：Bomb/Trap/Poison）
  - 声明 lifetime 倒计时
  - 重写 paint() 按类型绘制不同颜色图形

- [x] Task 8: 创建 EjectBall.h / EjectBall.cpp 抛射物类骨架
  - 继承 Entity，实现 type() 返回 EntityType::EjectBall
  - 声明 vx, vy, lifetime 成员
  - 重写 paint() 绘制实心圆

- [x] Task 9: 创建 SpatialGrid.h / SpatialGrid.cpp 空间网格类
  - 声明 CELL_SIZE = 200 常量
  - 声明内部存储 QHash<QPair<int,int>, QVector<Entity*>>
  - 实现 clear()、add(Entity*)、getNearby(Entity*)

- [x] Task 10: 创建 AIController.h / AIController.cpp AI 控制器骨架
  - 声明 AIState 结构体（direction, decisionTimer, target 等）
  - 声明静态方法 updateAI(Ball*, const QList<Ball*>&, const QList<Food*>&, qreal dt)
  - 维护 QHash<Ball*, AIState> 状态映射
  - 实现基础决策流程骨架（威胁检测/猎物检测/巡逻/分裂决策 四个步骤，每个步骤目前为空实现）

- [x] Task 11: 创建 GameScene.h / GameScene.cpp 游戏场景骨架
  - 继承 QGraphicsScene
  - 声明所有实体列表成员
  - 声明 updateGame(qreal dt) 方法骨架（步骤注释）
  - 声明 spawnFood/spawnSkillBall/spawnHazard/spawnAIBall 方法骨架
  - 声明 checkCollisions/applyAttraction 方法骨架
  - 声明 SpatialGrid 成员

- [x] Task 12: 创建 GameView.h / GameView.cpp 游戏视图骨架
  - 继承 QGraphicsView
  - 声明 State 枚举（Menu/Playing/Paused/GameOver/Victory）
  - 持有 GameScene* 和 UIManager* 指针
  - 声明 QTimer 成员（16ms 间隔）
  - 重写 keyPressEvent 骨架
  - 声明 updateCamera() 方法骨架
  - 声明状态切换方法（startGame/pauseGame/resumeGame/gameOver/victory/returnToMenu）
  - 构造函数中设置场景、窗口标题、尺寸、关闭抗锯齿

- [x] Task 13: 创建 UIManager.h / UIManager.cpp UI 管理器骨架
  - 持有 GameScene* 指针
  - 声明 HUD 相关的 QGraphicsTextItem 指针
  - 声明 showMenu()/showPause()/showGameOver(int,int)/showVictory(int,int)/hideAll()/updateHUD(...) 方法骨架
  - 声明 drawMenuBackground()/drawPauseOverlay() 辅助方法骨架

- [x] Task 14: 创建 main.cpp 入口文件
  - 创建 QApplication
  - 实例化 GameView
  - 调用 show() 进入事件循环
  - 窗口标题 "Agar.io Clone"，初始大小 1280×720

# Task Dependencies
- Task 2~14 均依赖 Task 1（项目目录和 CMake 配置）
- Task 4 依赖 Task 3（Ball 继承 Entity）
- Task 5~8 依赖 Task 3（均继承 Entity）
- Task 11 依赖 Task 4~10（GameScene 引用所有实体类和工具类）
- Task 12 依赖 Task 11, Task 13（GameView 持有 GameScene 和 UIManager）
- Task 13 依赖 Task 11（UIManager 引用 GameScene）
- Task 14 依赖 Task 12（main 实例化 GameView）
- Task 3~10 可并行（无相互依赖）
