#include "items/BaseItem.h"
#include <QPainter>
#include <QRandomGenerator>

namespace Whiteboard {

namespace {

QString fillStyleToString(FillStyle style)
{
    switch (style) {
    case FillStyle::None: return QStringLiteral("none");
    case FillStyle::Solid: return QStringLiteral("solid");
    case FillStyle::Hachure: return QStringLiteral("hachure");
    case FillStyle::CrossHachure: return QStringLiteral("cross-hachure");
    }
    return QStringLiteral("none");
}

FillStyle fillStyleFromString(const QString& str)
{
    if (str == QStringLiteral("solid")) return FillStyle::Solid;
    if (str == QStringLiteral("hachure")) return FillStyle::Hachure;
    if (str == QStringLiteral("cross-hachure")) return FillStyle::CrossHachure;
    return FillStyle::None;
}

QString penStyleToString(Qt::PenStyle style)
{
    switch (style) {
    case Qt::DashLine: return QStringLiteral("dashed");
    case Qt::DotLine: return QStringLiteral("dotted");
    default: return QStringLiteral("solid");
    }
}

Qt::PenStyle penStyleFromString(const QString& str)
{
    if (str == QStringLiteral("dashed")) return Qt::DashLine;
    if (str == QStringLiteral("dotted")) return Qt::DotLine;
    return Qt::SolidLine;
}

} // namespace

BaseItem::BaseItem(ItemType itemType, QGraphicsItem* parent)
    : QGraphicsItem(parent)
    , m_itemType(itemType)
    , m_id(QUuid::createUuid())
{
    setFlags(ItemIsSelectable | ItemIsMovable | ItemSendsGeometryChanges);
    m_properties.seed = QRandomGenerator::global()->generate();
}

void BaseItem::setProperties(const ItemProperties& props)
{
    prepareGeometryChange();
    m_properties = props;
    update();
}

void BaseItem::setLocked(bool locked)
{
    m_locked = locked;
    setFlag(ItemIsMovable, !locked);
    update();
}

QRectF BaseItem::handleRect(const QPointF& center) const
{
    return QRectF(center.x() - HandleHalfSize, center.y() - HandleHalfSize, HandleSize, HandleSize);
}

Handle BaseItem::handleAt(const QPointF& itemPos) const
{
    if (!isSelected() || m_locked) {
        return Handle::None;
    }

    const QRectF rect = geometryRect();
    const double x1 = rect.left();
    const double x2 = rect.center().x();
    const double x3 = rect.right();
    const double y1 = rect.top();
    const double y2 = rect.center().y();
    const double y3 = rect.bottom();

    if (handleRect({x1, y1}).contains(itemPos)) return Handle::TopLeft;
    if (handleRect({x2, y1}).contains(itemPos)) return Handle::TopCenter;
    if (handleRect({x3, y1}).contains(itemPos)) return Handle::TopRight;
    if (handleRect({x3, y2}).contains(itemPos)) return Handle::MidRight;
    if (handleRect({x3, y3}).contains(itemPos)) return Handle::BottomRight;
    if (handleRect({x2, y3}).contains(itemPos)) return Handle::BottomCenter;
    if (handleRect({x1, y3}).contains(itemPos)) return Handle::BottomLeft;
    if (handleRect({x1, y2}).contains(itemPos)) return Handle::MidLeft;

    return Handle::None;
}

void BaseItem::resizeByHandle(Handle handle, const QPointF& itemPos)
{
    QRectF rect = geometryRect();
    constexpr double minSize = 5.0;

    switch (handle) {
    case Handle::TopLeft: {
        double r = rect.right();
        double b = rect.bottom();
        double l = std::min(itemPos.x(), r - minSize);
        double t = std::min(itemPos.y(), b - minSize);
        setGeometryRect(QRectF(QPointF(l, t), QPointF(r, b)).normalized());
        break;
    }
    case Handle::TopCenter: {
        double t = std::min(itemPos.y(), rect.bottom() - minSize);
        rect.setTop(t);
        setGeometryRect(rect.normalized());
        break;
    }
    case Handle::TopRight: {
        double l = rect.left();
        double b = rect.bottom();
        double r = std::max(itemPos.x(), l + minSize);
        double t = std::min(itemPos.y(), b - minSize);
        setGeometryRect(QRectF(QPointF(l, t), QPointF(r, b)).normalized());
        break;
    }
    case Handle::MidRight: {
        double r = std::max(itemPos.x(), rect.left() + minSize);
        rect.setRight(r);
        setGeometryRect(rect.normalized());
        break;
    }
    case Handle::BottomRight: {
        double l = rect.left();
        double t = rect.top();
        double r = std::max(itemPos.x(), l + minSize);
        double b = std::max(itemPos.y(), t + minSize);
        setGeometryRect(QRectF(QPointF(l, t), QPointF(r, b)).normalized());
        break;
    }
    case Handle::BottomCenter: {
        double b = std::max(itemPos.y(), rect.top() + minSize);
        rect.setBottom(b);
        setGeometryRect(rect.normalized());
        break;
    }
    case Handle::BottomLeft: {
        double r = rect.right();
        double t = rect.top();
        double l = std::min(itemPos.x(), r - minSize);
        double b = std::max(itemPos.y(), t + minSize);
        setGeometryRect(QRectF(QPointF(l, t), QPointF(r, b)).normalized());
        break;
    }
    case Handle::MidLeft: {
        double l = std::min(itemPos.x(), rect.right() - minSize);
        rect.setLeft(l);
        setGeometryRect(rect.normalized());
        break;
    }
    default:
        break;
    }
}

QList<QPointF> BaseItem::anchorPointsLocal() const
{
    const QRectF r = geometryRect();
    if (r.isEmpty() || r.width() <= 0 || r.height() <= 0) {
        return {};
    }
    // 8 магнитных точек: 4 середины сторон (0..3) и 4 угла (4..7)
    return {
        QPointF(r.center().x(), r.top()),       // 0: верхний центр
        QPointF(r.right(), r.center().y()),      // 1: правый центр
        QPointF(r.center().x(), r.bottom()),    // 2: нижний центр
        QPointF(r.left(), r.center().y()),       // 3: левый центр
        r.topLeft(),                             // 4: верх-лево
        r.topRight(),                            // 5: верх-право
        r.bottomRight(),                         // 6: низ-право
        r.bottomLeft()                           // 7: низ-лево
    };
}

QList<QPointF> BaseItem::anchorPointsScene() const
{
    const auto locals = anchorPointsLocal();
    QList<QPointF> scenes;
    scenes.reserve(locals.size());
    for (const auto& pt : locals) {
        scenes.append(mapToScene(pt));
    }
    return scenes;
}

void BaseItem::paintSelectionHandles(QPainter* painter) const
{
    if (!isSelected()) {
        return;
    }

    painter->save();

    const QRectF rect = geometryRect();

    // Для заблокированных элементов рисуем красный контур замка и иконку замка
    if (m_locked) {
        QPen lockPen(QColor(224, 49, 49, 220), 1.5, Qt::DashLine);
        painter->setPen(lockPen);
        painter->setBrush(Qt::NoBrush);
        painter->drawRect(rect);

        // Рисуем маленькую плашку замка в верхнем правом углу
        QRectF lockBadge(rect.right() - 20, rect.top() - 22, 20, 20);
        painter->setPen(Qt::NoPen);
        painter->setBrush(QColor(224, 49, 49));
        painter->drawRoundedRect(lockBadge, 4, 4);

        // Дужка и тело замка
        painter->setPen(QPen(Qt::white, 1.5));
        painter->setBrush(Qt::NoBrush);
        painter->drawArc(QRectF(lockBadge.left() + 5, lockBadge.top() + 3, 10, 8), 0, 180 * 16);
        painter->setBrush(Qt::white);
        painter->drawRect(QRectF(lockBadge.left() + 4, lockBadge.top() + 8, 12, 8));

        painter->restore();
        return;
    }

    // Пунктирный ограничивающий прямоугольник
    QPen boxPen(QColor(25, 113, 194, 200), 1.0, Qt::DashLine);
    painter->setPen(boxPen);
    painter->setBrush(Qt::NoBrush);
    painter->drawRect(rect);

    // 8 маркеров
    const double x1 = rect.left();
    const double x2 = rect.center().x();
    const double x3 = rect.right();
    const double y1 = rect.top();
    const double y2 = rect.center().y();
    const double y3 = rect.bottom();

    const std::array<QPointF, 8> points = {
        QPointF{x1, y1}, QPointF{x2, y1}, QPointF{x3, y1},
        QPointF{x3, y2}, QPointF{x3, y3}, QPointF{x2, y3},
        QPointF{x1, y3}, QPointF{x1, y2}
    };

    QPen handlePen(QColor(25, 113, 194), 1.5, Qt::SolidLine);
    painter->setPen(handlePen);
    painter->setBrush(Qt::white);

    for (const auto& pt : points) {
        painter->drawRect(handleRect(pt));
    }

    painter->restore();
}

QJsonObject BaseItem::toJson() const
{
    QJsonObject obj;
    obj[QStringLiteral("id")] = m_id.toString(QUuid::WithoutBraces);
    obj[QStringLiteral("x")] = pos().x();
    obj[QStringLiteral("y")] = pos().y();
    obj[QStringLiteral("zIndex")] = zValue();
    obj[QStringLiteral("rotation")] = rotation();

    obj[QStringLiteral("strokeColor")] = m_properties.strokeColor.name(QColor::HexArgb);
    obj[QStringLiteral("fillColor")] = (m_properties.fillColor == Qt::transparent)
                                           ? QStringLiteral("transparent")
                                           : m_properties.fillColor.name(QColor::HexArgb);
    obj[QStringLiteral("strokeWidth")] = m_properties.strokeWidth;
    obj[QStringLiteral("roughness")] = m_properties.roughness;
    obj[QStringLiteral("seed")] = static_cast<double>(m_properties.seed);
    obj[QStringLiteral("fillStyle")] = fillStyleToString(m_properties.fillStyle);
    obj[QStringLiteral("strokeStyle")] = penStyleToString(m_properties.strokeStyle);
    obj[QStringLiteral("opacity")] = m_properties.opacity;
    obj[QStringLiteral("locked")] = m_locked;
    if (!m_groupId.isNull()) {
        obj[QStringLiteral("groupId")] = m_groupId.toString(QUuid::WithoutBraces);
    }

    return obj;
}

void BaseItem::fromJson(const QJsonObject& json)
{
    if (json.contains(QStringLiteral("id"))) {
        m_id = QUuid::fromString(json[QStringLiteral("id")].toString());
    }
    if (json.contains(QStringLiteral("groupId"))) {
        m_groupId = QUuid::fromString(json[QStringLiteral("groupId")].toString());
    }
    setLocked(json[QStringLiteral("locked")].toBool(false));
    setPos(json[QStringLiteral("x")].toDouble(), json[QStringLiteral("y")].toDouble());
    setZValue(json[QStringLiteral("zIndex")].toDouble());
    setRotation(json[QStringLiteral("rotation")].toDouble());

    m_properties.strokeColor = QColor::fromString(json[QStringLiteral("strokeColor")].toString());
    QString fillStr = json[QStringLiteral("fillColor")].toString();
    if (fillStr == QStringLiteral("transparent")) {
        m_properties.fillColor = Qt::transparent;
    } else {
        m_properties.fillColor = QColor::fromString(fillStr);
    }
    m_properties.strokeWidth = json[QStringLiteral("strokeWidth")].toDouble(2.0);
    m_properties.roughness = json[QStringLiteral("roughness")].toDouble(1.5);
    m_properties.seed = static_cast<uint32_t>(json[QStringLiteral("seed")].toDouble(12345));
    m_properties.fillStyle = fillStyleFromString(json[QStringLiteral("fillStyle")].toString());
    m_properties.strokeStyle = penStyleFromString(json[QStringLiteral("strokeStyle")].toString());
    m_properties.opacity = json[QStringLiteral("opacity")].toDouble(1.0);
}

} // namespace Whiteboard
