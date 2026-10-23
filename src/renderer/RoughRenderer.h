#pragma once

#include "items/BaseItem.h"
#include <QPainter>
#include <QPainterPath>
#include <QPointF>
#include <QRectF>
#include <QVector>
#include <cstdint>

namespace Whiteboard {

class SeededRandom {
public:
    explicit SeededRandom(uint32_t seed)
        : m_seed(seed == 0 ? 12345u : seed)
    {
    }

    // Возвращает детерминированное псевдослучайное число типа double из диапазона [0, 1)
    double next()
    {
        m_seed ^= m_seed << 13;
        m_seed ^= m_seed >> 17;
        m_seed ^= m_seed << 5;
        return (m_seed & 0x7FFFFFFF) / double(0x80000000);
    }

    double nextInRange(double lo, double hi)
    {
        return lo + next() * (hi - lo);
    }

private:
    uint32_t m_seed;
};

class RoughRenderer {
public:
    RoughRenderer() = delete;

    static void drawRect(QPainter& painter, const QRectF& rect, const ItemProperties& props);
    static void drawEllipse(QPainter& painter, const QRectF& rect, const ItemProperties& props);
    static void drawDiamond(QPainter& painter, const QRectF& rect, const ItemProperties& props);
    static void drawLine(QPainter& painter, const QPointF& p1, const QPointF& p2, const ItemProperties& props);
    static void drawArrow(QPainter& painter, const QPointF& p1, const QPointF& p2, const ItemProperties& props);
    static void drawFreehand(QPainter& painter, const QVector<QPointF>& points, const ItemProperties& props);

    // Базовые грубые примитивы
    static void drawRoughSegment(QPainter& painter, const QPointF& p1, const QPointF& p2,
                                 const ItemProperties& props, SeededRandom& rng);

    static void drawHachureFill(QPainter& painter, const QPainterPath& clipPath,
                                const QRectF& bounds, const ItemProperties& props,
                                SeededRandom& rng, double angleDegrees = 45.0);

private:
    static void drawRoughArc(QPainter& painter, const QPointF& center, double rx, double ry,
                             double startAngle, double sweepAngle,
                             const ItemProperties& props, SeededRandom& rng,
                             double jitterScale, double opacityMultiplier);
};

} // namespace Whiteboard
