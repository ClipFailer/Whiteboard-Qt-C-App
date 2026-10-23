#include "items/LineItem.h"
#include "renderer/RoughRenderer.h"
#include <algorithm>

namespace Whiteboard {

LineItem::LineItem(const QPointF& p1, const QPointF& p2, QGraphicsItem* parent)
    : BaseItem(ItemType::Line, parent)
    , m_p1(p1)
    , m_p2(p2)
{
}

QRectF LineItem::boundingRect() const
{
    const double margin = std::max(12.0, m_properties.strokeWidth + m_properties.roughness * 8.0 + HandleSize);
    const double minX = std::min(m_p1.x(), m_p2.x());
    const double maxX = std::max(m_p1.x(), m_p2.x());
    const double minY = std::min(m_p1.y(), m_p2.y());
    const double maxY = std::max(m_p1.y(), m_p2.y());

    return QRectF(minX, minY, maxX - minX, maxY - minY).adjusted(-margin, -margin, margin, margin);
}

void LineItem::paint(QPainter* painter, const QStyleOptionGraphicsItem* option, QWidget* widget)
{
    Q_UNUSED(option);
    Q_UNUSED(widget);

    painter->setRenderHint(QPainter::Antialiasing);
    RoughRenderer::drawLine(*painter, m_p1, m_p2, m_properties);

    if (isSelected()) {
        paintEndpoints(painter);
    }
}

Handle LineItem::handleAt(const QPointF& itemPos) const
{
    if (!isSelected()) {
        return Handle::None;
    }
    if (handleRect(m_p1).contains(itemPos)) {
        return Handle::StartPoint;
    }
    if (handleRect(m_p2).contains(itemPos)) {
        return Handle::EndPoint;
    }
    return Handle::None;
}

void LineItem::resizeByHandle(Handle handle, const QPointF& itemPos)
{
    prepareGeometryChange();
    if (handle == Handle::StartPoint) {
        m_p1 = itemPos;
    } else if (handle == Handle::EndPoint) {
        m_p2 = itemPos;
    }
    update();
}

void LineItem::setPoints(const QPointF& p1, const QPointF& p2)
{
    prepareGeometryChange();
    m_p1 = p1;
    m_p2 = p2;
    update();
}

void LineItem::paintEndpoints(QPainter* painter) const
{
    painter->save();
    QPen handlePen(QColor(25, 113, 194), 1.5, Qt::SolidLine);
    painter->setPen(handlePen);
    painter->setBrush(Qt::white);

    painter->drawEllipse(handleRect(m_p1));
    painter->drawEllipse(handleRect(m_p2));
    painter->restore();
}

std::unique_ptr<BaseItem> LineItem::clone() const
{
    auto copy = std::make_unique<LineItem>(m_p1, m_p2);
    copy->setProperties(m_properties);
    copy->setPos(pos());
    copy->setRotation(rotation());
    copy->setZValue(zValue());
    copy->setStartBinding(m_startBinding);
    copy->setEndBinding(m_endBinding);
    return copy;
}

QJsonObject LineItem::toJson() const
{
    QJsonObject json = BaseItem::toJson();
    json[QStringLiteral("type")] = QStringLiteral("line");
    json[QStringLiteral("x1")] = m_p1.x();
    json[QStringLiteral("y1")] = m_p1.y();
    json[QStringLiteral("x2")] = m_p2.x();
    json[QStringLiteral("y2")] = m_p2.y();

    if (m_startBinding.isValid()) {
        QJsonObject sb;
        sb[QStringLiteral("targetId")] = m_startBinding.targetItemId.toString(QUuid::WithoutBraces);
        sb[QStringLiteral("anchorIndex")] = m_startBinding.anchorIndex;
        json[QStringLiteral("startBinding")] = sb;
    }
    if (m_endBinding.isValid()) {
        QJsonObject eb;
        eb[QStringLiteral("targetId")] = m_endBinding.targetItemId.toString(QUuid::WithoutBraces);
        eb[QStringLiteral("anchorIndex")] = m_endBinding.anchorIndex;
        json[QStringLiteral("endBinding")] = eb;
    }

    return json;
}

void LineItem::fromJson(const QJsonObject& json)
{
    BaseItem::fromJson(json);
    const double x1 = json[QStringLiteral("x1")].toDouble(0.0);
    const double y1 = json[QStringLiteral("y1")].toDouble(0.0);
    const double x2 = json[QStringLiteral("x2")].toDouble(100.0);
    const double y2 = json[QStringLiteral("y2")].toDouble(50.0);
    setPoints(QPointF(x1, y1), QPointF(x2, y2));

    if (json.contains(QStringLiteral("startBinding"))) {
        QJsonObject sb = json[QStringLiteral("startBinding")].toObject();
        m_startBinding.targetItemId = QUuid::fromString(sb[QStringLiteral("targetId")].toString());
        m_startBinding.anchorIndex = sb[QStringLiteral("anchorIndex")].toInt(-1);
    }
    if (json.contains(QStringLiteral("endBinding"))) {
        QJsonObject eb = json[QStringLiteral("endBinding")].toObject();
        m_endBinding.targetItemId = QUuid::fromString(eb[QStringLiteral("targetId")].toString());
        m_endBinding.anchorIndex = eb[QStringLiteral("anchorIndex")].toInt(-1);
    }
}

} // namespace Whiteboard
