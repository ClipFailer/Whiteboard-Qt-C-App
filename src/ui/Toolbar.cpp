#include "ui/Toolbar.h"
#include <QVBoxLayout>
#include <QPainter>
#include <QPainterPath>
#include <QFrame>

namespace Whiteboard {

Toolbar::Toolbar(ToolManager* toolManager, QUndoStack* undoStack, QWidget* parent)
    : QWidget(parent)
    , m_toolManager(toolManager)
    , m_undoStack(undoStack)
    , m_buttonGroup(new QButtonGroup(this))
{
    m_buttonGroup->setExclusive(true);
    setupUI();

    connect(m_buttonGroup, &QButtonGroup::buttonClicked, this, &Toolbar::onToolButtonClicked);
    connect(m_toolManager, &ToolManager::toolChanged, this, &Toolbar::onToolChanged);
    connect(m_undoStack, &QUndoStack::canUndoChanged, m_undoBtn, &QToolButton::setEnabled);
    connect(m_undoStack, &QUndoStack::canRedoChanged, m_redoBtn, &QToolButton::setEnabled);

    m_undoBtn->setEnabled(m_undoStack->canUndo());
    m_redoBtn->setEnabled(m_undoStack->canRedo());

    updateTheme(false);
}

void Toolbar::setupUI()
{
    auto* layout = new QVBoxLayout(this);
    layout->setContentsMargins(6, 8, 6, 8);
    layout->setSpacing(4);

    createToolButton(Tool::Select, QStringLiteral("Select"), QStringLiteral("V / 1"), QStringLiteral("select"));
    createToolButton(Tool::Hand, QStringLiteral("Hand / Pan"), QStringLiteral("H / 2"), QStringLiteral("hand"));
    createToolButton(Tool::Rectangle, QStringLiteral("Rectangle"), QStringLiteral("R / 3"), QStringLiteral("rect"));
    createToolButton(Tool::Ellipse, QStringLiteral("Ellipse"), QStringLiteral("E / 4"), QStringLiteral("ellipse"));
    createToolButton(Tool::Diamond, QStringLiteral("Diamond"), QStringLiteral("5"), QStringLiteral("diamond"));
    createToolButton(Tool::Line, QStringLiteral("Line"), QStringLiteral("L / 6"), QStringLiteral("line"));
    createToolButton(Tool::Arrow, QStringLiteral("Arrow"), QStringLiteral("A / 7"), QStringLiteral("arrow"));
    createToolButton(Tool::Freehand, QStringLiteral("Pencil"), QStringLiteral("P / 8"), QStringLiteral("pencil"));
    createToolButton(Tool::Text, QStringLiteral("Text"), QStringLiteral("T / 9"), QStringLiteral("text"));
    createToolButton(Tool::StickyNote, QStringLiteral("Sticky Note"), QStringLiteral("S"), QStringLiteral("stickynote"));
    createToolButton(Tool::Eraser, QStringLiteral("Eraser"), QStringLiteral("0"), QStringLiteral("eraser"));

    // Разделительная линия
    auto* sep = new QFrame(this);
    sep->setFrameShape(QFrame::HLine);
    sep->setFrameShadow(QFrame::Sunken);
    sep->setFixedHeight(2);
    layout->addWidget(sep);

    // Отмена / Повтор
    m_undoBtn = new QToolButton(this);
    m_undoBtn->setToolTip(QStringLiteral("Undo (Ctrl+Z)"));
    m_undoBtn->setFixedSize(36, 36);
    connect(m_undoBtn, &QToolButton::clicked, m_undoStack, &QUndoStack::undo);
    layout->addWidget(m_undoBtn);

    m_redoBtn = new QToolButton(this);
    m_redoBtn->setToolTip(QStringLiteral("Redo (Ctrl+Y / Ctrl+Shift+Z)"));
    m_redoBtn->setFixedSize(36, 36);
    connect(m_redoBtn, &QToolButton::clicked, m_undoStack, &QUndoStack::redo);
    layout->addWidget(m_redoBtn);

    layout->addStretch();

    // Выделение по умолчанию
    if (m_toolButtons.contains(Tool::Select)) {
        m_toolButtons[Tool::Select]->setChecked(true);
    }
}

void Toolbar::createToolButton(Tool tool, const QString& tooltip, const QString& shortcut, const QString& iconName)
{
    auto* btn = new QToolButton(this);
    btn->setCheckable(true);
    btn->setToolTip(QStringLiteral("%1 (%2)").arg(tooltip, shortcut));
    btn->setProperty("toolType", static_cast<int>(tool));
    btn->setProperty("iconName", iconName);
    btn->setFixedSize(36, 36);

    m_buttonGroup->addButton(btn);
    m_toolButtons[tool] = btn;
    layout()->addWidget(btn);
}

QIcon Toolbar::createIcon(const QString& name, const QColor& color)
{
    QPixmap pix(28, 28);
    pix.fill(Qt::transparent);

    QPainter p(&pix);
    p.setRenderHint(QPainter::Antialiasing);
    QPen pen(color, 2.0, Qt::SolidLine, Qt::RoundCap, Qt::RoundJoin);
    p.setPen(pen);

    if (name == QStringLiteral("select")) {
        QPainterPath path;
        path.moveTo(6, 5);
        path.lineTo(6, 21);
        path.lineTo(11, 16);
        path.lineTo(16, 23);
        path.lineTo(19, 21);
        path.lineTo(14, 14);
        path.lineTo(20, 14);
        path.closeSubpath();
        p.setBrush(color);
        p.drawPath(path);
    } else if (name == QStringLiteral("hand")) {
        p.drawRoundedRect(QRectF(8, 7, 12, 14), 4, 4);
    } else if (name == QStringLiteral("rect")) {
        p.drawRoundedRect(QRectF(6, 7, 16, 14), 2, 2);
    } else if (name == QStringLiteral("ellipse")) {
        p.drawEllipse(QRectF(5, 6, 18, 16));
    } else if (name == QStringLiteral("diamond")) {
        QPolygonF poly;
        poly << QPointF(14, 5) << QPointF(23, 14) << QPointF(14, 23) << QPointF(5, 14);
        p.drawPolygon(poly);
    } else if (name == QStringLiteral("line")) {
        p.drawLine(QPointF(6, 22), QPointF(22, 6));
    } else if (name == QStringLiteral("arrow")) {
        p.drawLine(QPointF(6, 22), QPointF(22, 6));
        p.drawLine(QPointF(22, 6), QPointF(14, 6));
        p.drawLine(QPointF(22, 6), QPointF(22, 14));
    } else if (name == QStringLiteral("pencil")) {
        p.drawLine(QPointF(6, 22), QPointF(19, 9));
        p.drawLine(QPointF(19, 9), QPointF(22, 12));
        p.drawLine(QPointF(22, 12), QPointF(9, 25));
        p.drawLine(QPointF(6, 22), QPointF(9, 25));
    } else if (name == QStringLiteral("text")) {
        QFont f = p.font();
        f.setBold(true);
        f.setPointSize(14);
        p.setFont(f);
        p.drawText(QRectF(0, 0, 28, 28), Qt::AlignCenter, QStringLiteral("T"));
    } else if (name == QStringLiteral("stickynote")) {
        // Иконка стикера с загнутым уголком
        QPainterPath path;
        path.moveTo(6, 6);
        path.lineTo(22, 6);
        path.lineTo(22, 17);
        path.lineTo(17, 22);
        path.lineTo(6, 22);
        path.closeSubpath();
        p.drawPath(path);
        p.drawLine(QPointF(17, 22), QPointF(17, 17));
        p.drawLine(QPointF(17, 17), QPointF(22, 17));
    } else if (name == QStringLiteral("eraser")) {
        p.drawRect(QRectF(7, 8, 14, 12));
        p.drawLine(QPointF(14, 8), QPointF(14, 20));
    } else if (name == QStringLiteral("undo")) {
        QPainterPath path;
        path.arcMoveTo(QRectF(6, 7, 16, 16), 180);
        path.arcTo(QRectF(6, 7, 16, 16), 180, -180);
        p.drawPath(path);
        p.drawLine(QPointF(7, 15), QPointF(6, 9));
        p.drawLine(QPointF(7, 15), QPointF(13, 14));
    } else if (name == QStringLiteral("redo")) {
        QPainterPath path;
        path.arcMoveTo(QRectF(6, 7, 16, 16), 0);
        path.arcTo(QRectF(6, 7, 16, 16), 0, 180);
        p.drawPath(path);
        p.drawLine(QPointF(21, 15), QPointF(22, 9));
        p.drawLine(QPointF(21, 15), QPointF(15, 14));
    }

    return QIcon(pix);
}

void Toolbar::updateTheme(bool isDark)
{
    m_isDark = isDark;
    const QColor iconColor = isDark ? QColor(220, 220, 220) : QColor(50, 50, 50);

    for (auto it = m_toolButtons.begin(); it != m_toolButtons.end(); ++it) {
        QString iconName = it.value()->property("iconName").toString();
        it.value()->setIcon(createIcon(iconName, iconColor));
        it.value()->setIconSize(QSize(22, 22));
    }

    m_undoBtn->setIcon(createIcon(QStringLiteral("undo"), iconColor));
    m_undoBtn->setIconSize(QSize(22, 22));
    m_redoBtn->setIcon(createIcon(QStringLiteral("redo"), iconColor));
    m_redoBtn->setIconSize(QSize(22, 22));

    // Используем QPalette для базового фона (надёжно на всех платформах / стилях)
    QPalette pal = palette();
    const QColor bgColor = isDark ? QColor(0x1e, 0x1e, 0x1e) : QColor(0xff, 0xff, 0xff);
    pal.setColor(QPalette::Window, bgColor);
    pal.setColor(QPalette::Base,   bgColor);
    setPalette(pal);
    setAutoFillBackground(true);

    // QSS только для состояний QToolButton — селектор пространства имён не нужен
    if (isDark) {
        setStyleSheet(QStringLiteral(
            "QWidget { background: #1e1e1e; border-radius: 8px; }"
            "QToolButton {"
            "  border: 1px solid transparent;"
            "  border-radius: 6px;"
            "  background: transparent;"
            "  color: #e0e0e0;"
            "}"
            "QToolButton:hover { background: #2c2c2c; }"
            "QToolButton:checked {"
            "  background: #18385a;"
            "  border: 1px solid #339af0;"
            "}"
            "QToolButton:disabled { color: #555555; }"));
    } else {
        setStyleSheet(QStringLiteral(
            "QWidget { background: #ffffff; border-radius: 8px; }"
            "QToolButton {"
            "  border: 1px solid transparent;"
            "  border-radius: 6px;"
            "  background: transparent;"
            "  color: #212529;"
            "}"
            "QToolButton:hover { background: #f1f3f5; }"
            "QToolButton:checked {"
            "  background: #e7f5ff;"
            "  border: 1px solid #1971c2;"
            "}"
            "QToolButton:disabled { color: #adb5bd; }"));
    }
}

void Toolbar::onToolButtonClicked(QAbstractButton* button)
{
    Tool tool = static_cast<Tool>(button->property("toolType").toInt());
    m_toolManager->setTool(tool);
}

void Toolbar::onToolChanged(Tool tool)
{
    if (m_toolButtons.contains(tool)) {
        m_toolButtons[tool]->setChecked(true);
    }
}

} // namespace Whiteboard
