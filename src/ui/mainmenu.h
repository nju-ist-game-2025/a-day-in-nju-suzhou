#ifndef MAINMENU_H
#define MAINMENU_H

#include <QLabel>
#include <QPushButton>
#include <QVBoxLayout>
#include <QWidget>

class MainMenu : public QWidget {
    Q_OBJECT

   private:
    QPushButton* startButton;
    QPushButton* characterButton;  // 角色选择按钮
    QPushButton* codexButton;
    QPushButton* exitButton;
    QLabel* titleLabel;
    QString m_backgroundPath;  // 背景图片路径

   public:
    explicit MainMenu(QWidget* parent = nullptr);

   protected:
    void resizeEvent(QResizeEvent* event) override;

   signals:

    void startGameClicked();
    void selectCharacterClicked();  // 角色选择信号
    void codexClicked();            // new
    void exitGameClicked();
};

#endif  // MAINMENU_H
