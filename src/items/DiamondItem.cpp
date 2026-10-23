#include "items/DiamondItem.h"
#include "renderer/RoughRenderer.h"
#include <algorithm>

namespace Whiteboard {

DiamondItem::DiamondItem(const QRectF& rect, QGraphicsItem* parent)
    : BaseItem(ItemType::Diamond, parent)
    , m_rect(rect.normalized())
{
}

QRectF DiamondItem::boundingRect() const
{
    const double margin = std::max(12.0, m_properties.strokeWidth + m_properties.roughness * 8.0 + HandleSize);
    return m_rect.adjusted(-margin, -margin, margin, margin);
}

void DiamondItem::paint(QPainter* painter, const QStyleOptionGraphicsItem* option, QWidget* widget)
{
    Q_UNUSED(option);
    Q_UNUSED(widget);

    painter->setRenderHint(QPainter::Antialiasing);
    RoughRenderer::drawDiamond(*painter, m_rect, m_properties);

    if (isSelected()) {
        paintSelectionHandles(painter);
    }
}

void DiamondItem::setGeometryRect(const QRectF& rect)
{
    prepareGeometryChange();
    m_rect = rect.normalized();
    update();
}

std::unique_ptr<BaseItem> DiamondItem::clone() const
{
    auto copy = std::make_unique<DiamondItem>(m_rect);
    copy->setProperties(m_properties);
    copy->setPos(pos());
    copy->setRotation(rotation());
    copy->setZValue(zValue());
    return copy;
}

QJsonObject DiamondItem::toJson() const
{
    QJsonObject json = BaseItem::toJson();
    json[QStringLiteral("type")] = QStringLiteral("diamond");
    json[QStringLiteral("width")] = m_rect.width();
    json[QStringLiteral("height")] = m_rect.height();
    return json;
}

void DiamondItem::fromJson(const QJsonObject& json)
{
    BaseItem::fromJson(json);
    const double w = json[QStringLiteral("width")].toDouble(100.0);
    const double h = json[QStringLiteral("height")].toDouble(80.0);
    setGeometryRect(QRectF(0, 0, w, h));
}

} // namespace Whiteboard
