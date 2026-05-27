# AI ID 上限 + 单 AI 球数上限

| 常量 | 旧 | 新 |
|------|-----|-----|
| `Spawning::AIBALL_COUNT` | 10 | **50** |
| `Spawning::MAX_EFFECT` | 30 | **50** |
| `Spawning::MAX_BALLS_PER_AI` | (新增) | **15** |

**AIController.cpp** `updateAI()` split 前统计同 aiId 球数，≥15 则跳过。

| 文件 | 修改 |
|------|------|
| `Constants.h` | AIBALL_COUNT→50, MAX_EFFECT→50, 新增 MAX_BALLS_PER_AI=15 |
| `AIController.cpp` | 决策前检查同 aiId 球数 |