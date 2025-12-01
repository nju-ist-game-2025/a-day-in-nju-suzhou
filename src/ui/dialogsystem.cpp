#include "dialogsystem.h"
#include <QDebug>
#include <QFile>
#include <QFont>
#include <QLinearGradient>
#include <QPainter>
#include "../core/resourcefactory.h"
#include "../entities/level_3/teacherboss.h"

DialogSystem::DialogSystem(QGraphicsScene* scene, QObject* parent)
    : QObject(parent), m_scene(scene) {
}

DialogSystem::~DialogSystem() {
    removeEventFilter();
    cleanupDialogUI();
    cleanupBossSprite();

    if (m_levelTextTimer) {
        m_levelTextTimer->stop();
        delete m_levelTextTimer;
        m_levelTextTimer = nullptr;
    }
}

void DialogSystem::installEventFilter() {
    if (m_scene) {
        m_scene->removeEventFilter(this);
        m_scene->installEventFilter(this);
    }
}

void DialogSystem::removeEventFilter() {
    if (m_scene) {
        m_scene->removeEventFilter(this);
    }
}

bool DialogSystem::eventFilter(QObject* watched, QEvent* event) {
    // 如果游戏暂停，不拦截任何事件
    if (m_isPaused) {
        return QObject::eventFilter(watched, event);
    }

    // 只在对话进行中处理事件
    if (watched == m_scene && !m_isStoryFinished) {
        if (event->type() == QEvent::GraphicsSceneMousePress ||
            event->type() == QEvent::KeyPress) {
            if (event->type() == QEvent::KeyPress) {
                auto* keyEvent = dynamic_cast<QKeyEvent*>(event);
                // ESC键不拦截，让它传递给GameView处理暂停菜单
                if (keyEvent->key() == Qt::Key_Escape) {
                    return false;
                }
                // Ctrl+S 跳过剧情
                if (!m_skipRequested &&
                    keyEvent->modifiers().testFlag(Qt::ControlModifier) &&
                    keyEvent->key() == Qt::Key_S) {
                    m_skipRequested = true;
                    finishStory();
                    return true;
                }
                // Enter/Space 继续对话
                if (keyEvent->key() == Qt::Key_Return || keyEvent->key() == Qt::Key_Enter ||
                    keyEvent->key() == Qt::Key_Space) {
                    onDialogClicked();
                    return true;
                }
            } else if (event->type() == QEvent::GraphicsSceneMousePress) {
                onDialogClicked();
                return true;
            }
        }
    }
    return QObject::eventFilter(watched, event);
}

void DialogSystem::showStoryDialog(const QStringList& dialogs, bool isBossDialog, const QString& customBackground) {
    m_currentDialogs = dialogs;
    m_currentDialogIndex = 0;
    m_isBossDialog = isBossDialog;
    m_isStoryFinished = false;
    m_isTeacherBossInitialDialog = false;

    emit dialogStarted();

    // Boss对话时请求暂停敌人
    if (isBossDialog) {
        emit requestPauseEnemies();
    }

    // 检查是否使用透明背景
    bool useTransparentBackground = (customBackground == "transparent");

    QString imagePath;
    if (!useTransparentBackground) {
        if (!customBackground.isEmpty()) {
            imagePath = customBackground;
        } else {
            // 使用默认背景
            if (m_levelNumber == 1)
                imagePath = "assets/galgame/l1.png";
            else if (m_levelNumber == 2)
                imagePath = "assets/galgame/l2.png";
            else
                imagePath = "assets/galgame/l3.png";
        }
    }

    createDialogElements(useTransparentBackground, imagePath);

    // 第三关Boss对话时显示cow图片
    if (m_levelNumber == 3 && isBossDialog) {
        setupTeacherBossCowSprite(isBossDialog);
    }

    // 安装事件过滤器
    installEventFilter();

    // 显示第一句对话
    nextDialog();
}

void DialogSystem::setupTeacherBossCowSprite(bool isBossDialog) {
    if (!isBossDialog || m_levelNumber != 3)
        return;

    // 第三关Boss初始对话（TeacherBoss还未创建）：cow飞入动画
    if (!m_teacherBoss) {
        m_isTeacherBossInitialDialog = true;
        createBossEntryAnimation("cow.png", QPointF(-200, 180), QPointF(250, 180), 2000);
    }
    // 第三关Boss二三阶段对话或击败后对话：cow直接出现
    else {
        QString cowImageName = m_bossDefeated ? "cowFinal.png" : "cow.png";
        showStaticBossSprite(cowImageName, QPointF(250, 180));
        if (m_teacherBoss) {
            qDebug() << "[DialogSystem] TeacherBoss阶段" << m_teacherBoss->getPhase() << "对话：cow直接出现";
        }
    }
}

void DialogSystem::createDialogElements(bool useTransparentBackground, const QString& imagePath) {
    if (!useTransparentBackground && !imagePath.isEmpty()) {
        // 检查文件是否存在
        QFile file(imagePath);
        if (!file.exists()) {
            qWarning() << "DialogSystem: 图片文件不存在:" << imagePath;
            m_isStoryFinished = true;
            m_isBossDialog = false;
            return;
        }

        // 加载图片
        QPixmap bgPixmap(imagePath);
        if (bgPixmap.isNull()) {
            qWarning() << "DialogSystem: 加载图片失败:" << imagePath;
            m_isStoryFinished = true;
            m_isBossDialog = false;
            return;
        }

        // 创建背景图片项
        m_dialogBox = new QGraphicsPixmapItem(
            bgPixmap.scaled(800, 600, Qt::IgnoreAspectRatio, Qt::SmoothTransformation));
        m_dialogBox->setPos(0, 0);
        m_dialogBox->setZValue(10000);
        m_scene->addItem(m_dialogBox);
    }

    // 创建渐变文字背景
    QPixmap gradientPixmap(800, 250);
    gradientPixmap.fill(Qt::transparent);

    QPainter painter(&gradientPixmap);
    QLinearGradient gradient(0, 0, 0, 250);
    gradient.setColorAt(0.0, QColor(0, 0, 0, 0));
    gradient.setColorAt(0.3, QColor(0, 0, 0, 160));
    gradient.setColorAt(0.7, QColor(0, 0, 0, 200));
    gradient.setColorAt(1.0, QColor(0, 0, 0, 250));

    painter.fillRect(0, 0, 800, 250, gradient);
    painter.end();

    m_textBackground = new QGraphicsPixmapItem(gradientPixmap);
    m_textBackground->setPos(0, 350);
    m_textBackground->setZValue(10001);
    m_scene->addItem(m_textBackground);

    // 创建对话文本
    m_dialogText = new QGraphicsTextItem();
    m_dialogText->setDefaultTextColor(Qt::white);
    m_dialogText->setFont(QFont("Microsoft YaHei", 16, QFont::Bold));
    m_dialogText->setTextWidth(700);
    m_dialogText->setPos(50, 400);
    m_dialogText->setZValue(10002);
    m_scene->addItem(m_dialogText);

    // 创建继续提示
    m_continueHint = new QGraphicsTextItem("点击或按Enter键继续...");
    m_continueHint->setDefaultTextColor(QColor(255, 255, 255, 180));
    m_continueHint->setFont(QFont("Microsoft YaHei", 12, QFont::Light));
    m_continueHint->setPos(580, 550);
    m_continueHint->setZValue(10002);
    m_scene->addItem(m_continueHint);

    // 创建跳过提示
    m_skipHint = new QGraphicsTextItem("Ctrl+S 跳过剧情");
    m_skipHint->setDefaultTextColor(QColor(255, 255, 255, 140));
    m_skipHint->setFont(QFont("Microsoft YaHei", 11, QFont::Light));
    m_skipHint->setPos(30, 550);
    m_skipHint->setZValue(10002);
    m_scene->addItem(m_skipHint);

    // 让渐变背景可点击
    m_textBackground->setFlag(QGraphicsItem::ItemIsFocusable);
    m_textBackground->setAcceptHoverEvents(true);
}

void DialogSystem::nextDialog() {
    if (m_currentDialogIndex >= m_currentDialogs.size()) {
        finishStory();
        return;
    }

    // 第一次显示对话时，检查是否有待执行的对话框背景渐变
    if (m_currentDialogIndex == 0 && !m_pendingFadeDialogBackground.isEmpty()) {
        qDebug() << "[DialogSystem] 执行待处理的对话框背景渐变:" << m_pendingFadeDialogBackground;
        QTimer::singleShot(100, this, [this]() {
            fadeDialogBackgroundTo(m_pendingFadeDialogBackground, m_pendingFadeDialogDuration);
            m_pendingFadeDialogBackground.clear();
            m_pendingFadeDialogDuration = 0;
        });
    }

    // 检查是否需要在当前对话索引处切换背景
    if (m_pendingDialogBackgrounds.contains(m_currentDialogIndex)) {
        QString backgroundPath = m_pendingDialogBackgrounds.value(m_currentDialogIndex);
        qDebug() << "[DialogSystem] 对话中切换背景到:" << backgroundPath;

        QString fullPath = backgroundPath;
        if (!backgroundPath.startsWith("assets/")) {
            fullPath = "assets/background/" + backgroundPath;
            if (!fullPath.endsWith(".png")) {
                fullPath += ".png";
            }
        }
        QPixmap newBg(fullPath);
        if (!newBg.isNull() && m_dialogBox) {
            m_dialogBox->setPixmap(newBg.scaled(m_dialogBox->pixmap().size(),
                                                Qt::KeepAspectRatioByExpanding, Qt::SmoothTransformation));
        }
        m_pendingDialogBackgrounds.remove(m_currentDialogIndex);
    }

    QString currentText = m_currentDialogs[m_currentDialogIndex];
    if (m_dialogText) {
        m_dialogText->setPlainText(currentText);
    } else {
        qWarning() << "DialogSystem: m_dialogText is nullptr";
    }
    m_currentDialogIndex++;

    qDebug() << "DialogSystem: 显示对话:" << m_currentDialogIndex << "/" << m_currentDialogs.size();
}

void DialogSystem::onDialogClicked() {
    if (!m_isStoryFinished) {
        nextDialog();
    }
}

void DialogSystem::finishStory() {
    qDebug() << "DialogSystem: 剧情播放完毕";
    m_skipRequested = false;

    // 清理待切换的背景
    clearPendingDialogBackgrounds();

    // 移除事件过滤器
    removeEventFilter();

    // 清理UI
    cleanupDialogUI();
    cleanupBossSprite();

    m_isStoryFinished = true;

    emit dialogFinished();

    // 根据对话类型发出不同信号
    if (m_isEliteDialog) {
        m_isEliteDialog = false;
        emit eliteDialogFinished();
    } else if (m_isBossDialog) {
        m_isBossDialog = false;
        emit requestResumeEnemies();
        emit bossDialogFinished();
    } else {
        emit storyFinished();
    }
}

void DialogSystem::cleanupDialogUI() {
    if (m_textBackground && m_scene) {
        m_scene->removeItem(m_textBackground);
        delete m_textBackground;
        m_textBackground = nullptr;
    }

    if (m_dialogBox && m_scene) {
        m_scene->removeItem(m_dialogBox);
        delete m_dialogBox;
        m_dialogBox = nullptr;
    }

    if (m_dialogText && m_scene) {
        m_scene->removeItem(m_dialogText);
        delete m_dialogText;
        m_dialogText = nullptr;
    }

    if (m_continueHint && m_scene) {
        m_scene->removeItem(m_continueHint);
        delete m_continueHint;
        m_continueHint = nullptr;
    }

    if (m_skipHint && m_scene) {
        m_scene->removeItem(m_skipHint);
        delete m_skipHint;
        m_skipHint = nullptr;
    }
}

void DialogSystem::cleanupBossSprite() {
    if (m_dialogBossFlyAnimation) {
        m_dialogBossFlyAnimation->stop();
        delete m_dialogBossFlyAnimation;
        m_dialogBossFlyAnimation = nullptr;
    }

    if (m_dialogBossSprite && m_scene) {
        m_scene->removeItem(m_dialogBossSprite);
        delete m_dialogBossSprite;
        m_dialogBossSprite = nullptr;
    }

    m_isTeacherBossInitialDialog = false;
}

void DialogSystem::showLevelStartText(const QString& levelName) {
    QGraphicsTextItem* levelTextItem = new QGraphicsTextItem(levelName);
    levelTextItem->setDefaultTextColor(Qt::red);
    levelTextItem->setFont(QFont("Arial", 36, QFont::Bold));

    int sceneWidth = 800;
    int sceneHeight = 600;
    levelTextItem->setPos(sceneWidth / 2 - 200, sceneHeight / 2 - 60);
    levelTextItem->setZValue(10010);
    m_scene->addItem(levelTextItem);
    m_scene->update();
    qDebug() << "DialogSystem: 关卡文字已添加，Z值:" << levelTextItem->zValue();

    // 停止之前的定时器
    if (m_levelTextTimer) {
        m_levelTextTimer->stop();
        m_levelTextTimer->deleteLater();
    }

    // 2秒后自动移除
    m_levelTextTimer = new QTimer(this);
    m_levelTextTimer->setSingleShot(true);
    QPointer<QGraphicsScene> scenePtr(m_scene);
    QPointer<QGraphicsTextItem> levelTextItemPtr(levelTextItem);
    connect(m_levelTextTimer, &QTimer::timeout, [levelTextItemPtr, scenePtr]() {
        if (levelTextItemPtr) {
            if (scenePtr && levelTextItemPtr->scene() == scenePtr) {
                scenePtr->removeItem(levelTextItemPtr.data());
            }
            delete levelTextItemPtr.data();
            qDebug() << "DialogSystem: 关卡文字已移除";
        }
    });
    m_levelTextTimer->start(2000);
}

void DialogSystem::showPhaseTransitionText(const QString& text, const QColor& color) {
    QGraphicsTextItem* textItem = new QGraphicsTextItem(text);
    textItem->setDefaultTextColor(color);
    textItem->setFont(QFont("Arial", 16, QFont::Bold));

    QRectF textRect = textItem->boundingRect();
    textItem->setPos((800 - textRect.width()) / 2, 280);
    textItem->setZValue(1000);
    m_scene->addItem(textItem);

    // 2秒后自动移除
    QPointer<QGraphicsScene> scenePtr(m_scene);
    QPointer<QGraphicsTextItem> textItemPtr(textItem);
    QTimer::singleShot(2000, this, [textItemPtr, scenePtr]() {
        if (textItemPtr) {
            if (scenePtr && textItemPtr->scene() == scenePtr) {
                scenePtr->removeItem(textItemPtr.data());
            }
            delete textItemPtr.data();
            qDebug() << "DialogSystem: 阶段转换文字已移除";
        }
    });
}

void DialogSystem::showCredits(const QStringList& credits) {
    QGraphicsTextItem* creditsText = new QGraphicsTextItem();
    creditsText->setDefaultTextColor(QColor(102, 0, 153));
    creditsText->setFont(QFont("SimSun", 20, QFont::Bold));

    QString fullText;
    for (const QString& line : credits) {
        fullText += line + "\n\n\n";
    }
    creditsText->setPlainText(fullText);

    QRectF sceneRect = m_scene->sceneRect();
    creditsText->setPos(sceneRect.width() / 2 - creditsText->boundingRect().width() / 2,
                        sceneRect.height() + 100);
    creditsText->setZValue(11000);
    m_scene->addItem(creditsText);

    // 创建滚动动画
    QPropertyAnimation* animation = new QPropertyAnimation(creditsText, "pos");
    animation->setDuration(15000);
    animation->setStartValue(creditsText->pos());
    animation->setEndValue(QPointF(sceneRect.width() / 2 - creditsText->boundingRect().width() / 2,
                                   -creditsText->boundingRect().height() - 100));
    animation->setEasingCurve(QEasingCurve::Linear);

    QPointer<QGraphicsScene> scenePtr(m_scene);
    QPointer<QGraphicsTextItem> creditsTextPtr(creditsText);
    connect(animation, &QPropertyAnimation::finished, [scenePtr, creditsTextPtr, animation]() {
        if (creditsTextPtr) {
            if (scenePtr && creditsTextPtr->scene() == scenePtr) {
                scenePtr->removeItem(creditsTextPtr.data());
            }
            delete creditsTextPtr.data();
        }
        animation->deleteLater();
    });

    animation->start();
}

void DialogSystem::fadeDialogBackgroundTo(const QString& imagePath, int duration) {
    if (!m_scene || !m_dialogBox)
        return;

    QString fullPath = imagePath;
    if (!imagePath.startsWith("assets/")) {
        fullPath = "assets/background/" + imagePath;
        if (!fullPath.endsWith(".png")) {
            fullPath += ".png";
        }
    }

    QPixmap newBg;
    try {
        newBg = ResourceFactory::loadImage(fullPath);
    } catch (const QString& e) {
        qWarning() << "DialogSystem: fadeDialogBackgroundTo 加载图片失败:" << e;
        return;
    }

    if (newBg.isNull()) {
        qWarning() << "DialogSystem: 无法加载图片:" << fullPath;
        return;
    }

    // 创建覆盖层
    QGraphicsPixmapItem* dialogOverlay = new QGraphicsPixmapItem(
        newBg.scaled(800, 600, Qt::IgnoreAspectRatio, Qt::SmoothTransformation));
    dialogOverlay->setPos(0, 0);
    dialogOverlay->setZValue(10000);
    dialogOverlay->setOpacity(0.0);
    m_scene->addItem(dialogOverlay);

    // 渐变动画
    QVariantAnimation* anim = new QVariantAnimation(this);
    anim->setStartValue(0.0);
    anim->setEndValue(1.0);
    anim->setDuration(duration);
    connect(anim, &QVariantAnimation::valueChanged, this, [dialogOverlay](const QVariant& value) {
        if (dialogOverlay)
            dialogOverlay->setOpacity(value.toDouble());
    });

    QPointer<QGraphicsScene> scenePtr(m_scene);
    QGraphicsPixmapItem* dialogBoxPtr = m_dialogBox;
    connect(anim, &QVariantAnimation::finished, this, [scenePtr, dialogBoxPtr, dialogOverlay, newBg]() {
        if (dialogBoxPtr) {
            dialogBoxPtr->setPixmap(newBg.scaled(800, 600, Qt::IgnoreAspectRatio, Qt::SmoothTransformation));
        }
        if (dialogOverlay && scenePtr) {
            scenePtr->removeItem(dialogOverlay);
            delete dialogOverlay;
        }
    });

    anim->start(QAbstractAnimation::DeleteWhenStopped);
    qDebug() << "[DialogSystem] 对话框背景渐变动画开始:" << fullPath;
}

void DialogSystem::setPendingFadeDialogBackground(const QString& path, int duration) {
    m_pendingFadeDialogBackground = path;
    m_pendingFadeDialogDuration = duration;
}

void DialogSystem::setDialogBackgroundChange(int dialogIndex, const QString& backgroundName) {
    m_pendingDialogBackgrounds[dialogIndex] = backgroundName;
}

void DialogSystem::clearPendingDialogBackgrounds() {
    m_pendingDialogBackgrounds.clear();
    m_pendingFadeDialogBackground.clear();
    m_pendingFadeDialogDuration = 0;
}

void DialogSystem::createBossEntryAnimation(const QString& imagePath, QPointF startPos, QPointF endPos, int duration) {
    // 尝试多个可能的路径
    QStringList possiblePaths = {
        imagePath,
        "assets/boss/Teacher/" + imagePath,
        "../assets/boss/Teacher/" + imagePath,
        "../../our_game/assets/boss/Teacher/" + imagePath};

    QPixmap bossPixmap;
    for (const QString& path : possiblePaths) {
        if (QFile::exists(path)) {
            bossPixmap = QPixmap(path);
            break;
        }
    }

    if (bossPixmap.isNull()) {
        qWarning() << "DialogSystem: 无法加载Boss图片:" << imagePath;
        return;
    }

    // 缩放图片
    bossPixmap = bossPixmap.scaled(180, 180, Qt::KeepAspectRatio, Qt::SmoothTransformation);
    m_dialogBossSprite = new QGraphicsPixmapItem(bossPixmap);
    m_dialogBossSprite->setPos(startPos);
    m_dialogBossSprite->setZValue(10001);
    m_scene->addItem(m_dialogBossSprite);

    // 创建飞入动画
    m_dialogBossFlyAnimation = new QPropertyAnimation(this);
    m_dialogBossFlyAnimation->setTargetObject(nullptr);
    m_dialogBossFlyAnimation->setDuration(duration);
    m_dialogBossFlyAnimation->setStartValue(startPos);
    m_dialogBossFlyAnimation->setEndValue(endPos);
    m_dialogBossFlyAnimation->setEasingCurve(QEasingCurve::OutCubic);

    connect(m_dialogBossFlyAnimation, &QPropertyAnimation::valueChanged, this,
            [this](const QVariant& value) {
                if (m_dialogBossSprite) {
                    m_dialogBossSprite->setPos(value.toPointF());
                }
            });

    m_dialogBossFlyAnimation->start();
    qDebug() << "[DialogSystem] Boss入场动画开始";
}

void DialogSystem::showStaticBossSprite(const QString& imagePath, QPointF pos) {
    QStringList possiblePaths = {
        imagePath,
        "assets/boss/Teacher/" + imagePath,
        "../assets/boss/Teacher/" + imagePath,
        "../../our_game/assets/boss/Teacher/" + imagePath};

    QPixmap bossPixmap;
    for (const QString& path : possiblePaths) {
        if (QFile::exists(path)) {
            bossPixmap = QPixmap(path);
            break;
        }
    }

    if (bossPixmap.isNull()) {
        qWarning() << "DialogSystem: 无法加载Boss图片:" << imagePath;
        return;
    }

    bossPixmap = bossPixmap.scaled(180, 180, Qt::KeepAspectRatio, Qt::SmoothTransformation);
    m_dialogBossSprite = new QGraphicsPixmapItem(bossPixmap);
    m_dialogBossSprite->setPos(pos);
    m_dialogBossSprite->setZValue(10001);
    m_scene->addItem(m_dialogBossSprite);
    qDebug() << "[DialogSystem] 静态Boss图片已显示";
}
