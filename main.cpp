// 程序入口：Agar.io Clone 球球大作战
// 创建 QApplication 和 GameView，启动 Qt 事件循环
#include "GameView.h"
#include <QApplication>

int main(int argc, char *argv[])
{
    // 创建 Qt 应用程序实例
    QApplication a(argc, argv);
    // 实例化游戏视图（构造函数自动创建场景、UI、计时器）
    GameView view;
    view.show();
    // 进入事件循环，阻塞直到窗口关闭
    return a.exec();
}
