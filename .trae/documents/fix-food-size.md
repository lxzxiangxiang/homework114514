# Food 体积缩小

**根因**: `EntityRadius::FOOD_MAX = 8.0f` 且 spawnFood 随机公式加了 `+1`，食物半径最大到 `9.0`。

**当前**:
```cpp
// Constants.h
inline constexpr float FOOD_MIN = 3.0f;
inline constexpr float FOOD_MAX = 8.0f;

// GameScene.cpp L189
qreal r = FOOD_MIN + random * (FOOD_MAX - FOOD_MIN + 1);  // [3, 9)
```

**修复**: 缩小上限 + 移除多余的 `+1`：
```cpp
// Constants.h
inline constexpr float FOOD_MIN = 2.0f;
inline constexpr float FOOD_MAX = 5.0f;

// GameScene.cpp L189
qreal r = FOOD_MIN + random * (FOOD_MAX - FOOD_MIN);  // [2, 5)
```

| 项目 | 修改 |
|------|------|
| `Constants.h` L102-L103 | `FOOD_MIN 3→2`, `FOOD_MAX 8→5` |
| `GameScene.cpp` L189 | 移除 `+ 1` |