#include "items/StickyNoteItem.h"
#include "canvas/CanvasScene.h"
#include <QPainter>
#include <QPainterPath>
#include <QGraphicsSceneMouseEvent>
#include <algorithm>

namespace Whiteboard {

namespace {
const QList<QColor> s_pastelPalette = {
    QColor("#fff3bf"), // Pastel Yellow (default)
    QColor("#d3f9d8"), // Pastel Green
    QColor("#d0ebff"), // Pastel Blue / Cyan
    QColor("#ffdeeb")  // Pastel Pink
};
} // namespace

const QList<QColor>& StickyNoteItem::pastelPalette()
{
    return s_pastelPalette;
}

StickyNoteItem::StickyNoteItem(const QString& text, const QPointF& pos, QGraphicsItem* parent)
    : BaseItem(ItemType::StickyNote, parent)
    , m_text(text)
    , m_font(QStringLiteral("Segoe UI"), 14)
    , m_rect(0, 0, 200, 200)
    , m_alignment(Qt::AlignCenter)
    , m_noteColor(s_pastelPalette[0])
{
    setPos(pos);
    m_properties.fillColor = m_noteColor;
    m_properties.strokeColor = QColor("#495057");
    m_properties.strokeWidth = 1.0;
}

QRectF StickyNoteItem::boundingRect() const
{
    const double margin = std::max(12.0, HandleSize + 8.0);
    return m_rect.adjusted(-margin, -margin, margin + 8.0, margin + 8.0);
}

void StickyNoteItem::paint(QPainter* painter, const QStyleOptionGraphicsItem* option, QWidget* widget)
{
    Q_UNUSED(option);
    Q_UNUSED(widget);

    painter->save();
    painter->setRenderHint(QPainter::Antialiasing);
    painter->setRenderHint(QPainter::TextAntialiasing);
    painter->setOpacity(std::clamp(m_properties.opacity, 0.0, 1.0));

    const double foldSize = 24.0;
    const double w = m_rect.width();
    const double h = m_rect.height();

    // 1. Мягкая тень под стикером
    QPainterPath shadowPath;
    shadowPath.moveTo(4, 6);
    shadowPath.lineTo(w + 4, 6);
    shadowPath.lineTo(w + 4, h - foldSize + 6);
    shadowPath.lineTo(w - foldSize + 4, h + 6);
    shadowPath.lineTo(4, h + 6);
    shadowPath.closeSubpath();
    painter->fillPath(shadowPath, QColor(0, 0, 0, 30));

    // 2. Основное тело стикера (со срезанным правым нижним углом)
    QPainterPath notePath;
    notePath.moveTo(0, 0);
    notePath.lineTo(w, 0);
    notePath.lineTo(w, h - foldSize);
    notePath.lineTo(w - foldSize, h);
    notePath.lineTo(0, h);
    notePath.closeSubpath();

    painter->fillPath(notePath, m_noteColor);

    // Скетчевая граница стикера
    QPen borderPen(m_noteColor.darker(120), 1.2, Qt::SolidLine, Qt::RoundCap, Qt::RoundJoin);
    painter->setPen(borderPen);
    painter->drawPath(notePath);

    // 3. Загнутый уголок (dog-ear flap)
    QPainterPath foldPath;
    foldPath.moveTo(w, h - foldSize);
    foldPath.lineTo(w - foldSize, h);
    foldPath.lineTo(w - foldSize + 4, h - foldSize + 4);
    foldPath.closeSubpath();

    // Тень под уголком
    painter->fillPath(foldPath, QColor(0, 0, 0, 35));

    // Сам загнутый треугольник
    QPainterPath flapPath;
    flapPath.moveTo(w, h - foldSize);
    flapPath.lineTo(w - foldSize, h);
    flapPath.lineTo(w - foldSize, h - foldSize);
    flapPath.closeSubpath();

    painter->fillPath(flapPath, m_noteColor.darker(115));
    painter->setPen(borderPen);
    painter->drawPath(flapPath);

    // 4. Текст стикера (если не редактируется в данный момент)
    if (!m_isEditing && !m_text.isEmpty()) {
        painter->setFont(m_font);
        painter->setPen(m_properties.strokeColor.isValid() ? m_properties.strokeColor : QColor("#212529"));

        // Внутренние отступы для текста с учётом загнутого уголка
        QRectF textRect = m_rect.adjusted(14, 14, -18, -26);
        int flags = Qt::TextWordWrap | m_alignment;
        painter->drawText(textRect, flags, m_text);
    }

    painter->restore();

    if (isSelected()) {
        paintSelectionHandles(painter);
    }
}

void StickyNoteItem::setGeometryRect(const QRectF& rect)
{
    prepareGeometryChange();
    m_rect = QRectF(0, 0, std::max(60.0, rect.width()), std::max(60.0, rect.height()));
    update();
}

void StickyNoteItem::resizeByHandle(Handle handle, const QPointF& itemPos)
{
    BaseItem::resizeByHandle(handle, itemPos);
}

void StickyNoteItem::setText(const QString& text)
{
    prepareGeometryChange();
    m_text = text;
    update();
}

void StickyNoteItem::setFont(const QFont& font)
{
    prepareGeometryChange();
    m_font = font;
    update();
}

void StickyNoteItem::setTextAlignment(Qt::Alignment align)
{
    m_alignment = align;
    update();
}

void StickyNoteItem::setNoteColor(const QColor& color)
{
    m_noteColor = color;
    m_properties.fillColor = color;
    update();
}

void StickyNoteItem::setEditing(bool editing)
{
    m_isEditing = editing;
    update();
}

void StickyNoteItem::mouseDoubleClickEvent(QGraphicsSceneMouseEvent* event)
{
    if (event->button() == Qt::LeftButton) {
        if (auto* sc = dynamic_cast<CanvasScene*>(scene())) {
            emit sc->editTextRequested(nullptr); // будет обработано специальным сигналом или через Scene
        }
        event->accept();
        return;
    }
    BaseItem::mouseDoubleClickEvent(event);
}

std::unique_ptr<BaseItem> StickyNoteItem::clone() const
{
    auto copy = std::make_unique<StickyNoteItem>(m_text, pos());
    copy->setProperties(m_properties);
    copy->setGeometryRect(m_rect);
    copy->setFont(m_font);
    copy->setTextAlignment(m_alignment);
    copy->setNoteColor(m_noteColor);
    copy->setRotation(rotation());
    copy->setZValue(zValue());
    return copy;
}

QJsonObject StickyNoteItem::toJson() const
{
    QJsonObject json = BaseItem::toJson();
    json[QStringLiteral("type")] = QStringLiteral("sticky_note");
    json[QStringLiteral("width")] = m_rect.width();
    json[QStringLiteral("height")] = m_rect.height();
    json[QStringLiteral("text")] = m_text;
    json[QStringLiteral("fontFamily")] = m_font.family();
    json[QStringLiteral("fontSize")] = m_font.pointSize();
    json[QStringLiteral("alignment")] = static_cast<int>(m_alignment);
    json[QStringLiteral("noteColor")] = m_noteColor.name(QColor::HexArgb);
    return json;
}

void StickyNoteItem::fromJson(const QJsonObject& json)
{
    BaseItem::fromJson(json);
    double w = json[QStringLiteral("width")].toDouble(200.0);
    double h = json[QStringLiteral("height")].toDouble(200.0);
    m_rect = QRectF(0, 0, w, h);
    m_text = json[QStringLiteral("text")].toString(QStringLiteral("Заметка"));

    QString family = json[QStringLiteral("fontFamily")].toString(QStringLiteral("Segoe UI"));
    int size = json[QStringLiteral("fontSize")].toInt(14);
    m_font = QFont(family, size);

    if (json.contains(QStringLiteral("alignment"))) {
        m_alignment = static_cast<Qt::Alignment>(json[QStringLiteral("alignment")].toInt(Qt::AlignCenter));
    }
    if (json.contains(QStringLiteral("noteColor"))) {
        m_noteColor = QColor(json[QStringLiteral("noteColor")].toString());
        m_properties.fillColor = m_noteColor;
    }
}

} // namespace Whiteboard
