#ifndef DIALOGSYSTEM_H
#define DIALOGSYSTEM_H

#include <QGraphicsPixmapItem>
#include <QGraphicsScene>
#include <QGraphicsTextItem>
#include <QKeyEvent>
#include <QMap>
#include <QObject>
#include <QPointer>
#include <QPropertyAnimation>
#include <QString>
#include <QStringList>
#include <QTimer>
#include <QVariantAnimation>

class Player;
class TeacherBoss;

/**
 * @brief 对话系统类 - 管理游戏中的所有对话、剧情和相关UI
 *
 * 职责：
 *   - 显示和管理对话框（galgame风格）
 *   - 处理对话推进和跳过
 *   - 管理对话期间的背景图片和动画
 *   - Boss入场动画（如TeacherBoss的cow飞入动画）
 *   - 关卡开始文字显示
 *   - 阶段转换文字显示
 *   - 滚动字幕（Credits）
 *   - 事件过滤（处理对话输入）
 */
class DialogSystem : public QObject {
    Q_OBJECT

   public:
    explicit DialogSystem(QGraphicsScene* scene, QObject* parent = nullptr);
    ~DialogSystem() override;

    /**
     * @brief 安装事件过滤器到场景
     */
    void installEventFilter();

    /**
     * @brief 移除事件过滤器
     */
    void removeEventFilter();

    /**
     * @brief 设置暂停状态（暂停时不处理对话事件）
     */
    void setPaused(bool paused) { m_isPaused = paused; }

    /**
     * @brief 设置关卡号（用于选择默认对话背景）
     */
    void setLevelNumber(int level) { m_levelNumber = level; }

    /**
     * @brief 设置TeacherBoss引用（用于判断对话类型）
     */
    void setTeacherBoss(TeacherBoss* boss) { m_teacherBoss = boss; }

    /**
     * @brief 设置Boss已击败标志（用于选择cow图片）
     */
    void setBossDefeated(bool defeated) { m_bossDefeated = defeated; }

    /**
     * @brief 显示剧情对话
     * @param dialogs 对话内容列表
     * @param isBossDialog 是否为Boss对话
     * @param customBackground 自定义背景路径（空则使用默认，"transparent"为透明）
     */
    void showStoryDialog(const QStringList& dialogs, bool isBossDialog = false, const QString& customBackground = QString());

    /**
     * @brief 设置精英房间对话模式
     */
    void setEliteDialog(bool value) { m_isEliteDialog = value; }

    /**
     * @brief 推进到下一句对话
     */
    void nextDialog();

    /**
     * @brief 处理对话点击事件
     */
    void onDialogClicked();

    /**
     * @brief 结束当前剧情
     */
    void finishStory();

    /**
     * @brief 显示关卡开始文字
     * @param levelName 关卡名称
     */
    void showLevelStartText(const QString& levelName);

    /**
     * @brief 显示阶段转换文字
     * @param text 显示的文字
     * @param color 文字颜色
     */
    void showPhaseTransitionText(const QString& text, const QColor& color = QColor(75, 0, 130));

    /**
     * @brief 显示滚动字幕
     * @param credits 字幕内容列表
     */
    void showCredits(const QStringList& credits);

    /**
     * @brief 对话框背景渐变
     * @param imagePath 目标图片路径
     * @param duration 渐变持续时间（毫秒）
     */
    void fadeDialogBackgroundTo(const QString& imagePath, int duration);

    /**
     * @brief 设置待执行的对话框背景渐变（在对话显示后执行）
     */
    void setPendingFadeDialogBackground(const QString& path, int duration);

    /**
     * @brief 设置对话中的背景切换（在指定对话索引时切换）
     */
    void setDialogBackgroundChange(int dialogIndex, const QString& backgroundName);

    /**
     * @brief 清理待切换的对话背景
     */
    void clearPendingDialogBackgrounds();

    // ========== 状态查询 ==========
    bool isStoryFinished() const { return m_isStoryFinished; }
    bool isBossDialog() const { return m_isBossDialog; }
    bool isEliteDialog() const { return m_isEliteDialog; }
    bool isTeacherBossInitialDialog() const { return m_isTeacherBossInitialDialog; }
    bool isDialogActive() const { return !m_isStoryFinished; }

   protected:
    /**
     * @brief 事件过滤器处理对话输入
     */
    bool eventFilter(QObject* watched, QEvent* event) override;

    // ========== Boss入场动画支持 ==========
    /**
     * @brief 设置TeacherBoss初始对话模式（启用cow入场动画）
     */
    void setTeacherBossInitialDialog(bool value) { m_isTeacherBossInitialDialog = value; }

    /**
     * @brief 创建Boss入场动画（cow飞入）
     * @param imagePath Boss图片路径
     * @param startPos 起始位置
     * @param endPos 结束位置
     * @param duration 动画时长（毫秒）
     */
    void createBossEntryAnimation(const QString& imagePath, QPointF startPos, QPointF endPos, int duration = 2000);

    /**
     * @brief 显示静态Boss图片（无动画）
     * @param imagePath Boss图片路径
     * @param pos 显示位置
     */
    void showStaticBossSprite(const QString& imagePath, QPointF pos);

    /**
     * @brief 清理Boss角色图片
     */
    void cleanupBossSprite();

    /**
     * @brief 设置跳过请求标记
     */
    void setSkipRequested(bool value) { m_skipRequested = value; }
    bool isSkipRequested() const { return m_skipRequested; }

   signals:
    /**
     * @brief 对话开始信号
     */
    void dialogStarted();

    /**
     * @brief 对话结束信号
     */
    void dialogFinished();

    /**
     * @brief 剧情结束信号（非Boss对话结束时发出）
     */
    void storyFinished();

    /**
     * @brief Boss对话结束信号
     */
    void bossDialogFinished();

    /**
     * @brief 精英对话结束信号
     */
    void eliteDialogFinished();

    /**
     * @brief 请求暂停敌人定时器（Boss对话开始时）
     */
    void requestPauseEnemies();

    /**
     * @brief 请求恢复敌人定时器（Boss对话结束时）
     */
    void requestResumeEnemies();

   private:
    /**
     * @brief 清理对话UI元素
     */
    void cleanupDialogUI();

    /**
     * @brief 创建对话框背景和文本元素
     * @param useTransparentBackground 是否使用透明背景
     * @param imagePath 背景图片路径
     */
    void createDialogElements(bool useTransparentBackground, const QString& imagePath);

    /**
     * @brief 创建第三关Boss的cow入场动画或静态图片
     * @param isBossDialog 是否为Boss对话
     */
    void setupTeacherBossCowSprite(bool isBossDialog);

    QGraphicsScene* m_scene = nullptr;

    // 关卡和Boss引用
    int m_levelNumber = 1;
    TeacherBoss* m_teacherBoss = nullptr;
    bool m_bossDefeated = false;
    bool m_isPaused = false;

    // 对话UI元素
    QGraphicsPixmapItem* m_dialogBox = nullptr;       // 对话背景图片
    QGraphicsTextItem* m_dialogText = nullptr;        // 对话文本
    QGraphicsTextItem* m_continueHint = nullptr;      // 继续提示
    QGraphicsTextItem* m_skipHint = nullptr;          // 跳过提示
    QGraphicsPixmapItem* m_textBackground = nullptr;  // 渐变文字背景

    // 对话状态
    QStringList m_currentDialogs;   // 当前对话内容
    int m_currentDialogIndex = 0;   // 当前对话索引
    bool m_isStoryFinished = true;  // 剧情是否已结束
    bool m_isBossDialog = false;    // 是否为Boss对话
    bool m_skipRequested = false;   // 是否请求跳过
    bool m_isEliteDialog = false;   // 是否为精英房间对话

    // TeacherBoss相关
    bool m_isTeacherBossInitialDialog = false;               // 是否为TeacherBoss初始对话
    QGraphicsPixmapItem* m_dialogBossSprite = nullptr;       // Boss入场角色图片
    QPropertyAnimation* m_dialogBossFlyAnimation = nullptr;  // Boss飞入动画

    // 对话背景切换相关
    QMap<int, QString> m_pendingDialogBackgrounds;  // 待切换的对话背景
    QString m_pendingFadeDialogBackground;          // 待渐变的背景路径
    int m_pendingFadeDialogDuration = 0;            // 渐变持续时间

    // 关卡文字显示定时器
    QTimer* m_levelTextTimer = nullptr;
};

#endif  // DIALOGSYSTEM_H
