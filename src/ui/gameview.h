#ifndef GAMEVIEW_H
#define GAMEVIEW_H

#include <QGraphicsScene>
#include <QGraphicsView>
#include <QKeyEvent>
#include <QWidget>
#include "../entities/player.h"

class GameView : public QWidget {
    Q_OBJECT

   private:
    QGraphicsView* view;
    QGraphicsScene* scene;
    Player* player;

   public:
    explicit GameView(QWidget* parent = nullptr);
    ~GameView();

    void initGame();

   protected:
    void keyPressEvent(QKeyEvent* event) override;
    void keyReleaseEvent(QKeyEvent* event) override;

   signals:
    void backToMenu();
};

#endif  // GAMEVIEW_H
