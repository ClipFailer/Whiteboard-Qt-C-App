#include "items/ArrowItem.h"
#include "renderer/RoughRenderer.h"
#include <algorithm>

namespace Whiteboard {

ArrowItem::ArrowItem(const QPointF& p1, const QPointF& p2, QGraphicsItem* parent)
    : LineItem(p1, p2, parent)
{
    m_itemType = ItemType::Arrow;
}

QRectF ArrowItem::boundingRect() const
{
    // Поля вмещают наконечник стрелки ~30px
    const double margin = std::max(25.0, m_properties.strokeWidth + m_properties.roughness * 8.0 + HandleSize + 20.0);
    const double minX = std::min(m_p1.x(), m_p2.x());
    const double maxX = std::max(m_p1.x(), m_p2.x());
    const double minY = std::min(m_p1.y(), m_p2.y());
    const double maxY = std::max(m_p1.y(), m_p2.y());

    return QRectF(minX, minY, maxX - minX, maxY - minY).adjusted(-margin, -margin, margin, margin);
}

void ArrowItem::paint(QPainter* painter, const QStyleOptionGraphicsItem* option, QWidget* widget)
{
    Q_UNUSED(option);
    Q_UNUSED(widget);

    painter->setRenderHint(QPainter::Antialiasing);
    RoughRenderer::drawArrow(*painter, m_p1, m_p2, m_properties);

    if (isSelected()) {
        paintEndpoints(painter);
    }
}

std::unique_ptr<BaseItem> ArrowItem::clone() const
{
    auto copy = std::make_unique<ArrowItem>(m_p1, m_p2);
    copy->setProperties(m_properties);
    copy->setPos(pos());
    copy->setRotation(rotation());
    copy->setZValue(zValue());
    copy->setStartBinding(m_startBinding);
    copy->setEndBinding(m_endBinding);
    return copy;
}

QJsonObject ArrowItem::toJson() const
{
    QJsonObject json = LineItem::toJson();
    json[QStringLiteral("type")] = QStringLiteral("arrow");
    return json;
}

void ArrowItem::fromJson(const QJsonObject& json)
{
    LineItem::fromJson(json);
}

} // namespace Whiteboard
