#include "QtWidgetsApplication1.h"
#include <QApplication>

#if defined(_MSC_VER)
#pragma execution_character_set("utf-8")
#endif

int main(int argc, char *argv[])
{
    QApplication app(argc, argv);
    QApplication::setApplicationName(QStringLiteral("虎牌：动态事件排名赛"));
    QtWidgetsApplication1 window;
    window.show();
    return app.exec();
}
