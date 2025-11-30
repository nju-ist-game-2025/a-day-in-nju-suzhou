#include "probabilityenemy.h"
#include <QDebug>
#include <QGraphicsScene>
#include <QGraphicsTextItem>
#include <QPainter>
#include <QtMath>
#include "../../core/audiomanager.h"
#include "../../core/configmanager.h"
#include "../../ui/explosion.h"
#include "../player.h"

// HealTextController 实现
HealTextController::HealTextController(Enemy* target, QGraphicsTextItem* textItem, QObject* parent)
    : QObject(parent), m_target(target), m_textItem(textItem), m_updateTimer(nullptr) {
    // 创建位置更新定时器
    m_updateTimer = new QTimer(this);
    connect(m_updateTimer, &QTimer::timeout, this, &HealTextController::updatePosition);
    m_updateTimer->start(16);  // 约60fps更新位置

    // 1.5秒后清理
    QTimer::singleShot(1500, this, &HealTextController::cleanup);
}

HealTextController::~HealTextController() {
    if (m_updateTimer) {
        m_updateTimer->stop();
    }
    // 确保文字被移除（只有当它仍在场景中时才删除）
    if (m_textItem && m_textItem->scene()) {
        m_textItem->scene()->removeItem(m_textItem);
        delete m_textItem;
        m_textItem = nullptr;
    }
}

void HealTextController::updatePosition() {
    if (m_target && m_target->scene() && m_textItem) {
        m_textItem->setPos(m_target->pos().x() + 20, m_target->pos().y() - 30);
    }
}

void HealTextController::cleanup() {
    if (m_updateTimer) {
        m_updateTimer->stop();
    }
    // 只有当它仍在场景中时才删除
    if (m_textItem && m_textItem->scene()) {
        m_textItem->scene()->removeItem(m_textItem);
        delete m_textItem;
        m_textItem = nullptr;
    }
    deleteLater();
}

// ProbabilityEnemy 实现
ProbabilityEnemy::ProbabilityEnemy(const QPixmap& pic, double scale)
    : Enemy(pic, scale),
      m_growthTimer(nullptr),
      m_blinkTimer(nullptr),
      m_explodeTimer(nullptr),
      m_contactTimer(nullptr),
      m_isBlinking(false),
      m_exploded(false),
      m_isRed(false),
      m_originalPixmap(pic),
      m_currentScale(INITIAL_SCALE_RATIO),
      m_initialScale(INITIAL_SCALE_RATIO),
      m_elapsedTime(0) {
    // 计算最大缩放比例：使图片高度等于场景高度
    m_maxScale = SCENE_HEIGHT / static_cast<double>(pic.height());

    // 从配置文件读取概率论属性
    ConfigManager& config = ConfigManager::instance();
    setHealth(config.getEnemyInt("probability_theory", "health", 50));
    setContactDamage(config.getEnemyInt("probability_theory", "contact_damage", 2));
    setVisionRange(0);  // 无视野
    setAttackRange(0);  // 无攻击范围
    setSpeed(0);        // 不移动

    // 停止所有继承的定时器（因为不需要移动、AI和攻击检测）
    if (aiTimer) {
        aiTimer->stop();
    }
    if (moveTimer) {
        moveTimer->stop();
    }
    if (attackTimer) {
        attackTimer->stop();
    }

    // 设置初始缩放
    updateScale();

    // 创建成长定时器
    m_growthTimer = new QTimer(this);
    connect(m_growthTimer, &QTimer::timeout, this, &ProbabilityEnemy::onGrowthUpdate);
    m_growthTimer->start(GROWTH_UPDATE_INTERVAL);

    // 创建闪烁定时器
    m_blinkTimer = new QTimer(this);
    connect(m_blinkTimer, &QTimer::timeout, this, &ProbabilityEnemy::onBlinkTimeout);

    // 创建爆炸定时器
    m_explodeTimer = new QTimer(this);
    m_explodeTimer->setSingleShot(true);
    connect(m_explodeTimer, &QTimer::timeout, this, &ProbabilityEnemy::onExplodeTimeout);

    // 创建接触检测定时器（给敌人回血）
    m_contactTimer = new QTimer(this);
    connect(m_contactTimer, &QTimer::timeout, this, &ProbabilityEnemy::onContactCheck);
    m_contactTimer->start(CONTACT_CHECK_INTERVAL);

    qDebug() << "ProbabilityEnemy 创建完成 - 初始缩放:" << m_currentScale
             << "最大缩放:" << m_maxScale << "成长时间:" << GROWTH_DURATION_MS << "ms";
}

ProbabilityEnemy::~ProbabilityEnemy() {
    if (m_growthTimer) {
        m_growthTimer->stop();
        delete m_growthTimer;
        m_growthTimer = nullptr;
    }
    if (m_blinkTimer) {
        m_blinkTimer->stop();
        delete m_blinkTimer;
        m_blinkTimer = nullptr;
    }
    if (m_explodeTimer) {
        m_explodeTimer->stop();
        delete m_explodeTimer;
        m_explodeTimer = nullptr;
    }
    if (m_contactTimer) {
        m_contactTimer->stop();
        delete m_contactTimer;
        m_contactTimer = nullptr;
    }
}

void ProbabilityEnemy::move() {
    // 概率论不移动
}

void ProbabilityEnemy::attackPlayer() {
    // 概率论不主动攻击
}

bool ProbabilityEnemy::hasContactingEnemies() const {
    if (!scene())
        return false;

    // 检测是否有其他敌人与概率论接触
    QList<QGraphicsItem*> collisions = collidingItems();
    for (QGraphicsItem* item : collisions) {
        Enemy* enemy = dynamic_cast<Enemy*>(item);
        if (!enemy)
            continue;

        // 跳过其他概率论敌人
        if (dynamic_cast<const ProbabilityEnemy*>(enemy))
            continue;

        // 使用像素级碰撞检测
        if (Entity::pixelCollision(const_cast<ProbabilityEnemy*>(this), enemy)) {
            return true;
        }
    }
    return false;
}

void ProbabilityEnemy::takeDamage(int damage) {
    if (m_exploded)
        return;

    flash();  // 显示受击闪烁
    health -= qMax(1, damage);

    if (health <= 0) {
        // 被打死时，显示死亡特效但不造成范围伤害
        explode(false);
    }
}

void ProbabilityEnemy::onGrowthUpdate() {
    if (m_isBlinking || m_exploded)
        return;

    // 暂停状态下不成长
    if (m_isPaused)
        return;

    // 更新已过时间
    m_elapsedTime += GROWTH_UPDATE_INTERVAL;

    // 计算当前缩放比例（线性插值）
    double progress = static_cast<double>(m_elapsedTime) / static_cast<double>(GROWTH_DURATION_MS);
    if (progress >= 1.0) {
        progress = 1.0;
        m_currentScale = m_maxScale;

        // 停止成长，开始闪烁
        m_growthTimer->stop();
        startBlinking();

        qDebug() << "ProbabilityEnemy 成长完成，开始闪烁";
    } else {
        // 线性插值计算当前缩放
        m_currentScale = m_initialScale + (m_maxScale - m_initialScale) * progress;
    }

    // 更新缩放
    updateScale();
}

void ProbabilityEnemy::updateScale() {
    // 计算新的图片大小
    int newWidth = static_cast<int>(m_originalPixmap.width() * m_currentScale);
    int newHeight = static_cast<int>(m_originalPixmap.height() * m_currentScale);

    if (newWidth < 1)
        newWidth = 1;
    if (newHeight < 1)
        newHeight = 1;

    // 缩放图片
    m_normalPixmap = m_originalPixmap.scaled(newWidth, newHeight, Qt::KeepAspectRatio, Qt::SmoothTransformation);

    // 创建红色闪烁版本
    m_redPixmap = m_normalPixmap;
    QPainter painter(&m_redPixmap);
    painter.setCompositionMode(QPainter::CompositionMode_SourceIn);
    painter.fillRect(m_redPixmap.rect(), QColor(255, 0, 0, 180));
    painter.end();

    // 更新显示
    if (!m_isRed) {
        QGraphicsPixmapItem::setPixmap(m_normalPixmap);
    } else {
        QGraphicsPixmapItem::setPixmap(m_redPixmap);
    }

    // 设置偏移量使图片中心对齐到pos()位置
    setOffset(-newWidth / 2.0, -newHeight / 2.0);
}

void ProbabilityEnemy::startBlinking() {
    m_isBlinking = true;

    // 开始闪烁（每300ms切换一次）
    m_blinkTimer->start(BLINK_INTERVAL);

    // 2.5秒后爆炸
    m_explodeTimer->start(EXPLODE_DELAY);

    qDebug() << "ProbabilityEnemy 开始闪烁，" << EXPLODE_DELAY << "ms 后爆炸";
}

void ProbabilityEnemy::onBlinkTimeout() {
    toggleBlink();
}

void ProbabilityEnemy::toggleBlink() {
    m_isRed = !m_isRed;
    if (m_isRed) {
        QGraphicsPixmapItem::setPixmap(m_redPixmap);
    } else {
        QGraphicsPixmapItem::setPixmap(m_normalPixmap);
    }
}

void ProbabilityEnemy::onExplodeTimeout() {
    explode(true);
}

void ProbabilityEnemy::onContactCheck() {
    if (m_exploded || m_isPaused || !scene())
        return;

    healContactingEnemies();
}

void ProbabilityEnemy::healContactingEnemies() {
    // 检测与概率论接触的敌人，给它们回血
    QList<QGraphicsItem*> collisions = collidingItems();
    for (QGraphicsItem* item : collisions) {
        // 跳过非敌人
        Enemy* enemy = dynamic_cast<Enemy*>(item);
        if (!enemy)
            continue;

        // 跳过其他概率论敌人
        if (dynamic_cast<ProbabilityEnemy*>(enemy))
            continue;

        // 使用像素级碰撞检测
        if (Entity::pixelCollision(this, enemy)) {
            int currentHealth = enemy->getHealth();
            int maxHealth = enemy->getMaxHealth();

            // 如果已满血，不治疗也不显示
            if (currentHealth >= maxHealth)
                continue;

            // 计算实际治疗量（不超过上限）
            int actualHeal = qMin(HEAL_AMOUNT, maxHealth - currentHealth);
            int newHealth = currentHealth + actualHeal;
            enemy->setCurrentHealth(newHealth);

            // 创建显示绿色治疗文字（跟随目标）
            // 即使实际治疗量不足2点，也显示"治疗2点生命值++"
            QGraphicsTextItem* textItem = new QGraphicsTextItem(QString("治疗%1点生命值++").arg(HEAL_AMOUNT));
            QFont font;
            font.setPointSize(14);
            font.setBold(true);
            textItem->setFont(font);
            textItem->setDefaultTextColor(QColor(0, 200, 0));  // 绿色
            textItem->setPos(enemy->pos().x() + 20, enemy->pos().y() - 30);
            textItem->setZValue(300);
            scene()->addItem(textItem);

            // 创建控制器来管理文字跟随和清理
            new HealTextController(enemy, textItem);

            qDebug() << "ProbabilityEnemy 给敌人回血" << actualHeal << "点，当前血量:" << newHealth << "/" << maxHealth;
        }
    }
}

void ProbabilityEnemy::explode(bool dealDamage) {
    if (m_exploded)
        return;

    m_exploded = true;

    // 停止所有定时器
    if (m_growthTimer) {
        m_growthTimer->stop();
    }
    if (m_blinkTimer) {
        m_blinkTimer->stop();
    }
    if (m_explodeTimer) {
        m_explodeTimer->stop();
    }

    // 只有主动爆炸（成长完成）才对所有实体造成伤害
    if (dealDamage) {
        damageAllEntities();
    }

    // 播放爆炸音效
    AudioManager::instance().playSound("enemy_death");

    // 创建爆炸动画
    if (scene()) {
        auto* explosion = new Explosion();
        explosion->setPos(this->pos());
        scene()->addItem(explosion);
        explosion->startAnimation();
    }

    // 发出dying信号并删除自己
    emit dying(this);

    if (scene()) {
        scene()->removeItem(this);
    }

    deleteLater();

    qDebug() << "ProbabilityEnemy 爆炸！造成伤害:" << dealDamage;
}

void ProbabilityEnemy::damageAllEntities() {
    if (!scene())
        return;

    qDebug() << "ProbabilityEnemy 概率爆炸！盛宴降临！";

    // 获取场景中所有物品
    QList<QGraphicsItem*> allItems = scene()->items();

    for (QGraphicsItem* item : allItems) {
        // 跳过自己
        if (item == this)
            continue;

        // 对玩家：强制扣到只剩1滴血
        if (Player* p = dynamic_cast<Player*>(item)) {
            p->setCurrentHealth(1);
            qDebug() << "ProbabilityEnemy 将玩家血量强制设为1";
            continue;
        }

        // 对敌人：强制回满血（不包括其他ProbabilityEnemy）
        if (Enemy* enemy = dynamic_cast<Enemy*>(item)) {
            // 跳过其他概率论敌人
            if (dynamic_cast<ProbabilityEnemy*>(enemy))
                continue;

            int currentHealth = enemy->getHealth();
            int maxHealth = enemy->getMaxHealth();

            // 只有未满血的敌人才需要回满并显示文字
            if (currentHealth < maxHealth) {
                // 回满血
                enemy->setCurrentHealth(maxHealth);

                // 显示绿色跟随文字 "恢复至满额状态！"（位置比普通回血文字更高，防止重叠）
                QGraphicsTextItem* healText = new QGraphicsTextItem("恢复至满额状态！");
                QFont font;
                font.setPointSize(14);
                font.setBold(true);
                healText->setFont(font);
                healText->setDefaultTextColor(QColor(0, 200, 0));                // 绿色
                healText->setPos(enemy->pos().x() + 20, enemy->pos().y() - 55);  // 比普通回血文字高25像素
                healText->setZValue(300);
                scene()->addItem(healText);

                // 创建控制器来管理文字跟随和清理
                new HealTextController(enemy, healText);

                qDebug() << "ProbabilityEnemy 将敌人血量回满至" << maxHealth;
            }
        }
    }

    // 在屏幕最上方显示蓝紫色渐变文字 "概率爆炸！盛宴降临！"
    showExplosionText();
}

void ProbabilityEnemy::showExplosionText() {
    if (!scene())
        return;

    // 创建文字
    QGraphicsTextItem* textItem = new QGraphicsTextItem();

    // 使用HTML设置蓝紫色渐变效果
    QString htmlText =
        "<span style='font-size: 32px; font-weight: bold;'>"
        "<span style='color: #4169E1;'>概</span>"
        "<span style='color: #5A5FD6;'>率</span>"
        "<span style='color: #7B68EE;'>爆</span>"
        "<span style='color: #8A2BE2;'>炸</span>"
        "<span style='color: #9932CC;'>！</span>"
        "<span style='color: #8A2BE2;'>盛</span>"
        "<span style='color: #7B68EE;'>宴</span>"
        "<span style='color: #5A5FD6;'>降</span>"
        "<span style='color: #4169E1;'>临</span>"
        "<span style='color: #5A5FD6;'>！</span>"
        "</span>";
    textItem->setHtml(htmlText);

    // 设置位置在屏幕顶部中央
    qreal textWidth = textItem->boundingRect().width();
    textItem->setPos((800 - textWidth) / 2, 50);  // 800是场景宽度
    textItem->setZValue(500);                     // 确保在最上层

    scene()->addItem(textItem);

    // 使用 QPointer 保护指针
    QPointer<QGraphicsTextItem> textPtr(textItem);
    QPointer<QGraphicsScene> scenePtr(scene());

    // 3秒后删除文字，使用 scenePtr 作为上下文对象（因为this在爆炸后会被删除）
    QTimer::singleShot(3000, scenePtr.data(), [textPtr, scenePtr]() {
        if (textPtr) {
            if (scenePtr && textPtr->scene() == scenePtr) {
                scenePtr->removeItem(textPtr.data());
            }
            delete textPtr.data();
        }
    });
}

void ProbabilityEnemy::pauseTimers() {
    Enemy::pauseTimers();

    if (m_growthTimer && m_growthTimer->isActive()) {
        m_growthTimer->stop();
    }
    if (m_blinkTimer && m_blinkTimer->isActive()) {
        m_blinkTimer->stop();
    }
    if (m_explodeTimer && m_explodeTimer->isActive()) {
        m_explodeTimer->stop();
    }
    if (m_contactTimer && m_contactTimer->isActive()) {
        m_contactTimer->stop();
    }
}

void ProbabilityEnemy::resumeTimers() {
    Enemy::resumeTimers();

    // 恢复接触检测定时器
    if (m_contactTimer && !m_exploded) {
        m_contactTimer->start(CONTACT_CHECK_INTERVAL);
    }

    if (!m_isBlinking && !m_exploded) {
        // 还在成长阶段，恢复成长定时器
        if (m_growthTimer) {
            m_growthTimer->start(GROWTH_UPDATE_INTERVAL);
        }
    } else if (m_isBlinking && !m_exploded) {
        // 在闪烁阶段，恢复闪烁和爆炸定时器
        if (m_blinkTimer) {
            m_blinkTimer->start(BLINK_INTERVAL);
        }
        if (m_explodeTimer) {
            // 简单处理：给一个短时间爆炸
            m_explodeTimer->start(500);
        }
    }
}
