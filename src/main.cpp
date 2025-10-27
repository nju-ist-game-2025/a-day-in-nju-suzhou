#include <QApplication>
#include "gamewindow.h"

// Windows specific fixes for MinGW
#ifdef __MINGW32__
extern "C" {
int __declspec(dllimport) __argc;
char** __declspec(dllimport) __argv;
}
int* __imp___argc = &__argc;
char*** __imp___argv = &__argv;
#endif

int main(int argc, char* argv[]) {
    QApplication a(argc, argv);
    GameWindow w;
    w.show();
    return a.exec();
}
