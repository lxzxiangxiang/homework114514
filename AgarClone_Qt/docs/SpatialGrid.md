# SpatialGrid.cpp — 空间网格(SpatialGrid)实现

## 文件概述
空间网格用于优化碰撞检测，将 O(N²) 的全量遍历降为约 O(N) 的局部查询。地图被划分为 200×200 像素的单元格，每个实体注册到所在单元格。碰撞检测时只需查询实体所在单元格及其 8 个邻域内的对象。

## 涉及类
- `SpatialGrid` — 空间网格工具类

## 数据结构
- 单元格大小：`CELL_SIZE = 200`（像素）
- 内部存储：`QHash<QPair<int, int>, QVector<Entity*>>`
  - Key：`(cellX, cellY)` 整数坐标对
  - Value：该单元格内的实体指针列表

## 方法说明

### `void SpatialGrid::clear()`
清空整个网格（通常在每帧碰撞检测前调用）。

### `void SpatialGrid::add(Entity* entity)`
将实体注册到网格。

**算法：**
- `cellX = entity->pos().x() / CELL_SIZE`
- `cellY = entity->pos().y() / CELL_SIZE`
- 将 entity 追加到 `m_grid[{cellX, cellY}]` 的 vector 中

### `void SpatialGrid::remove(Entity* entity)`
从网格中移除指定实体。若移除后该 cell 为空，则从 hash 中删除该条目以节省内存。

### `QList<Entity*> SpatialGrid::getNearby(Entity* entity) const`
基于实体的查询重载，委托给坐标版本。

### `QList<Entity*> SpatialGrid::getNearby(qreal x, qreal y, qreal radius) const`
查询指定位置周围的实体。

**算法（3×3 邻域查询 + 距离过滤）：**
1. 计算中心单元格 `(cx, cy)`
2. 计算搜索半径平方：`(radius × 2)²`
3. 遍历 `dx ∈ [-1, 0, 1]`、`dy ∈ [-1, 0, 1]` 共 9 个邻域 cell
4. 对每个 cell 内的实体，计算到查询位置的平方距离
5. 若 `distSq ≤ searchRadiusSq`，加入结果列表
6. 跳过已死亡实体

**复杂度：**
- 理论 O(N)，实际取决于实体密度分布
- 远比全量 O(N²) 高效，尤其在实体数量多时
