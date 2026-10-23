#pragma once

#include <QGraphicsItem>
#include <QColor>
#include <QUuid>
#include <QJsonObject>
#include <memory>

namespace Whiteboard {

enum class FillStyle {
    None,
    Solid,
    Hachure,
    CrossHachure
};

enum class ItemType {
    Rectangle,
    Ellipse,
    Diamond,
    Line,
    Arrow,
    Freehand,
    Text,
    Image,
    StickyNote
};

enum class Handle {
    None,
    TopLeft,
    TopCenter,
    TopRight,
    MidRight,
    BottomRight,
    BottomCenter,
    BottomLeft,
    MidLeft,
    StartPoint,
    EndPoint
};

struct ItemProperties {
    QColor strokeColor{Qt::black};
    QColor fillColor{Qt::transparent};
    double strokeWidth{2.0};
    double roughness{1.5};    // 0 = гладко, 3 = очень грубо
    uint32_t seed{12345};
    FillStyle fillStyle{FillStyle::None}; // None (без заливки), Solid (сплошная), Hachure (штриховка), CrossHachure (перекрёстная)
    Qt::PenStyle strokeStyle{Qt::SolidLine};
    double opacity{1.0};

    bool operator==(const ItemProperties& other) const = default;
};

class BaseItem : public QGraphicsItem {
public:
    enum { Type = UserType + 100 };

    explicit BaseItem(ItemType itemType, QGraphicsItem* parent = nullptr);
    ~BaseItem() override = default;

    int type() const override { return Type; }

    ItemType itemType() const { return m_itemType; }
    QUuid id() const { return m_id; }
    void setId(const QUuid& id) { m_id = id; }

    bool isLocked() const { return m_locked; }
    void setLocked(bool locked);

    QUuid groupId() const { return m_groupId; }
    void setGroupId(const QUuid& id) { m_groupId = id; }

    const ItemProperties& properties() const { return m_properties; }
    virtual void setProperties(const ItemProperties& props);

    virtual QRectF geometryRect() const { return QRectF(); }
    virtual void setGeometryRect(const QRectF& rect) { Q_UNUSED(rect); }

    virtual Handle handleAt(const QPointF& itemPos) const;
    virtual void resizeByHandle(Handle handle, const QPointF& itemPos);
    virtual void finishResize() {}

    // Магнитные точки привязки для стрелок/линий (Connectors)
    virtual QList<QPointF> anchorPointsLocal() const;
    QList<QPointF> anchorPointsScene() const;

    virtual std::unique_ptr<BaseItem> clone() const = 0;
    virtual QJsonObject toJson() const;
    virtual void fromJson(const QJsonObject& json);

    // Вспомогательные функции для отрисовки маркеров
    static constexpr double HandleSize = 8.0;
    static constexpr double HandleHalfSize = HandleSize / 2.0;

protected:
    void paintSelectionHandles(QPainter* painter) const;
    QRectF handleRect(const QPointF& center) const;

    ItemType m_itemType;
    QUuid m_id;
    QUuid m_groupId;
    bool m_locked{false};
    ItemProperties m_properties;
};

} // namespace Whiteboard
