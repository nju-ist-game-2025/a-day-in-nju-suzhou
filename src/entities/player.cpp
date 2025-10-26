#include "player.h"
#include "constants.h"
#include "enemy.h"

Player::Player(const QPixmap& pic_player, double scale)
    :redContainers(3), redHearts(3.0), blackHearts(0), soulHearts(0), invincible(false){
    setTransformationMode(Qt::SmoothTransformation);
    this->setPixmap(pic_player.scaled(scale, scale));
    xdir = 0;
    ydir = 0;
    speed = 1.0;
    curr_xdir = 0;
    curr_ydir = 1;//默认向下
    this->setPos(400, 300);//初始位置,根据实际需要后续修改

    hurt = 1;//后续可修改

    keysPressed[Qt::Key_Up] = false;
    keysPressed[Qt::Key_Down] = false;
    keysPressed[Qt::Key_Left] = false;
    keysPressed[Qt::Key_Right] = false;

    keysTimer = new QTimer();
    connect(keysTimer, &QTimer::timeout, this, &Player::move);
    keysTimer->start(16);

    crashTimer = new QTimer();
    connect(crashTimer, &QTimer::timeout, this, &Player::crashEnemy);
    crashTimer->start(50);
}

void Player::keyPressEvent(QKeyEvent *event) {
    if(!event) return;
    if(keysPressed.count(event->key())) {
        keysPressed[event->key()] = true;
    }
    if(event->key() == Qt::Key_W || event->key() == Qt::Key_A || event->key() == Qt::Key_S || event->key() == Qt::Key_D) {
        Projectile *bullet = new Projectile(0, this->hurt, this->pos(), pic_bullet);//还可添加图片比例的参数
        switch (event->key()) {
            case Qt::Key_W: bullet->setDir(0, -1); break;
            case Qt::Key_S: bullet->setDir(0, 1); break;
            case Qt::Key_A: bullet->setDir(-1, 0); break;
            case Qt::Key_D: bullet->setDir(1, 0); break;
            default: break;
        }
        event->accept();
    }
}

void Player::keyReleaseEvent(QKeyEvent *event) {
    if(keysPressed.count(event->key())) {
        keysPressed[event->key()] = false;
    }
}

void Player::move() {
    if(keysPressed[Qt::Key_Up] == true) xdir--;
    if(keysPressed[Qt::Key_Down] == true) xdir++;
    if(keysPressed[Qt::Key_Left] == true) ydir--;
    if(keysPressed[Qt::Key_Right] == true) ydir++;
    if(pos().x() + xdir >= room_bound_x) xdir = 0;
    if(pos().y() + ydir >= room_bound_y) ydir = 0;
    if(curr_xdir != xdir && curr_ydir != ydir) {
        if(ydir == 1) {
            this->setPixmap(down);
            curr_ydir = 1; curr_xdir = 0;
        } else if(ydir == -1) {
            this->setPixmap(up);
            curr_ydir = -1; curr_xdir = 0;
        } else if(xdir == 1) {
            this->setPixmap(right);
            curr_xdir = 1; curr_ydir = 0;
        } else if(xdir == -1) {
            this->setPixmap(left);
            curr_xdir = -1; curr_ydir = 0;
        }
    }
    QPointF dir(xdir, ydir);
    this->setPos(pos() + speed*dir);
}

void Player::takeDamage(int damage) {
    if (invincible) return;
    while(damage > 0 && soulHearts > 0){
        soulHearts--;
        damage -= 2;
    }
    while(damage > 0 && blackHearts > 0){
        blackHearts--;
        damage -= 2;
    }
    while(damage > 0 && redHearts > 0.0){
        redHearts -= 0.5;
        damage--;
    }
    if(redHearts <= 0.0) die();
    else setInvincible();
}

//死亡效果，有待UI同学的具体实现
void Player::die() {

}

//短暂无敌效果，有待UI同学的具体实现
void Player::setInvincible() {
    invincible = true;
    QTimer::singleShot(1000, this, [this](){
        invincible = false;
    });
}

void Player::crashEnemy() {
    foreach(QGraphicsItem *item, scene()->items()) {
        if (auto it = dynamic_cast<Enemy*>(item)) {
            if(abs(it->pos().x() - this->pos().x()) > it->crash_r + crash_r ||
                abs(it->pos().y() - this->pos().y()) > it->crash_r + crash_r) continue;
            else it->takeDamage(it->crash_hurt);
        }
    }
}
