#include "items/FreehandItem.h"
#include "renderer/RoughRenderer.h"
#include <QJsonArray>
#include <algorithm>

namespace Whiteboard {

FreehandItem::FreehandItem(QGraphicsItem* parent)
    : BaseItem(ItemType::Freehand, parent)
{
}

FreehandItem::FreehandItem(const QVector<QPointF>& points, QGraphicsItem* parent)
    : BaseItem(ItemType::Freehand, parent)
    , m_points(points)
{
    updateBounds();
}

QRectF FreehandItem::boundingRect() const
{
    const double margin = std::max(12.0, m_properties.strokeWidth + m_properties.roughness * 6.0 + HandleSize);
    return m_bounds.adjusted(-margin, -margin, margin, margin);
}

void FreehandItem::paint(QPainter* painter, const QStyleOptionGraphicsItem* option, QWidget* widget)
{
    Q_UNUSED(option);
    Q_UNUSED(widget);

    painter->setRenderHint(QPainter::Antialiasing);
    RoughRenderer::drawFreehand(*painter, m_points, m_properties);

    if (isSelected() && !m_points.isEmpty()) {
        paintSelectionHandles(painter);
    }
}

QRectF FreehandItem::geometryRect() const
{
    return m_bounds;
}

void FreehandItem::addPoint(const QPointF& point)
{
    prepareGeometryChange();
    m_points.append(point);
    if (m_points.size() == 1) {
        m_bounds = QRectF(point.x() - 1, point.y() - 1, 2, 2);
    } else {
        m_bounds = m_bounds.united(QRectF(point.x() - 1, point.y() - 1, 2, 2));
    }
    update();
}

void FreehandItem::setPoints(const QVector<QPointF>& points)
{
    prepareGeometryChange();
    m_points = points;
    updateBounds();
    update();
}

void FreehandItem::updateBounds()
{
    if (m_points.isEmpty()) {
        m_bounds = QRectF();
        return;
    }

    double minX = m_points[0].x();
    double maxX = minX;
    double minY = m_points[0].y();
    double maxY = minY;

    for (const auto& pt : m_points) {
        minX = std::min(minX, pt.x());
        maxX = std::max(maxX, pt.x());
        minY = std::min(minY, pt.y());
        maxY = std::max(maxY, pt.y());
    }

    m_bounds = QRectF(minX, minY, std::max(1.0, maxX - minX), std::max(1.0, maxY - minY));
}

void FreehandItem::resizeByHandle(Handle handle, const QPointF& itemPos)
{
    if (m_points.isEmpty() || m_bounds.width() < 1.0 || m_bounds.height() < 1.0) {
        return;
    }

    QRectF oldBounds = m_bounds;
    QRectF newBounds = oldBounds;
    constexpr double minSize = 5.0;

    switch (handle) {
    case Handle::TopLeft: {
        double r = oldBounds.right();
        double b = oldBounds.bottom();
        newBounds = QRectF(QPointF(std::min(itemPos.x(), r - minSize), std::min(itemPos.y(), b - minSize)),
                           QPointF(r, b));
        break;
    }
    case Handle::BottomRight: {
        double l = oldBounds.left();
        double t = oldBounds.top();
        newBounds = QRectF(QPointF(l, t),
                           QPointF(std::max(itemPos.x(), l + minSize), std::max(itemPos.y(), t + minSize)));
        break;
    }
    default:
        return;
    }

    const double sx = newBounds.width() / oldBounds.width();
    const double sy = newBounds.height() / oldBounds.height();

    prepareGeometryChange();
    for (auto& pt : m_points) {
        double relX = (pt.x() - oldBounds.left()) * sx;
        double relY = (pt.y() - oldBounds.top()) * sy;
        pt = QPointF(newBounds.left() + relX, newBounds.top() + relY);
    }
    updateBounds();
    update();
}

std::unique_ptr<BaseItem> FreehandItem::clone() const
{
    auto copy = std::make_unique<FreehandItem>(m_points);
    copy->setProperties(m_properties);
    copy->setPos(pos());
    copy->setRotation(rotation());
    copy->setZValue(zValue());
    return copy;
}

QJsonObject FreehandItem::toJson() const
{
    QJsonObject json = BaseItem::toJson();
    json[QStringLiteral("type")] = QStringLiteral("freehand");

    QJsonArray pointsArr;
    for (const auto& pt : m_points) {
        QJsonObject ptObj;
        ptObj[QStringLiteral("x")] = pt.x();
        ptObj[QStringLiteral("y")] = pt.y();
        pointsArr.append(ptObj);
    }
    json[QStringLiteral("points")] = pointsArr;
    return json;
}

void FreehandItem::fromJson(const QJsonObject& json)
{
    BaseItem::fromJson(json);
    m_points.clear();
    const QJsonArray pointsArr = json[QStringLiteral("points")].toArray();
    for (const auto& v : pointsArr) {
        const QJsonObject ptObj = v.toObject();
        m_points.append(QPointF(ptObj[QStringLiteral("x")].toDouble(), ptObj[QStringLiteral("y")].toDouble()));
    }
    updateBounds();
}

} // namespace Whiteboard
