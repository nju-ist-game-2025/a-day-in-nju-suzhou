#ifndef BOSS_H
#define BOSS_H

#include <QPainter>
#include <QPixmap>
#include "enemy.h"

class Boss : public Enemy {
Q_OBJECT

public:
    explicit Boss(const QPixmap &pic, double scale = 1.5);

    ~Boss() override;

    // 重写绘制方法，添加血条
    void paint(QPainter *painter, const QStyleOptionGraphicsItem *option, QWidget *widget) override;

    QRectF boundingRect() const override;

protected:
    // 绘制血条
    void drawHealthBar(QPainter *painter);
};

#endif  // BOSS_H
