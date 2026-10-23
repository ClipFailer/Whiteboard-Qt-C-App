#pragma once

#include "items/BaseItem.h"
#include "tools/ToolManager.h"
#include "canvas/CanvasScene.h"
#include <QWidget>
#include <QColor>
#include <QSlider>
#include <QComboBox>
#include <QLabel>
#include <QPushButton>
#include <QButtonGroup>
#include <QScrollArea>

namespace Whiteboard {

class PropertiesPanel : public QWidget {
    Q_OBJECT

public:
    explicit PropertiesPanel(ToolManager* toolManager, CanvasScene* scene, QWidget* parent = nullptr);

    void updateTheme(bool isDark);
    void updateFromSelection();

private slots:
    void onStrokeColorSelected(const QColor& color);
    void onFillColorSelected(const QColor& color);
    void onStrokeWidthChanged(int width);
    void onRoughnessChanged(int roughnessTimesTen);
    void onFillStyleChanged(int index);
    void onStrokeStyleChanged(int index);
    void onOpacityChanged(int opacityPercent);

private:
    void setupUI();
    void applyPropertyChange(const std::function<void(ItemProperties&)>& mutator);
    QWidget* createColorPalette(bool isStroke);

    ToolManager* m_toolManager;
    CanvasScene* m_scene;

    ItemProperties m_currentProps;
    bool m_updatingFromSelection{false};

    // Сохраняем ссылки на область прокрутки и её контейнер, чтобы updateTheme()
    // мог перерисовать их фон без полного пересоздания виджетов.
    QScrollArea* m_scrollArea{nullptr};
    QWidget*     m_container{nullptr};

    // Элементы управления UI
    QPushButton* m_customStrokeBtn{nullptr};
    QPushButton* m_customFillBtn{nullptr};
    QSlider* m_strokeWidthSlider{nullptr};
    QLabel* m_strokeWidthLabel{nullptr};
    QSlider* m_roughnessSlider{nullptr};
    QLabel* m_roughnessLabel{nullptr};
    QComboBox* m_fillStyleCombo{nullptr};
    QComboBox* m_strokeStyleCombo{nullptr};
    QSlider* m_opacitySlider{nullptr};
    QLabel* m_opacityLabel{nullptr};
    QList<QPushButton*> m_strokeColorButtons;
    QList<QPushButton*> m_fillColorButtons;

    // Секция выравнивания и распределения
    QWidget* m_alignBox{nullptr};
    QPushButton* m_btnAlignLeft{nullptr};
    QPushButton* m_btnAlignCenterH{nullptr};
    QPushButton* m_btnAlignRight{nullptr};
    QPushButton* m_btnAlignTop{nullptr};
    QPushButton* m_btnAlignCenterV{nullptr};
    QPushButton* m_btnAlignBottom{nullptr};
    QPushButton* m_btnDistributeH{nullptr};
    QPushButton* m_btnDistributeV{nullptr};

    // Секция типографики
    QWidget* m_typographyBox{nullptr};
    QComboBox* m_fontFamilyCombo{nullptr};
    QComboBox* m_fontSizeCombo{nullptr};
    QPushButton* m_btnAlignTextLeft{nullptr};
    QPushButton* m_btnAlignTextCenter{nullptr};
    QPushButton* m_btnAlignTextRight{nullptr};

    // Секция цветов стикера
    QWidget* m_stickyColorBox{nullptr};
    QList<QPushButton*> m_stickyColorButtons;
};

} // namespace Whiteboard
