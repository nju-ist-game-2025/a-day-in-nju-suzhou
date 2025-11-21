#include <QApplication>
#include <QDebug>
#include "core/configmanager.h"
#include "core/gamewindow.h"

int main(int argc, char* argv[]) {
    // 禁用缩放，确保窗口大小在所有电脑上一致
#if QT_VERSION < QT_VERSION_CHECK(6, 0, 0)
    QApplication::setAttribute(Qt::AA_DisableHighDpiScaling);
#endif

    QApplication a(argc, argv);
    // 加载配置文件
    if (!ConfigManager::instance().loadConfig("assets/config.json")) {
        qCritical() << "无法加载配置文件，程序退出";
        return 1;
    }
    GameWindow w;
    w.show();
    return QApplication::exec();
}