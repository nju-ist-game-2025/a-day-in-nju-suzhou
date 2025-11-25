#ifndef CHARACTERSELECTOR_H
#define CHARACTERSELECTOR_H

#include <QHBoxLayout>
#include <QLabel>
#include <QList>
#include <QPushButton>
#include <QString>
#include <QVBoxLayout>
#include <QWidget>

/**
 * @brief 角色选择界面
 * 允许玩家在游戏开始前选择角色
 */
class CharacterSelector : public QWidget {
    Q_OBJECT

   public:
    explicit CharacterSelector(QWidget* parent = nullptr);

    // 获取当前选中的角色图片路径
    QString getSelectedCharacter() const { return m_selectedCharacter; }

    // 获取当前选中的角色名称
    QString getSelectedCharacterName() const { return m_selectedName; }

   signals:
    void characterSelected(const QString& characterPath);  // 选择角色后发出
    void backToMenu();                                     // 返回主菜单

   private slots:
    void onCharacterClicked(int index);
    void onConfirmClicked();
    void onBackClicked();

   private:
    void loadCharacters();
    void updateSelection(int index);

    struct CharacterInfo {
        QString name;       // 角色名称
        QString imagePath;  // 图片路径
        QPushButton* button;
    };

    QList<CharacterInfo> m_characters;
    int m_selectedIndex;
    int m_defaultSelectedIndex;  // 配置文件中默认选中的角色索引
    QString m_selectedCharacter;
    QString m_selectedName;

    QLabel* m_titleLabel;
    QLabel* m_previewLabel;
    QLabel* m_nameLabel;
    QPushButton* m_confirmButton;
    QPushButton* m_backButton;
    QHBoxLayout* m_characterLayout;

   protected:
    void resizeEvent(QResizeEvent* event) override;
};

#endif  // CHARACTERSELECTOR_H
