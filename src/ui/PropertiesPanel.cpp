#include "ui/PropertiesPanel.h"
#include "commands/ChangePropertyCommand.h"
#include "items/TextItem.h"
#include "items/StickyNoteItem.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QGridLayout>
#include <QGroupBox>
#include <QColorDialog>
#include <QScrollArea>

namespace Whiteboard {

namespace {

const QList<QColor> StrokeColors = {
    QColor("#1e1e1e"), QColor("#e03131"), QColor("#2f9e44"),
    QColor("#1971c2"), QColor("#f08c00"), QColor("#9c36b5"),
    QColor("#868e96"), QColor("#ffffff")
};

const QList<QColor> FillColors = {
    Qt::transparent,   QColor("#ffc9c9"), QColor("#b2f2bb"),
    QColor("#a5d8ff"), QColor("#ffec99"), QColor("#eebefa"),
    QColor("#dee2e6"), QColor("#ffffff")
};

} // namespace

PropertiesPanel::PropertiesPanel(ToolManager* toolManager, CanvasScene* scene, QWidget* parent)
    : QWidget(parent)
    , m_toolManager(toolManager)
    , m_scene(scene)
    , m_currentProps(toolManager->defaultProperties())
{
    setFixedWidth(240);
    setupUI();
    connect(m_scene, &QGraphicsScene::selectionChanged, this, &PropertiesPanel::updateFromSelection);
    updateFromSelection();
    updateTheme(false);
}

void PropertiesPanel::setupUI()
{
    auto* mainLayout = new QVBoxLayout(this);
    mainLayout->setContentsMargins(10, 10, 10, 10);
    mainLayout->setSpacing(12);

    auto* scrollArea = new QScrollArea(this);
    m_scrollArea = scrollArea;
    scrollArea->setWidgetResizable(true);
    scrollArea->setFrameShape(QFrame::NoFrame);

    auto* container = new QWidget();
    m_container = container;
    auto* layout = new QVBoxLayout(container);
    layout->setContentsMargins(0, 0, 0, 0);
    layout->setSpacing(12);

    // 1. Цвет обводки
    auto* strokeBox = new QGroupBox(QStringLiteral("Stroke Color"), container);
    auto* strokeLayout = new QVBoxLayout(strokeBox);
    strokeLayout->addWidget(createColorPalette(true));
    m_customStrokeBtn = new QPushButton(QStringLiteral("Custom Color..."), strokeBox);
    connect(m_customStrokeBtn, &QPushButton::clicked, this, [this]() {
        QColor col = QColorDialog::getColor(m_currentProps.strokeColor, this, QStringLiteral("Select Stroke Color"));
        if (col.isValid()) {
            onStrokeColorSelected(col);
        }
    });
    strokeLayout->addWidget(m_customStrokeBtn);
    layout->addWidget(strokeBox);

    // 2. Цвет заливки
    auto* fillBox = new QGroupBox(QStringLiteral("Background Fill"), container);
    auto* fillLayout = new QVBoxLayout(fillBox);
    fillLayout->addWidget(createColorPalette(false));
    m_customFillBtn = new QPushButton(QStringLiteral("Custom Color..."), fillBox);
    connect(m_customFillBtn, &QPushButton::clicked, this, [this]() {
        QColor col = QColorDialog::getColor(m_currentProps.fillColor, this, QStringLiteral("Select Fill Color"),
                                            QColorDialog::ShowAlphaChannel);
        if (col.isValid()) {
            onFillColorSelected(col);
        }
    });
    fillLayout->addWidget(m_customFillBtn);
    layout->addWidget(fillBox);

    // 3. Стиль заливки
    auto* fillStyleBox = new QGroupBox(QStringLiteral("Fill Style"), container);
    auto* fsLayout = new QVBoxLayout(fillStyleBox);
    m_fillStyleCombo = new QComboBox(fillStyleBox);
    m_fillStyleCombo->addItem(QStringLiteral("None"), static_cast<int>(FillStyle::None));
    m_fillStyleCombo->addItem(QStringLiteral("Solid"), static_cast<int>(FillStyle::Solid));
    m_fillStyleCombo->addItem(QStringLiteral("Hachure"), static_cast<int>(FillStyle::Hachure));
    m_fillStyleCombo->addItem(QStringLiteral("Cross-Hachure"), static_cast<int>(FillStyle::CrossHachure));
    connect(m_fillStyleCombo, QOverload<int>::of(&QComboBox::currentIndexChanged), this, &PropertiesPanel::onFillStyleChanged);
    fsLayout->addWidget(m_fillStyleCombo);
    layout->addWidget(fillStyleBox);

    // 4. Ширина обводки
    auto* widthBox = new QGroupBox(QStringLiteral("Stroke Width"), container);
    auto* widthLayout = new QVBoxLayout(widthBox);
    auto* widthHeader = new QHBoxLayout();
    m_strokeWidthLabel = new QLabel(QStringLiteral("2 px"), widthBox);
    widthHeader->addWidget(new QLabel(QStringLiteral("Width:"), widthBox));
    widthHeader->addStretch();
    widthHeader->addWidget(m_strokeWidthLabel);
    widthLayout->addLayout(widthHeader);

    m_strokeWidthSlider = new QSlider(Qt::Horizontal, widthBox);
    m_strokeWidthSlider->setRange(1, 10);
    m_strokeWidthSlider->setValue(2);
    connect(m_strokeWidthSlider, &QSlider::valueChanged, this, &PropertiesPanel::onStrokeWidthChanged);
    widthLayout->addWidget(m_strokeWidthSlider);
    layout->addWidget(widthBox);

    // 5. Стиль обводки
    auto* strokeStyleBox = new QGroupBox(QStringLiteral("Stroke Style"), container);
    auto* ssLayout = new QVBoxLayout(strokeStyleBox);
    m_strokeStyleCombo = new QComboBox(strokeStyleBox);
    m_strokeStyleCombo->addItem(QStringLiteral("Solid"), static_cast<int>(Qt::SolidLine));
    m_strokeStyleCombo->addItem(QStringLiteral("Dashed"), static_cast<int>(Qt::DashLine));
    m_strokeStyleCombo->addItem(QStringLiteral("Dotted"), static_cast<int>(Qt::DotLine));
    connect(m_strokeStyleCombo, QOverload<int>::of(&QComboBox::currentIndexChanged), this, &PropertiesPanel::onStrokeStyleChanged);
    ssLayout->addWidget(m_strokeStyleCombo);
    layout->addWidget(strokeStyleBox);

    // 6. Шероховатость (0 - 3)
    auto* roughBox = new QGroupBox(QStringLiteral("Roughness"), container);
    auto* roughLayout = new QVBoxLayout(roughBox);
    auto* roughHeader = new QHBoxLayout();
    m_roughnessLabel = new QLabel(QStringLiteral("1.5"), roughBox);
    roughHeader->addWidget(new QLabel(QStringLiteral("Sloppiness:"), roughBox));
    roughHeader->addStretch();
    roughHeader->addWidget(m_roughnessLabel);
    roughLayout->addLayout(roughHeader);

    m_roughnessSlider = new QSlider(Qt::Horizontal, roughBox);
    m_roughnessSlider->setRange(0, 30);
    m_roughnessSlider->setValue(15);
    connect(m_roughnessSlider, &QSlider::valueChanged, this, &PropertiesPanel::onRoughnessChanged);
    roughLayout->addWidget(m_roughnessSlider);
    layout->addWidget(roughBox);

    // 7. Непрозрачность
    auto* opacityBox = new QGroupBox(QStringLiteral("Opacity"), container);
    auto* opLayout = new QVBoxLayout(opacityBox);
    auto* opHeader = new QHBoxLayout();
    m_opacityLabel = new QLabel(QStringLiteral("100%"), opacityBox);
    opHeader->addWidget(new QLabel(QStringLiteral("Transparency:"), opacityBox));
    opHeader->addStretch();
    opHeader->addWidget(m_opacityLabel);
    opLayout->addLayout(opHeader);

    m_opacitySlider = new QSlider(Qt::Horizontal, opacityBox);
    m_opacitySlider->setRange(10, 100);
    m_opacitySlider->setValue(100);
    connect(m_opacitySlider, &QSlider::valueChanged, this, &PropertiesPanel::onOpacityChanged);
    opLayout->addWidget(m_opacitySlider);
    layout->addWidget(opacityBox);

    // 8. Секция выравнивания и распределения
    auto* alignBox = new QGroupBox(QStringLiteral("Align & Distribute"), container);
    m_alignBox = alignBox;
    auto* alignLayout = new QVBoxLayout(alignBox);

    auto makeBtn = [](const QString& text, const QString& tip) {
        auto* b = new QPushButton(text);
        b->setToolTip(tip);
        b->setFixedHeight(28);
        return b;
    };

    auto* rowAlign = new QHBoxLayout();
    rowAlign->setSpacing(3);
    m_btnAlignLeft = makeBtn(QStringLiteral("⇤"), QStringLiteral("Выровнять по левому краю"));
    m_btnAlignCenterH = makeBtn(QStringLiteral("⇥⇤"), QStringLiteral("Выровнять по горизонтальному центру"));
    m_btnAlignRight = makeBtn(QStringLiteral("⇥"), QStringLiteral("Выровнять по правому краю"));
    m_btnAlignTop = makeBtn(QStringLiteral("⤒"), QStringLiteral("Выровнять по верху"));
    m_btnAlignCenterV = makeBtn(QStringLiteral("↕"), QStringLiteral("Выровнять по вертикальному центру"));
    m_btnAlignBottom = makeBtn(QStringLiteral("⤓"), QStringLiteral("Выровнять по низу"));

    rowAlign->addWidget(m_btnAlignLeft);
    rowAlign->addWidget(m_btnAlignCenterH);
    rowAlign->addWidget(m_btnAlignRight);
    rowAlign->addWidget(m_btnAlignTop);
    rowAlign->addWidget(m_btnAlignCenterV);
    rowAlign->addWidget(m_btnAlignBottom);
    alignLayout->addLayout(rowAlign);

    auto* rowDist = new QHBoxLayout();
    rowDist->setSpacing(4);
    m_btnDistributeH = makeBtn(QStringLiteral("↔ Распред. H"), QStringLiteral("Равное распределение по горизонтали"));
    m_btnDistributeV = makeBtn(QStringLiteral("↕ Распред. V"), QStringLiteral("Равное распределение по вертикали"));
    rowDist->addWidget(m_btnDistributeH);
    rowDist->addWidget(m_btnDistributeV);
    alignLayout->addLayout(rowDist);

    connect(m_btnAlignLeft, &QPushButton::clicked, m_scene, &CanvasScene::alignLeft);
    connect(m_btnAlignCenterH, &QPushButton::clicked, m_scene, &CanvasScene::alignCenterH);
    connect(m_btnAlignRight, &QPushButton::clicked, m_scene, &CanvasScene::alignRight);
    connect(m_btnAlignTop, &QPushButton::clicked, m_scene, &CanvasScene::alignTop);
    connect(m_btnAlignCenterV, &QPushButton::clicked, m_scene, &CanvasScene::alignCenterV);
    connect(m_btnAlignBottom, &QPushButton::clicked, m_scene, &CanvasScene::alignBottom);
    connect(m_btnDistributeH, &QPushButton::clicked, m_scene, &CanvasScene::distributeHorizontally);
    connect(m_btnDistributeV, &QPushButton::clicked, m_scene, &CanvasScene::distributeVertically);
    layout->addWidget(alignBox);

    // 9. Секция типографики (Шрифт, размер, выравнивание текста)
    auto* typoBox = new QGroupBox(QStringLiteral("Typography"), container);
    m_typographyBox = typoBox;
    auto* typoLayout = new QVBoxLayout(typoBox);

    m_fontFamilyCombo = new QComboBox(typoBox);
    m_fontFamilyCombo->addItem(QStringLiteral("Рукописный"), QStringLiteral("Segoe Print"));
    m_fontFamilyCombo->addItem(QStringLiteral("Sans-serif"), QStringLiteral("Segoe UI"));
    m_fontFamilyCombo->addItem(QStringLiteral("Моноширинный"), QStringLiteral("Cascadia Code"));
    typoLayout->addWidget(m_fontFamilyCombo);

    auto* fontRow = new QHBoxLayout();
    m_fontSizeCombo = new QComboBox(typoBox);
    m_fontSizeCombo->addItem(QStringLiteral("S (14px)"), 14);
    m_fontSizeCombo->addItem(QStringLiteral("M (20px)"), 20);
    m_fontSizeCombo->addItem(QStringLiteral("L (28px)"), 28);
    m_fontSizeCombo->addItem(QStringLiteral("XL (36px)"), 36);
    fontRow->addWidget(m_fontSizeCombo);

    m_btnAlignTextLeft = makeBtn(QStringLiteral("Left"), QStringLiteral("По левому краю"));
    m_btnAlignTextCenter = makeBtn(QStringLiteral("Center"), QStringLiteral("По центру"));
    m_btnAlignTextRight = makeBtn(QStringLiteral("Right"), QStringLiteral("По правому краю"));
    fontRow->addWidget(m_btnAlignTextLeft);
    fontRow->addWidget(m_btnAlignTextCenter);
    fontRow->addWidget(m_btnAlignTextRight);
    typoLayout->addLayout(fontRow);

    connect(m_fontFamilyCombo, QOverload<int>::of(&QComboBox::currentIndexChanged), this, [this](int idx) {
        if (m_updatingFromSelection) return;
        QString family = m_fontFamilyCombo->itemData(idx).toString();
        for (auto* it : m_scene->selectedWhiteboardItems()) {
            if (auto* txt = dynamic_cast<TextItem*>(it)) {
                QFont f = txt->font();
                f.setFamily(family);
                txt->setFont(f);
            } else if (auto* note = dynamic_cast<StickyNoteItem*>(it)) {
                QFont f = note->font();
                f.setFamily(family);
                note->setFont(f);
            }
        }
    });

    connect(m_fontSizeCombo, QOverload<int>::of(&QComboBox::currentIndexChanged), this, [this](int idx) {
        if (m_updatingFromSelection) return;
        int size = m_fontSizeCombo->itemData(idx).toInt();
        for (auto* it : m_scene->selectedWhiteboardItems()) {
            if (auto* txt = dynamic_cast<TextItem*>(it)) {
                txt->setFontSize(size);
            } else if (auto* note = dynamic_cast<StickyNoteItem*>(it)) {
                QFont f = note->font();
                f.setPointSize(size);
                note->setFont(f);
            }
        }
    });

    connect(m_btnAlignTextLeft, &QPushButton::clicked, this, [this]() {
        for (auto* it : m_scene->selectedWhiteboardItems()) {
            if (auto* note = dynamic_cast<StickyNoteItem*>(it)) {
                note->setTextAlignment(Qt::AlignLeft | Qt::AlignVCenter);
            }
        }
    });
    connect(m_btnAlignTextCenter, &QPushButton::clicked, this, [this]() {
        for (auto* it : m_scene->selectedWhiteboardItems()) {
            if (auto* note = dynamic_cast<StickyNoteItem*>(it)) {
                note->setTextAlignment(Qt::AlignCenter);
            }
        }
    });
    connect(m_btnAlignTextRight, &QPushButton::clicked, this, [this]() {
        for (auto* it : m_scene->selectedWhiteboardItems()) {
            if (auto* note = dynamic_cast<StickyNoteItem*>(it)) {
                note->setTextAlignment(Qt::AlignRight | Qt::AlignVCenter);
            }
        }
    });
    layout->addWidget(typoBox);

    // 10. Палитра цветов стикера
    auto* stickyBox = new QGroupBox(QStringLiteral("Sticky Note Colors"), container);
    m_stickyColorBox = stickyBox;
    auto* stickyLayout = new QHBoxLayout(stickyBox);
    stickyLayout->setSpacing(6);
    for (const auto& color : StickyNoteItem::pastelPalette()) {
        auto* btn = new QPushButton(stickyBox);
        btn->setFixedSize(28, 28);
        btn->setCursor(Qt::PointingHandCursor);
        btn->setStyleSheet(QStringLiteral("QPushButton { background-color: %1; border: 1.5px solid %2; border-radius: 6px; }")
                               .arg(color.name(), color.darker(120).name()));
        connect(btn, &QPushButton::clicked, this, [this, color]() {
            for (auto* it : m_scene->selectedWhiteboardItems()) {
                if (auto* note = dynamic_cast<StickyNoteItem*>(it)) {
                    note->setNoteColor(color);
                }
            }
        });
        m_stickyColorButtons.append(btn);
        stickyLayout->addWidget(btn);
    }
    layout->addWidget(stickyBox);

    layout->addStretch();
    scrollArea->setWidget(container);
    mainLayout->addWidget(scrollArea);
}

QWidget* PropertiesPanel::createColorPalette(bool isStroke)
{
    auto* widget = new QWidget(this);
    auto* grid = new QGridLayout(widget);
    grid->setContentsMargins(0, 0, 0, 0);
    grid->setSpacing(4);

    const auto& colors = isStroke ? StrokeColors : FillColors;
    int row = 0;
    int col = 0;

    for (const auto& color : colors) {
        auto* btn = new QPushButton(widget);
        btn->setFixedSize(22, 22);
        btn->setCursor(Qt::PointingHandCursor);

        QString style;
        if (color == Qt::transparent) {
            style = QStringLiteral("QPushButton { background: #ffffff; border: 1.5px dashed #adb5bd; border-radius: 4px; }");
        } else {
            style = QStringLiteral("QPushButton { background-color: %1; border: 1px solid #ced4da; border-radius: 4px; }")
                        .arg(color.name());
        }
        btn->setStyleSheet(style);

        if (isStroke) {
            m_strokeColorButtons.append(btn);
            connect(btn, &QPushButton::clicked, this, [this, color]() { onStrokeColorSelected(color); });
        } else {
            m_fillColorButtons.append(btn);
            connect(btn, &QPushButton::clicked, this, [this, color]() { onFillColorSelected(color); });
        }

        grid->addWidget(btn, row, col);
        col++;
        if (col >= 4) {
            col = 0;
            row++;
        }
    }

    return widget;
}

void PropertiesPanel::updateFromSelection()
{
    m_updatingFromSelection = true;

    auto selected = m_scene->selectedWhiteboardItems();
    if (!selected.isEmpty()) {
        m_currentProps = selected.first()->properties();
    } else {
        m_currentProps = m_toolManager->defaultProperties();
    }

    m_strokeWidthSlider->setValue(static_cast<int>(m_currentProps.strokeWidth));
    m_strokeWidthLabel->setText(QStringLiteral("%1 px").arg(static_cast<int>(m_currentProps.strokeWidth)));

    m_roughnessSlider->setValue(static_cast<int>(m_currentProps.roughness * 10.0));
    m_roughnessLabel->setText(QString::number(m_currentProps.roughness, 'f', 1));

    int fsIndex = m_fillStyleCombo->findData(static_cast<int>(m_currentProps.fillStyle));
    if (fsIndex >= 0) m_fillStyleCombo->setCurrentIndex(fsIndex);

    int ssIndex = m_strokeStyleCombo->findData(static_cast<int>(m_currentProps.strokeStyle));
    if (ssIndex >= 0) m_strokeStyleCombo->setCurrentIndex(ssIndex);

    int opVal = static_cast<int>(m_currentProps.opacity * 100.0);
    m_opacitySlider->setValue(opVal);
    m_opacityLabel->setText(QStringLiteral("%1%").arg(opVal));

    // Обновляем доступность выравнивания и распределения
    if (m_alignBox) {
        m_alignBox->setEnabled(selected.size() >= 2);
        if (m_btnDistributeH) m_btnDistributeH->setEnabled(selected.size() >= 3);
        if (m_btnDistributeV) m_btnDistributeV->setEnabled(selected.size() >= 3);
    }

    // Проверяем наличие текста или стикера
    bool hasTextOrSticky = false;
    bool hasSticky = false;
    for (auto* it : selected) {
        if (dynamic_cast<TextItem*>(it)) {
            hasTextOrSticky = true;
        } else if (dynamic_cast<StickyNoteItem*>(it)) {
            hasTextOrSticky = true;
            hasSticky = true;
        }
    }
    if (m_typographyBox) {
        m_typographyBox->setVisible(hasTextOrSticky);
    }
    if (m_stickyColorBox) {
        m_stickyColorBox->setVisible(hasSticky);
    }

    m_updatingFromSelection = false;
}

void PropertiesPanel::applyPropertyChange(const std::function<void(ItemProperties&)>& mutator)
{
    if (m_updatingFromSelection) {
        return;
    }

    mutator(m_currentProps);

    auto selected = m_scene->selectedWhiteboardItems();
    if (!selected.isEmpty()) {
        QList<PropertyChangeRecord> records;
        for (auto* item : selected) {
            ItemProperties oldProps = item->properties();
            ItemProperties newProps = oldProps;
            mutator(newProps);
            records.append({item, oldProps, newProps});
        }
        m_scene->undoStack()->push(new ChangePropertyCommand(records));
    } else {
        m_toolManager->setDefaultProperties(m_currentProps);
    }
}

void PropertiesPanel::onStrokeColorSelected(const QColor& color)
{
    applyPropertyChange([color](ItemProperties& p) { p.strokeColor = color; });
}

void PropertiesPanel::onFillColorSelected(const QColor& color)
{
    applyPropertyChange([color](ItemProperties& p) { p.fillColor = color; });
}

void PropertiesPanel::onStrokeWidthChanged(int width)
{
    m_strokeWidthLabel->setText(QStringLiteral("%1 px").arg(width));
    applyPropertyChange([width](ItemProperties& p) { p.strokeWidth = width; });
}

void PropertiesPanel::onRoughnessChanged(int roughnessTimesTen)
{
    double r = roughnessTimesTen / 10.0;
    m_roughnessLabel->setText(QString::number(r, 'f', 1));
    applyPropertyChange([r](ItemProperties& p) { p.roughness = r; });
}

void PropertiesPanel::onFillStyleChanged(int index)
{
    auto style = static_cast<FillStyle>(m_fillStyleCombo->itemData(index).toInt());
    applyPropertyChange([style](ItemProperties& p) { p.fillStyle = style; });
}

void PropertiesPanel::onStrokeStyleChanged(int index)
{
    auto style = static_cast<Qt::PenStyle>(m_strokeStyleCombo->itemData(index).toInt());
    applyPropertyChange([style](ItemProperties& p) { p.strokeStyle = style; });
}

void PropertiesPanel::onOpacityChanged(int opacityPercent)
{
    double op = opacityPercent / 100.0;
    m_opacityLabel->setText(QStringLiteral("%1%").arg(opacityPercent));
    applyPropertyChange([op](ItemProperties& p) { p.opacity = op; });
}

void PropertiesPanel::updateTheme(bool isDark)
{
    // Цвет фона, который должен быть одинаковым по всему дереву виджетов:
    // PropertiesPanel → QScrollArea → контейнер QWidget → дочерние QGroupBox.
    // Устанавливаем тремя способами, чтобы обойти каждый уровень наследования / переопределения:
    //   1. QPalette на `this`  – воспринимается большинством стилей платформы
    //   2. setAutoFillBackground – принуждает виджет рисовать цвет Window своей палитры
    //   3. QSS «* { background: X }» на контейнере области прокрутки – переопределяет
    //      стиль платформы для каждого дочернего виджета вне зависимости от глубины вложенности.

    const QColor bg      = isDark ? QColor(0x1e, 0x1e, 0x1e) : QColor(0xf8, 0xf9, 0xfa);
    const QColor txt     = isDark ? QColor(0xe0, 0xe0, 0xe0) : QColor(0x21, 0x25, 0x29);
    const QColor subTxt  = isDark ? QColor(0xa0, 0xa0, 0xa0) : QColor(0x6c, 0x75, 0x7d);
    const QColor border  = isDark ? QColor(0x38, 0x38, 0x38) : QColor(0xe9, 0xec, 0xef);
    const QColor inputBg = isDark ? QColor(0x28, 0x28, 0x28) : QColor(0xff, 0xff, 0xff);
    const QColor inputBr = isDark ? QColor(0x48, 0x48, 0x48) : QColor(0xce, 0xd4, 0xda);
    const QColor hover   = isDark ? QColor(0x33, 0x33, 0x33) : QColor(0xe9, 0xec, 0xef);

    // 1. Палитра самой панели
    auto applyPalette = [&](QWidget* w) {
        QPalette p = w->palette();
        p.setColor(QPalette::Window,      bg);
        p.setColor(QPalette::Base,        inputBg);
        p.setColor(QPalette::WindowText,  txt);
        p.setColor(QPalette::Text,        txt);
        p.setColor(QPalette::Button,      inputBg);
        p.setColor(QPalette::ButtonText,  txt);
        w->setPalette(p);
        w->setAutoFillBackground(true);
    };

    applyPalette(this);
    if (m_scrollArea) {
        applyPalette(m_scrollArea);
        m_scrollArea->viewport() && (applyPalette(m_scrollArea->viewport()), true);
    }
    if (m_container) {
        applyPalette(m_container);
    }

    // 2. Полный QSS — используем простые селекторы классов, без селекторов пространства имён.
    //    Универсальный селектор «* { background }» на контейнере гарантирует, что каждый
    //    вложенный виджет (QWidget внутри QGroupBox, спейсеры QHBoxLayout и т.д.)
    //    получает тёмный фон вместо платформенного белого по умолчанию.
    const QString bgHex      = bg.name();
    const QString txtHex     = txt.name();
    const QString subTxtHex  = subTxt.name();
    const QString borderHex  = border.name();
    const QString inputBgHex = inputBg.name();
    const QString inputBrHex = inputBr.name();
    const QString hoverHex   = hover.name();

    // Применяется ко всей панели (PropertiesPanel + область прокрутки)
    setStyleSheet(QString(
        "QScrollArea, QScrollArea > QWidget, QScrollArea > QWidget > QWidget {"
        "  background: %1;"
        "}"
        "QGroupBox {"
        "  background: %1;"
        "  color: %3;"
        "  font-weight: bold;"
        "  font-size: 11px;"
        "  border: 1px solid %4;"
        "  border-radius: 6px;"
        "  margin-top: 10px;"
        "  padding-top: 8px;"
        "}"
        "QGroupBox::title {"
        "  subcontrol-origin: margin;"
        "  left: 8px;"
        "  padding: 0 4px;"
        "  background: %1;"
        "}"
        "QGroupBox > QWidget {"
        "  background: %1;"
        "}"
        "QLabel {"
        "  color: %3;"
        "  background: transparent;"
        "}"
        "QComboBox {"
        "  background: %5;"
        "  color: %2;"
        "  border: 1px solid %6;"
        "  border-radius: 4px;"
        "  padding: 3px 6px;"
        "}"
        "QComboBox:hover { background: %7; }"
        "QComboBox QAbstractItemView {"
        "  background: %5;"
        "  color: %2;"
        "  selection-background-color: #1971c2;"
        "  selection-color: #ffffff;"
        "}"
        "QPushButton {"
        "  background: %5;"
        "  color: %2;"
        "  border: 1px solid %6;"
        "  border-radius: 4px;"
        "  padding: 4px 8px;"
        "}"
        "QPushButton:hover { background: %7; }"
        "QSlider::groove:horizontal {"
        "  height: 4px;"
        "  background: %4;"
        "  border-radius: 2px;"
        "}"
        "QSlider::handle:horizontal {"
        "  background: #1971c2;"
        "  border: none;"
        "  width: 14px; height: 14px;"
        "  margin: -5px 0;"
        "  border-radius: 7px;"
        "}"
        "QSlider::sub-page:horizontal {"
        "  background: #1971c2;"
        "  border-radius: 2px;"
        "}"
        "QScrollBar:vertical {"
        "  background: %1;"
        "  width: 6px;"
        "  border-radius: 3px;"
        "}"
        "QScrollBar::handle:vertical {"
        "  background: %4;"
        "  border-radius: 3px;"
        "}"
        "QScrollBar::add-line:vertical, QScrollBar::sub-line:vertical { height: 0; }"
    ).arg(bgHex, txtHex, subTxtHex, borderHex, inputBgHex, inputBrHex, hoverHex));
}

} // namespace Whiteboard
