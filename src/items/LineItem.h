#pragma once

#include "items/BaseItem.h"

namespace Whiteboard {

struct AnchorBinding {
    QUuid targetItemId;
    int anchorIndex{-1}; // Индекс точки привязки: 0..3 середины, 4..7 углы

    bool isValid() const { return !targetItemId.isNull() && anchorIndex >= 0; }
    bool operator==(const AnchorBinding& other) const = default;
};

class LineItem : public BaseItem {
public:
    explicit LineItem(const QPointF& p1 = QPointF(0, 0), const QPointF& p2 = QPointF(100, 50),
                      QGraphicsItem* parent = nullptr);

    QRectF boundingRect() const override;
    void paint(QPainter* painter, const QStyleOptionGraphicsItem* option, QWidget* widget) override;

    Handle handleAt(const QPointF& itemPos) const override;
    void resizeByHandle(Handle handle, const QPointF& itemPos) override;

    QPointF p1() const { return m_p1; }
    QPointF p2() const { return m_p2; }
    void setPoints(const QPointF& p1, const QPointF& p2);

    AnchorBinding startBinding() const { return m_startBinding; }
    void setStartBinding(const AnchorBinding& b) { m_startBinding = b; }

    AnchorBinding endBinding() const { return m_endBinding; }
    void setEndBinding(const AnchorBinding& b) { m_endBinding = b; }

    std::unique_ptr<BaseItem> clone() const override;
    QJsonObject toJson() const override;
    void fromJson(const QJsonObject& json) override;

protected:
    void paintEndpoints(QPainter* painter) const;

    QPointF m_p1;
    QPointF m_p2;
    AnchorBinding m_startBinding;
    AnchorBinding m_endBinding;
};

} // namespace Whiteboard
