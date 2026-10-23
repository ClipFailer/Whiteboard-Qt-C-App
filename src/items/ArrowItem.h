#pragma once

#include "items/LineItem.h"

namespace Whiteboard {

class ArrowItem : public LineItem {
public:
    explicit ArrowItem(const QPointF& p1 = QPointF(0, 0), const QPointF& p2 = QPointF(100, 50),
                       QGraphicsItem* parent = nullptr);

    QRectF boundingRect() const override;
    void paint(QPainter* painter, const QStyleOptionGraphicsItem* option, QWidget* widget) override;

    std::unique_ptr<BaseItem> clone() const override;
    QJsonObject toJson() const override;
    void fromJson(const QJsonObject& json) override;
};

} // namespace Whiteboard
