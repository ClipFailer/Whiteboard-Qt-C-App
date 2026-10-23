#include "items/EllipseItem.h"
#include "renderer/RoughRenderer.h"
#include <algorithm>

namespace Whiteboard {

EllipseItem::EllipseItem(const QRectF& rect, QGraphicsItem* parent)
    : BaseItem(ItemType::Ellipse, parent)
    , m_rect(rect.normalized())
{
}

QRectF EllipseItem::boundingRect() const
{
    const double margin = std::max(12.0, m_properties.strokeWidth + m_properties.roughness * 8.0 + HandleSize);
    return m_rect.adjusted(-margin, -margin, margin, margin);
}

void EllipseItem::paint(QPainter* painter, const QStyleOptionGraphicsItem* option, QWidget* widget)
{
    Q_UNUSED(option);
    Q_UNUSED(widget);

    painter->setRenderHint(QPainter::Antialiasing);
    RoughRenderer::drawEllipse(*painter, m_rect, m_properties);

    if (isSelected()) {
        paintSelectionHandles(painter);
    }
}

void EllipseItem::setGeometryRect(const QRectF& rect)
{
    prepareGeometryChange();
    m_rect = rect.normalized();
    update();
}

std::unique_ptr<BaseItem> EllipseItem::clone() const
{
    auto copy = std::make_unique<EllipseItem>(m_rect);
    copy->setProperties(m_properties);
    copy->setPos(pos());
    copy->setRotation(rotation());
    copy->setZValue(zValue());
    return copy;
}

QJsonObject EllipseItem::toJson() const
{
    QJsonObject json = BaseItem::toJson();
    json[QStringLiteral("type")] = QStringLiteral("ellipse");
    json[QStringLiteral("width")] = m_rect.width();
    json[QStringLiteral("height")] = m_rect.height();
    return json;
}

void EllipseItem::fromJson(const QJsonObject& json)
{
    BaseItem::fromJson(json);
    const double w = json[QStringLiteral("width")].toDouble(100.0);
    const double h = json[QStringLiteral("height")].toDouble(60.0);
    setGeometryRect(QRectF(0, 0, w, h));
}

} // namespace Whiteboard
