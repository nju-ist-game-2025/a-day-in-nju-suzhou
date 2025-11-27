#include "scalingenemy.h"
#include <QDebug>
#include <QFont>
#include <QGraphicsScene>
#include <QGraphicsTextItem>
#include <QPainter>
#include <QPointer>
#include <QRandomGenerator>
#include <QTimer>
#include "player.h"
#include "../core/audiomanager.h"
#include "../ui/explosion.h"

ScalingEnemy::ScalingEnemy(const QPixmap &pic, double scale)
    : Enemy(pic, 1.0), // 不让Enemy缩放，传入scale=1.0
      m_scalingTimer(nullptr),
      m_baseScale(scale), // 保存基础缩放用于setScale
      m_minScale(1.0),
      m_maxScale(3.0),
      m_currentScale(1.0),
      m_scaleSpeed(0.04),
      m_scalingUp(true),
      m_originalPixmap(pic), // 保存原始未缩放图片
      m_isFlashing(false),
      m_flashTimer(nullptr)
{
    // 设置移动模式为绕圈移动
    setMovementPattern(MOVE_CIRCLE);

    // 设置绕圈参数
    setCircleRadius(180.0);
    setSpeed(2.5);
    setHealth(25);
    setContactDamage(3);

    // 禁用平滑变换，保持像素锐利
    setTransformationMode(Qt::FastTransformation);

    // 直接使用原图，不进行任何缩放处理
    m_normalPixmap = pic;
    QGraphicsPixmapItem::setPixmap(m_normalPixmap);

    // 设置变换原点为图片中心
    setTransformOriginPoint(m_normalPixmap.width() / 2.0, m_normalPixmap.height() / 2.0);

    // 使用setScale来控制初始大小
    setScale(m_baseScale * m_currentScale);

    // 创建缩放定时器
    m_scalingTimer = new QTimer(this);
    connect(m_scalingTimer, &QTimer::timeout, this, &ScalingEnemy::updateScaling);
    m_scalingTimer->start(50);

    // 创建闪烁定时器
    m_flashTimer = new QTimer(this);
    m_flashTimer->setSingleShot(true);
    connect(m_flashTimer, &QTimer::timeout, this, &ScalingEnemy::endFlashEffect);

    // 预生成闪烁图片（使用原图）
    m_flashPixmap = m_normalPixmap;
    QPainter painter(&m_flashPixmap);
    painter.setCompositionMode(QPainter::CompositionMode_SourceIn);
    painter.fillRect(m_flashPixmap.rect(), QColor(255, 0, 0, 180));
    painter.end();
}

ScalingEnemy::~ScalingEnemy() {
    if (m_scalingTimer) {
        m_scalingTimer->stop();
        delete m_scalingTimer;
        m_scalingTimer = nullptr;
    }
    if (m_flashTimer)
    {
        m_flashTimer->stop();
        delete m_flashTimer;
        m_flashTimer = nullptr;
    }
}

void ScalingEnemy::pauseTimers() {
    Enemy::pauseTimers();
    if (m_scalingTimer) {
        m_scalingTimer->stop();
    }
    if (m_flashTimer)
    {
        m_flashTimer->stop();
    }
}

void ScalingEnemy::resumeTimers() {
    Enemy::resumeTimers();
    if (m_scalingTimer) {
        m_scalingTimer->start(50);
    }
}

void ScalingEnemy::updateScaledPixmaps()
{
    // 此方法现在仅用于更新闪烁图片
    m_flashPixmap = m_normalPixmap;
    QPainter painter(&m_flashPixmap);
    painter.setCompositionMode(QPainter::CompositionMode_SourceIn);
    painter.fillRect(m_flashPixmap.rect(), QColor(255, 0, 0, 180));
    painter.end();
}

void ScalingEnemy::updateScaling()
{
    // 更新缩放比例
    if (m_scalingUp) {
        m_currentScale += m_scaleSpeed;
        if (m_currentScale >= m_maxScale) {
            m_currentScale = m_maxScale;
            m_scalingUp = false;
        }
    } else {
        m_currentScale -= m_scaleSpeed;
        if (m_currentScale <= m_minScale) {
            m_currentScale = m_minScale;
            m_scalingUp = true;
        }
    }

    // 使用setScale进行变换（保持原图不变，只改变渲染缩放）
    // m_baseScale 是基础大小，m_currentScale 是动态伸缩
    double totalScale = m_baseScale * m_currentScale;
    setScale(totalScale);

    // 调试输出（每50帧输出一次减少日志量）
    static int debugCounter = 0;
    if (++debugCounter >= 50)
    {
        qDebug() << "ScalingEnemy::updateScaling - baseScale:" << m_baseScale
                 << "currentScale:" << m_currentScale << "totalScale:" << totalScale;
        debugCounter = 0;
    }
}

void ScalingEnemy::takeDamage(int damage)
{
    // 应用闪烁效果
    applyFlashEffect();

    // 调用父类处理伤害逻辑（但不调用父类的flash）
    int realDamage = qMax(1, damage);
    health -= realDamage;

    if (health <= 0)
    {
        // 停止定时器
        if (aiTimer)
            aiTimer->stop();
        if (moveTimer)
            moveTimer->stop();
        if (attackTimer)
            attackTimer->stop();
        if (m_scalingTimer)
            m_scalingTimer->stop();
        if (m_flashTimer)
            m_flashTimer->stop();

        // 播放死亡音效
        AudioManager::instance().playSound("enemy_death");

        // 创建爆炸动画
        Explosion *explosion = new Explosion();
        explosion->setPos(this->pos());
        if (scene())
        {
            scene()->addItem(explosion);
            explosion->startAnimation();
        }

        emit dying(this);

        if (scene())
        {
            scene()->removeItem(this);
        }
        deleteLater();
    }
}

void ScalingEnemy::applyFlashEffect()
{
    m_isFlashing = true;
    QGraphicsPixmapItem::setPixmap(m_flashPixmap);
    m_flashTimer->start(120); // 120ms后结束闪烁
}

void ScalingEnemy::endFlashEffect()
{
    m_isFlashing = false;
    QGraphicsPixmapItem::setPixmap(m_normalPixmap);
}

QRectF ScalingEnemy::boundingRect() const
{
    return QGraphicsPixmapItem::boundingRect();
}

QPainterPath ScalingEnemy::shape() const
{
    // 返回基于当前pixmap的碰撞形状
    QPainterPath path;
    path.addRect(boundingRect());
    return path;
}

void ScalingEnemy::onContactWithPlayer(Player* p) {
    Q_UNUSED(p);
    if (QRandomGenerator::global()->bounded(100) < 50)
    {
        applySleepEffect();
    }
}

void ScalingEnemy::attackPlayer() {
    if (!player)
        return;

    if (m_isPaused)
        return;

    QList<QGraphicsItem *> collisions = collidingItems();
    for (QGraphicsItem *item : collisions)
    {
        Player *p = dynamic_cast<Player *>(item);
        if (p)
        {
            p->takeDamage(contactDamage);

            if (QRandomGenerator::global()->bounded(100) < 50)
            {
                applySleepEffect();
            }
            break;
        }
    }
}

void ScalingEnemy::applySleepEffect() {
    if (!player || !scene())
        return;

    if (!player->canMove())
        return;

    if (player->isEffectOnCooldown())
        return;

    qDebug() << "ScalingEnemy触发昏睡效果！";

    player->setEffectCooldown(true);

    QGraphicsTextItem *sleepText = new QGraphicsTextItem("昏睡ZZZ");
    QFont font;
    font.setPointSize(16);
    font.setBold(true);
    sleepText->setFont(font);
    sleepText->setDefaultTextColor(QColor(128, 128, 128));
    sleepText->setPos(player->pos().x(), player->pos().y() - 40);
    sleepText->setZValue(200);
    scene()->addItem(sleepText);

    player->setCanMove(false);

    QPointer<Player> playerPtr = player;

    QTimer::singleShot(1500, [playerPtr, sleepText]()
                       {
        if (playerPtr)
        {
            playerPtr->setCanMove(true);
            QTimer::singleShot(3000, [playerPtr]()
            {
                if (playerPtr)
                {
                    playerPtr->setEffectCooldown(false);
                }
            });
        }
        if (sleepText)
        {
            if (sleepText->scene())
            {
                sleepText->scene()->removeItem(sleepText);
            }
            delete sleepText;
        } });
}
