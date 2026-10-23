#pragma once

#include "items/BaseItem.h"

namespace Whiteboard {

class EllipseItem : public BaseItem {
public:
    explicit EllipseItem(const QRectF& rect = QRectF(0, 0, 100, 60), QGraphicsItem* parent = nullptr);

    QRectF boundingRect() const override;
    void paint(QPainter* painter, const QStyleOptionGraphicsItem* option, QWidget* widget) override;

    QRectF geometryRect() const override { return m_rect; }
    void setGeometryRect(const QRectF& rect) override;

    std::unique_ptr<BaseItem> clone() const override;
    QJsonObject toJson() const override;
    void fromJson(const QJsonObject& json) override;

private:
    QRectF m_rect;
};

} // namespace Whiteboard
