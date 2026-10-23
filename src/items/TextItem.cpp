#include "items/TextItem.h"
#include <QFontMetricsF>
#include <QGraphicsScene>
#include <QGraphicsSceneMouseEvent>
#include <QPainter>
#include <QTextLayout>
#include <algorithm>

namespace Whiteboard {

namespace {
QFont createHandwritingFont(int pointSize = 18)
{
    QFont font(QStringLiteral("Segoe UI"), pointSize);
    font.setStyleHint(QFont::SansSerif);
    const QStringList families = {QStringLiteral("Segoe Print"), QStringLiteral("Comic Sans MS"), QStringLiteral("Segoe UI")};
    for (const auto& fam : families) {
        QFont testFont(fam, pointSize);
        if (testFont.exactMatch() || fam == families.last()) {
            font = testFont;
            break;
        }
    }
    return font;
}
} // namespace

TextItem::TextItem(const QString& text, const QPointF& pos, QGraphicsItem* parent)
    : BaseItem(ItemType::Text, parent)
    , m_text(text)
    , m_font(createHandwritingFont(18))
{
    setPos(pos);
    recalcRect();
}

void TextItem::recalcRect()
{
    prepareGeometryChange();
    QFontMetricsF fm(m_font);

    if (m_userWidth > 20.0) {
        // Пользователь явно задал размер блока → переносим текст по m_userWidth, вычисляем высоту
        QRectF wrapBounds = fm.boundingRect(
            QRectF(0, 0, m_userWidth - 8.0, 50000),
            Qt::AlignLeft | Qt::AlignTop | Qt::TextWordWrap,
            m_text.isEmpty() ? QStringLiteral(" ") : m_text);
        const double h = std::max(24.0, wrapBounds.height() + 10.0);
        m_rect = QRectF(0, 0, m_userWidth, h);
    } else {
        // Авторазмер: измеряем текст без переноса
        QRectF tight = fm.boundingRect(
            QRectF(0, 0, 9999, 9999),
            Qt::AlignLeft | Qt::AlignTop,
            m_text.isEmpty() ? QStringLiteral(" ") : m_text);
        const double w = std::max(20.0, tight.width() + 10.0);
        const double h = std::max(24.0, tight.height() + 8.0);
        m_rect = QRectF(0, 0, w, h);
    }
}


QRectF TextItem::boundingRect() const
{
    const double margin = std::max(8.0, HandleSize);
    return m_rect.adjusted(-margin, -margin, margin, margin);
}

void TextItem::paint(QPainter* painter, const QStyleOptionGraphicsItem* option, QWidget* widget)
{
    Q_UNUSED(option);
    Q_UNUSED(widget);

    if (m_isEditing) {
        painter->save();
        QPen p(QColor(100, 100, 100, 120), 1.0, Qt::DashLine);
        painter->setPen(p);
        painter->drawRect(m_rect);
        painter->restore();
        return;
    }

    painter->save();
    painter->setRenderHint(QPainter::TextAntialiasing);
    painter->setRenderHint(QPainter::Antialiasing);

    // Необязательная заливка фона
    if (m_properties.fillStyle == FillStyle::Solid && m_properties.fillColor != Qt::transparent) {
        QColor bg = m_properties.fillColor;
        bg.setAlphaF(std::clamp(bg.alphaF() * m_properties.opacity, 0.0, 1.0));
        painter->fillRect(m_rect, bg);
    }

    QColor textColor = m_properties.strokeColor;
    textColor.setAlphaF(std::clamp(textColor.alphaF() * m_properties.opacity, 0.0, 1.0));
    painter->setPen(textColor);
    painter->setFont(m_font);

    // Всегда используем TextWordWrap, чтобы текст соблюдал ширину блока
    painter->drawText(m_rect.adjusted(4, 4, -4, -4),
                      Qt::AlignLeft | Qt::AlignTop | Qt::TextWordWrap,
                      m_text);

    painter->restore();

    if (isSelected()) {
        paintSelectionHandles(painter);
    }
}

QRectF TextItem::geometryRect() const
{
    return m_rect;
}

void TextItem::setGeometryRect(const QRectF& rect)
{
    prepareGeometryChange();
    m_rect = rect;
    // Синхронизируем явную ширину пользователя, чтобы recalcRect учитывала её при следующем вызове
    m_userWidth = rect.width();
    update();
}

void TextItem::resizeByHandle(Handle handle, const QPointF& itemPos)
{
    if (m_text.isEmpty()) {
        return;
    }

    if (!m_isResizing || m_activeResizeHandle != handle) {
        m_isResizing = true;
        m_activeResizeHandle = handle;
        m_startRect = m_rect;
        m_startPos = pos();
        m_startFontSize = m_font.pointSizeF() > 0 ? m_font.pointSizeF() : static_cast<qreal>(m_font.pointSize());
        if (m_startFontSize <= 0.0) {
            m_startFontSize = 18.0;
        }
        m_startUserWidth = (m_userWidth > 20.0) ? m_userWidth : m_rect.width();
    }

    // Переводим локальную координату текущего кадра в координаты сцены,
    // а затем относительно исходной позиции m_startPos, чтобы избежать накопления погрешности.
    const QPointF scenePos = mapToScene(itemPos);
    const QPointF curPos = scenePos - m_startPos;

    const double w0 = std::max(10.0, m_startRect.width());
    const double h0 = std::max(10.0, m_startRect.height());

    double scale = 1.0;
    enum class Anchor {
        TopLeft,
        BottomRight,
        TopRight,
        BottomLeft,
        MidLeft,
        TopCenter
    };
    Anchor anchor = Anchor::TopLeft;

    switch (handle) {
    case Handle::BottomRight: {
        scale = (curPos.x() * w0 + curPos.y() * h0) / (w0 * w0 + h0 * h0);
        anchor = Anchor::TopLeft;
        break;
    }
    case Handle::TopLeft: {
        scale = ((w0 - curPos.x()) * w0 + (h0 - curPos.y()) * h0) / (w0 * w0 + h0 * h0);
        anchor = Anchor::BottomRight;
        break;
    }
    case Handle::TopRight: {
        scale = (curPos.x() * w0 + (h0 - curPos.y()) * h0) / (w0 * w0 + h0 * h0);
        anchor = Anchor::BottomLeft;
        break;
    }
    case Handle::BottomLeft: {
        scale = ((w0 - curPos.x()) * w0 + curPos.y() * h0) / (w0 * w0 + h0 * h0);
        anchor = Anchor::TopRight;
        break;
    }
    case Handle::MidRight: {
        scale = curPos.x() / w0;
        anchor = Anchor::TopLeft;
        break;
    }
    case Handle::MidLeft: {
        scale = (w0 - curPos.x()) / w0;
        anchor = Anchor::MidLeft;
        break;
    }
    case Handle::BottomCenter: {
        scale = curPos.y() / h0;
        anchor = Anchor::TopLeft;
        break;
    }
    case Handle::TopCenter: {
        scale = (h0 - curPos.y()) / h0;
        anchor = Anchor::TopCenter;
        break;
    }
    default:
        return;
    }

    // Ограничиваем масштаб разумными пределами для плавного изменения
    scale = std::clamp(scale, 0.1, 20.0);
    const qreal newFontSize = std::clamp(m_startFontSize * scale, 6.0, 250.0);

    prepareGeometryChange();
    m_font.setPointSizeF(newFontSize);

    if (m_startUserWidth > 20.0) {
        m_userWidth = std::max(20.0, m_startUserWidth * scale);
    }

    recalcRect();

    const double wNew = m_rect.width();
    const double hNew = m_rect.height();

    switch (anchor) {
    case Anchor::TopLeft:
        setPos(m_startPos);
        break;
    case Anchor::BottomRight:
        setPos(m_startPos + QPointF(w0 - wNew, h0 - hNew));
        break;
    case Anchor::TopRight:
        setPos(m_startPos + QPointF(w0 - wNew, 0.0));
        break;
    case Anchor::BottomLeft:
        setPos(m_startPos + QPointF(0.0, h0 - hNew));
        break;
    case Anchor::MidLeft:
        setPos(m_startPos + QPointF(w0 - wNew, 0.0));
        break;
    case Anchor::TopCenter:
        setPos(m_startPos + QPointF(0.0, h0 - hNew));
        break;
    }

    update();
}

void TextItem::finishResize()
{
    m_isResizing = false;
    m_activeResizeHandle = Handle::None;
}

void TextItem::setText(const QString& text)
{
    m_text = text;
    recalcRect();
    update();
}

void TextItem::setFont(const QFont& font)
{
    m_font = font;
    recalcRect();
    update();
}

void TextItem::setFontSize(int pointSize)
{
    setFontSize(static_cast<qreal>(pointSize));
}

void TextItem::setFontSize(qreal pointSize)
{
    m_font.setPointSizeF(std::clamp(pointSize, 6.0, 250.0));
    recalcRect();
    update();
}

void TextItem::setEditing(bool editing)
{
    m_isEditing = editing;
    update();
}

void TextItem::mouseDoubleClickEvent(QGraphicsSceneMouseEvent* event)
{
    Q_UNUSED(event);
    BaseItem::mouseDoubleClickEvent(event);
}

std::unique_ptr<BaseItem> TextItem::clone() const
{
    auto copy = std::make_unique<TextItem>(m_text, pos());
    copy->setProperties(m_properties);
    copy->setFont(m_font);
    // Сохраняем явную ширину блока, чтобы клон выглядел одинаково
    copy->m_userWidth = m_userWidth;
    copy->recalcRect();
    copy->setRotation(rotation());
    copy->setZValue(zValue());
    return copy;
}

QJsonObject TextItem::toJson() const
{
    QJsonObject json = BaseItem::toJson();
    json[QStringLiteral("type")] = QStringLiteral("text");
    json[QStringLiteral("text")] = m_text;
    json[QStringLiteral("fontSize")] = fontSize();
    json[QStringLiteral("fontFamily")] = m_font.family();
    json[QStringLiteral("width")] = m_rect.width();
    json[QStringLiteral("height")] = m_rect.height();
    json[QStringLiteral("userWidth")] = m_userWidth;
    return json;
}

void TextItem::fromJson(const QJsonObject& json)
{
    BaseItem::fromJson(json);
    m_text = json[QStringLiteral("text")].toString(QStringLiteral("Text"));
    if (json.contains(QStringLiteral("fontFamily"))) {
        m_font.setFamily(json[QStringLiteral("fontFamily")].toString());
    }
    if (json.contains(QStringLiteral("fontSize"))) {
        m_font.setPointSizeF(json[QStringLiteral("fontSize")].toDouble(18.0));
    }
    m_userWidth = json[QStringLiteral("userWidth")].toDouble(0.0);
    if (m_userWidth <= 0.0 && json.contains(QStringLiteral("width"))) {
        // Устаревшие файлы без userWidth: восстанавливаем явную ширину, если она была сохранена
        double w = json[QStringLiteral("width")].toDouble(0.0);
        if (w > 20.0) m_userWidth = w;
    }
    recalcRect();
}

} // namespace Whiteboard
