#ifndef MAINMENU_H
#define MAINMENU_H

#include <QLabel>
#include <QPixmap>
#include <QPushButton>
#include <QVBoxLayout>
#include <QWidget>

class MainMenu : public QWidget
{
    Q_OBJECT

private:
    QPushButton *startButton;
    QPushButton *characterButton; // 角色选择按钮
    QPushButton *codexButton;
    QPushButton *devModeButton; // 开发者模式按钮
    QPushButton *exitButton;
    QLabel *titleLabel;
    QPixmap m_titlePixmap;
    bool m_useTitleImage = false;
    QString m_backgroundPath;

public:
    explicit MainMenu(QWidget *parent = nullptr);

protected:
    void resizeEvent(QResizeEvent *event) override;
    void showEvent(QShowEvent *event) override;

private:
    // 调整标题图片的大小以适应当前窗口尺寸
    void adjustTitlePixmap();

signals:

    void startGameClicked();
    void selectCharacterClicked();  // 角色选择信号
    void codexClicked();            // new
    void devModeClicked(int level); // 开发者模式选择关卡
    void exitGameClicked();
};

#endif // MAINMENU_H
