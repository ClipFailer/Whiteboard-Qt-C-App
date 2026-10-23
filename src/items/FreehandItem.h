#pragma once

#include "items/BaseItem.h"
#include <QVector>

namespace Whiteboard {

class FreehandItem : public BaseItem {
public:
    explicit FreehandItem(QGraphicsItem* parent = nullptr);
    explicit FreehandItem(const QVector<QPointF>& points, QGraphicsItem* parent = nullptr);

    QRectF boundingRect() const override;
    void paint(QPainter* painter, const QStyleOptionGraphicsItem* option, QWidget* widget) override;

    QRectF geometryRect() const override;
    void resizeByHandle(Handle handle, const QPointF& itemPos) override;

    void addPoint(const QPointF& point);
    const QVector<QPointF>& points() const { return m_points; }
    void setPoints(const QVector<QPointF>& points);

    std::unique_ptr<BaseItem> clone() const override;
    QJsonObject toJson() const override;
    void fromJson(const QJsonObject& json) override;

private:
    void updateBounds();

    QVector<QPointF> m_points;
    QRectF m_bounds;
};

} // namespace Whiteboard
