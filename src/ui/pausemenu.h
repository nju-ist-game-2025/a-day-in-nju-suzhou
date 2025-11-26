#ifndef PAUSEMENU_H
#define PAUSEMENU_H

#include <QGraphicsProxyWidget>
#include <QGraphicsRectItem>
#include <QGraphicsTextItem>
#include <QObject>
#include <QPushButton>

/**
 * @brief 暂停菜单类
 * 在游戏中按ESC键时显示的暂停菜单
 */
class PauseMenu : public QObject {
Q_OBJECT

public:
    explicit PauseMenu(QGraphicsScene *scene, QObject *parent = nullptr);

    ~PauseMenu();

    void show();

    void hide();

    bool isVisible() const { return m_isVisible; }

signals:

    void resumeGame();    // 继续游戏
    void returnToMenu();  // 返回主菜单
    void exitGame();      // 退出游戏

private:
    void createUI();

    QGraphicsScene *m_scene;
    QGraphicsRectItem *m_overlay;         // 半透明遮罩
    QGraphicsRectItem *m_menuBackground;  // 菜单背景
    QGraphicsTextItem *m_titleText;       // 标题
    QGraphicsProxyWidget *m_resumeProxy;
    QGraphicsProxyWidget *m_menuProxy;
    QGraphicsProxyWidget *m_exitProxy;
    QPushButton *m_resumeButton;
    QPushButton *m_menuButton;
    QPushButton *m_exitButton;
    bool m_isVisible;
};

#endif  // PAUSEMENU_H
