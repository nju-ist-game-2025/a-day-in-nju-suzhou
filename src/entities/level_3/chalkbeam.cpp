#include "chalkbeam.h"
#include <QBrush>
#include <QDebug>
#include <QPen>
#include <QtMath>
#include "../../core/audiomanager.h"
#include "../../ui/explosion.h"
#include "../player.h"

ChalkBeam::ChalkBeam(QPointF targetPos, const QPixmap &beamPic, QGraphicsScene *scene)
        : QObject(),
          QGraphicsPixmapItem(),
          m_scene(scene),
          m_targetPos(targetPos),
          m_beamPixmap(beamPic),
          m_warningCircle(nullptr),
          m_warningTimer(nullptr),
          m_fallTimer(nullptr),
          m_warningTime(1500),      // 默认1.5秒警告时间
          m_damage(2),              // 默认2点伤害
          m_explosionRadius(50.0),  // 爆炸半径50像素
          m_fallSpeed(15.0),        // 下落速度
          m_currentY(-50),          // 从屏幕上方开始
          m_isDestroying(false) {
    // 设置粉笔图片
    if (!m_beamPixmap.isNull()) {
        QPixmap scaledPix = m_beamPixmap.scaled(30, 60, Qt::KeepAspectRatio, Qt::SmoothTransformation);
        setPixmap(scaledPix);
    } else {
        // 创建默认粉笔图片
        QPixmap defaultPix(30, 60);
        defaultPix.fill(Qt::white);
        setPixmap(defaultPix);
    }

    // 初始位置在屏幕上方，不可见
    setPos(m_targetPos.x(), -100);
    setVisible(false);
    setZValue(90);  // 在大多数物体之上

    qDebug() << "[ChalkBeam] 创建粉笔光束，目标位置:" << m_targetPos;
}

ChalkBeam::~ChalkBeam() {
    if (m_warningTimer) {
        m_warningTimer->stop();
        delete m_warningTimer;
    }
    if (m_fallTimer) {
        m_fallTimer->stop();
        delete m_fallTimer;
    }
    if (m_warningCircle && m_scene) {
        m_scene->removeItem(m_warningCircle);
        delete m_warningCircle;
    }
    qDebug() << "[ChalkBeam] 粉笔光束销毁";
}

void ChalkBeam::startWarning() {
    // 创建警告圈
    createWarningCircle();

    // 启动警告定时器
    m_warningTimer = new QTimer(this);
    m_warningTimer->setSingleShot(true);
    connect(m_warningTimer, &QTimer::timeout, this, &ChalkBeam::onWarningTimeout);
    m_warningTimer->start(m_warningTime);

    qDebug() << "[ChalkBeam] 开始警告，" << m_warningTime << "ms后落下";
}

void ChalkBeam::createWarningCircle() {
    if (!m_scene)
        return;

    // 创建红色警告圈（只有边框）
    double radius = m_explosionRadius;
    m_warningCircle = new QGraphicsEllipseItem(
            m_targetPos.x() - radius,
            m_targetPos.y() - radius,
            radius * 2,
            radius * 2);

    // 设置样式：红色边框，透明填充
    QPen pen(Qt::red, 3);
    m_warningCircle->setPen(pen);
    m_warningCircle->setBrush(QBrush(QColor(255, 0, 0, 50)));  // 半透明红色
    m_warningCircle->setZValue(85);

    m_scene->addItem(m_warningCircle);
}

void ChalkBeam::onWarningTimeout() {
    qDebug() << "[ChalkBeam] 警告结束，开始落下";
    startFalling();
}

void ChalkBeam::startFalling() {
    // 显示粉笔
    setVisible(true);
    m_currentY = -50;
    setPos(m_targetPos.x() - pixmap().width() / 2, m_currentY);

    // 启动下落定时器
    m_fallTimer = new QTimer(this);
    connect(m_fallTimer, &QTimer::timeout, this, &ChalkBeam::onFallTimer);
    m_fallTimer->start(16);  // 约60fps
}

void ChalkBeam::onFallTimer() {
    m_currentY += m_fallSpeed;
    setPos(m_targetPos.x() - pixmap().width() / 2, m_currentY);

    // 检查是否到达目标位置
    if (m_currentY >= m_targetPos.y()) {
        m_fallTimer->stop();
        explode();
    }
}

void ChalkBeam::explode() {
    if (m_isDestroying)
        return;
    m_isDestroying = true;

    qDebug() << "[ChalkBeam] 粉笔落地，爆炸！";

    // 播放爆炸音效
    AudioManager::instance().playSound("enemy_death");

    // 创建爆炸动画
    if (m_scene) {
        Explosion *explosion = new Explosion();
        explosion->setPos(m_targetPos);
        m_scene->addItem(explosion);
        explosion->startAnimation();
    }

    // 对范围内玩家造成伤害
    damagePlayer();

    // 移除警告圈
    if (m_warningCircle && m_scene) {
        m_scene->removeItem(m_warningCircle);
        delete m_warningCircle;
        m_warningCircle = nullptr;
    }

    // 延迟删除自己
    QTimer::singleShot(100, this, &ChalkBeam::onExplosionComplete);
}

void ChalkBeam::damagePlayer() {
    if (!m_scene)
        return;

    // 查找场景中的玩家
    QList<QGraphicsItem *> items = m_scene->items();
    for (QGraphicsItem *item: items) {
        Player *player = dynamic_cast<Player *>(item);
        if (player) {
            // 计算与玩家的距离
            QPointF playerCenter = player->pos() +
                                   QPointF(player->boundingRect().width() / 2, player->boundingRect().height() / 2);

            double dx = playerCenter.x() - m_targetPos.x();
            double dy = playerCenter.y() - m_targetPos.y();
            double distance = qSqrt(dx * dx + dy * dy);

            // 如果在爆炸范围内
            if (distance < m_explosionRadius) {
                player->takeDamage(m_damage);
                qDebug() << "[ChalkBeam] 玩家在爆炸范围内，造成" << m_damage << "点伤害";
            }
            break;
        }
    }
}

void ChalkBeam::onExplosionComplete() {
    if (m_scene) {
        m_scene->removeItem(this);
    }
    deleteLater();
}
