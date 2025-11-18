#include <QApplication>
#include "core/configmanager.h"
#include "gamewindow.h"

int main(int argc, char* argv[]) {
    QApplication a(argc, argv);

    // 加载配置文件
    if (!ConfigManager::instance().loadConfig("assets/config.json")) {
        qCritical() << "无法加载配置文件，程序退出";
        return 1;
    }

    GameWindow w;
    w.show();
    return a.exec();
}
