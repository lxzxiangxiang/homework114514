# 计划：为 .cpp 文件添加中文注释与文档

## 目标
在 `AgarClone_Qt/` 下新建 `docs/` 文件夹，为每个 `.cpp` 文件编写独立的中文文档，同时在所有 11 个 `.cpp` 文件中添加中文注释。

## 涉及文件（11 个 .cpp）
| 文件 | 行数 | 主要内容 |
|------|------|----------|
| main.cpp | ~10 | 程序入口 |
| Food.cpp | ~36 | 豆子实体 |
| EjectBall.cpp | ~40 | 抛射物 |
| SkillBall.cpp | ~45 | 技能球 |
| Hazard.cpp | ~50 | 危险物 |
| SpatialGrid.cpp | ~70 | 空间网格 |
| Ball.cpp | ~169 | 玩家/AI 球体 |
| AIController.cpp | ~155 | AI 控制器 |
| GameScene.cpp | ~180 | 游戏场景 |
| GameView.cpp | ~295 | 游戏视图+状态机 |
| UIManager.cpp | ~349 | UI 管理器 |

## 实施步骤

### Step 1：创建文档目录
- 新建 `AgarClone_Qt/docs/` 文件夹

### Step 2：为每个 .cpp 编写独立的中文文档文件
在 `docs/` 下为每个 .cpp 创建对应的 `.md` 文档：

| 文档文件 | 对应源文件 | 文档内容要点 |
|----------|-----------|-------------|
| `main.md` | main.cpp | 入口函数，QApplication 创建，GameView 启动 |
| `Food.md` | Food.cpp | 豆子实体，随机半径/颜色，绘制椭圆，被吃逻辑 |
| `EjectBall.md` | EjectBall.cpp | 抛射物实体，初速度、减速、生命周期 |
| `SkillBall.md` | SkillBall.cpp | 技能球实体，5种技能随机，脉冲光效，30秒消失 |
| `Hazard.md` | Hazard.cpp | 危险物实体，3种类型，触碰减益，45秒消失 |
| `SpatialGrid.md` | SpatialGrid.cpp | 空间网格碰撞优化，3×3邻域查询，O(N)复杂度 |
| `Ball.md` | Ball.cpp | 球体实体，移动/分裂/吐孢/吞食，技能/负面效果，绘制特效 |
| `AIController.md` | AIController.cpp | AI状态机，三级决策(巡逻/猎食/躲避/分裂)，平滑转向 |
| `GameScene.md` | GameScene.cpp | 场景管理，实体生成/更新/移除，碰撞检测，游戏循环 |
| `GameView.md` | GameView.cpp | 视图+状态机，键盘输入，摄像机动态缩放，胜负判定 |
| `UIManager.md` | UIManager.cpp | UI管理，HUD显示，菜单/暂停/结束/胜利界面 |

每个文档包含：
- 文件概述
- 类/结构体说明
- 每个方法的用途、参数、返回值、算法简述

### Step 3：为所有 .cpp 文件添加中文行内注释
为每个 `.cpp` 文件中的方法、关键逻辑添加中文注释，格式遵循：
- 文件头注释：文件功能概述 + 所属项目
- 类/结构体注释：用途说明
- 方法注释：功能、参数含义、算法逻辑
- 关键代码段注释：解释核心算法（如 AI 决策、碰撞检测、摄像机缩放等）

各文件注释要点：
- **main.cpp**：入口说明
- **Food.cpp**：构造函数随机逻辑，paint 绘制逻辑
- **EjectBall.cpp**：速度衰减公式，生命周期管理
- **SkillBall.cpp**：脉冲光效算法，技能随机分配
- **Hazard.cpp**：三种危险物绘制差异
- **SpatialGrid.cpp**：单元格计算公式，邻域遍历算法
- **Ball.cpp**：速度与半径反比公式，移动边界限制，技能/减益计时器，paint 各种特效
- **AIController.cpp**：决策状态机流程，威胁/猎物评估算法，平滑转向插值
- **GameScene.cpp**：游戏循环10步骤，实体生成随机逻辑，移除死亡实体lambda
- **GameView.cpp**：状态机转换规则，HUD数据聚合，摄像机加权中心+动态缩放
- **UIManager.cpp**：HUD布局计算，界面文本格式

### Step 4：验证编译
- 注释不影响代码逻辑，确保 `cmake --build` 仍然通过
