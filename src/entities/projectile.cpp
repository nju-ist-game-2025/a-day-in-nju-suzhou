#include "projectile.h"
#include "enemy.h"
#include "player.h"

Projectile::Projectile(int _mode, double _hurt, QPointF pos, const QPixmap &pic_bullet, double scale)
        : mode(_mode) {
    setTransformationMode(Qt::SmoothTransformation);

    // 禁用缓存以避免留下轨迹
    setCacheMode(QGraphicsItem::NoCache);

    xdir = 0;
    ydir = 0;
    speed = 1.0;
    hurt = _hurt;

    // 修复：如果scale是1.0，直接使用原始pixmap，否则按比例缩放
    if (scale == 1.0) {
        this->setPixmap(pic_bullet);
    } else {
        // 按比例缩放（保持宽高比）
        this->setPixmap(pic_bullet.scaled(
                pic_bullet.width() * scale,
                pic_bullet.height() * scale,
                Qt::KeepAspectRatio,
                Qt::SmoothTransformation));
    }

    this->setPos(pos);

    moveTimer = new QTimer(this);
    connect(moveTimer, &QTimer::timeout, this, &Projectile::move);
    moveTimer->start(16);

    crashTimer = new QTimer(this);
    connect(crashTimer, &QTimer::timeout, this, &Projectile::checkCrash);
    crashTimer->start(50);
}

void Projectile::move() {
    // 检查是否超出边界
    double newX = pos().x() + xdir;
    double newY = pos().y() + ydir;

    if (newX < 0 || newX > scene_bound_x || newY < 0 || newY > scene_bound_y) {
        // 超出边界，删除子弹
        // 定时器由父对象自动管理，无需手动删除
        if (scene()) {
            scene()->removeItem(this);
        }
        deleteLater();
    } else {
        QPointF dir(xdir, ydir);
        this->setPos(pos() + speed * dir);
    }
}

void Projectile::checkCrash() {
    // 确保scene存在
    if (!scene())
        return;

            foreach (QGraphicsItem *item, scene()->items()) {
            if (mode) {
                if (auto it = dynamic_cast<Player *>(item)) {
                    if (abs(it->pos().x() - this->pos().x()) > it->crash_r ||
                        abs(it->pos().y() - this->pos().y()) > it->crash_r)
                        continue;
                    else {
                        it->takeDamage(hurt);
                        // 子弹击中玩家后消失
                        if (scene()) {
                            scene()->removeItem(this);
                        }
                        deleteLater();
                        return;
                    }
                }
            } else {
                if (auto it = dynamic_cast<Enemy *>(item)) {
                    if (abs(it->pos().x() - this->pos().x()) > it->crash_r ||
                        abs(it->pos().y() - this->pos().y()) > it->crash_r)
                        continue;
                    else {
                        it->takeDamage(static_cast<int>(hurt));
                        // 子弹击中怪物后消失
                        if (scene()) {
                            scene()->removeItem(this);
                        }
                        deleteLater();
                        return;
                    }
                }
            }
        }
}
