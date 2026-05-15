# main.cpp — 程序入口

## 文件概述
Agar.io Clone 球球大作战游戏的程序入口文件。仅负责创建 Qt 应用程序实例、初始化游戏视图并启动事件循环。

## 涉及类
- QApplication：Qt 应用程序主类
- GameView：游戏视图（继承 QGraphicsView），构造函数中已完成全部窗口配置

## 方法说明

### `int main(int argc, char *argv[])`
程序主入口函数。

| 参数 | 说明 |
|------|------|
| argc | 命令行参数数量 |
| argv | 命令行参数数组 |

**执行流程：**
1. 创建 `QApplication` 对象 `a`，接管命令行参数
2. 实例化 `GameView` 对象 `view`（构造函数设置窗口标题 "Agar.io Clone"、尺寸 1280×720、场景和 UI）
3. 调用 `view.show()` 显示窗口
4. 进入 Qt 事件循环 `a.exec()`，阻塞直到窗口关闭
