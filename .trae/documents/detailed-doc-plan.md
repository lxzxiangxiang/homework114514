# 详细文档编写计划

在 `AgarClone_Qt/docs/详细设计文档.md` 创建一份完整的设计文档。

## 文档结构

### 1. 项目概述
- 技术栈、架构图

### 2. 目录与文件说明

### 3. 类架构
- Entity 继承树 (Entity→Ball/Food/SkillBall/Hazard/EjectBall)
- 管理类关系 (GameView→GameScene, GameView→UIManager, GameScene→AIController)

### 4. 运行时生命周期（全流程调用链）
- 启动→菜单→开始→主循环→暂停→结束→胜利→重启

### 5. GameScene::updateGame() 17步详解

### 6. 各系统详解
- 6.1 移动系统 (Ball::move, 速度公式, 边界)
- 6.2 分裂系统 (split, 质量分配, 方向算法)
- 6.3 吐孢系统 (eject, 质量转移, EjectBall)
- 6.4 吞食系统 (eat, 质量合并)
- 6.5 技能系统 (5种技能 + 计时器)
- 6.6 减益系统 (3种减益 + 效果)
- 6.7 AI系统 (3级决策, 威胁/猎物/巡逻/分裂)
- 6.8 碰撞系统 (5种碰撞, 空间网格优化)
- 6.9 吸引力系统 (同源合并力)
- 6.10 摄像机系统 (质量加权+lerp+动态zoom)
- 6.11 HUD系统 (6行, 随摄像机变换)
- 6.12 空间网格 (SpatialGrid, CELL_SIZE=200)
- 6.13 道具生成器 (SkillBall/Hazard 计时刷新)

### 7. 配置常量速查

### 8. 操作说明

### 9. 数据流图

### 10. 已知问题与后续计划
