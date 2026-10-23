#include "renderer/RoughRenderer.h"
#include <cmath>
#include <numbers>
#include <algorithm>

namespace Whiteboard {

namespace {

constexpr double Pi = std::numbers::pi;

QColor applyOpacity(const QColor& color, double opacityMultiplier)
{
    QColor c = color;
    c.setAlphaF(std::clamp(c.alphaF() * opacityMultiplier, 0.0, 1.0));
    return c;
}

} // namespace

void RoughRenderer::drawRoughSegment(QPainter& painter, const QPointF& p1, const QPointF& p2,
                                     const ItemProperties& props, SeededRandom& rng)
{
    const double dx = p2.x() - p1.x();
    const double dy = p2.y() - p1.y();
    const double len = std::hypot(dx, dy);

    if (len < 0.5) {
        return;
    }

    const double ux = dx / len;
    const double uy = dy / len;
    const double nx = -uy;
    const double ny = ux;

    const double roughness = std::max(0.0, props.roughness);

    // Если полностью гладко — рисуем чистую линию
    if (roughness < 0.05) {
        QPen pen(applyOpacity(props.strokeColor, props.opacity), props.strokeWidth, props.strokeStyle, Qt::RoundCap, Qt::RoundJoin);
        painter.setPen(pen);
        painter.drawLine(p1, p2);
        return;
    }

    // 1. Основная кривая Безье: небольшой джиттер (roughness * 2px)
    const double jitter1 = roughness * 2.0;
    const double overshoot1 = rng.nextInRange(0.0, jitter1 * 0.4);
    const double overshoot2 = rng.nextInRange(0.0, jitter1 * 0.4);

    QPointF start1 = p1 - QPointF(ux * overshoot1, uy * overshoot1)
                     + QPointF(nx * rng.nextInRange(-jitter1 * 0.2, jitter1 * 0.2),
                               ny * rng.nextInRange(-jitter1 * 0.2, jitter1 * 0.2));
    QPointF end1   = p2 + QPointF(ux * overshoot2, uy * overshoot2)
                     + QPointF(nx * rng.nextInRange(-jitter1 * 0.2, jitter1 * 0.2),
                               ny * rng.nextInRange(-jitter1 * 0.2, jitter1 * 0.2));

    QPointF cp1 = p1 + QPointF(ux * (len * 0.33), uy * (len * 0.33))
                  + QPointF(nx * rng.nextInRange(-jitter1, jitter1),
                            ny * rng.nextInRange(-jitter1, jitter1));
    QPointF cp2 = p1 + QPointF(ux * (len * 0.66), uy * (len * 0.66))
                  + QPointF(nx * rng.nextInRange(-jitter1, jitter1),
                            ny * rng.nextInRange(-jitter1, jitter1));

    QPainterPath path1;
    path1.moveTo(start1);
    path1.cubicTo(cp1, cp2, end1);

    QPen pen1(applyOpacity(props.strokeColor, props.opacity), props.strokeWidth, props.strokeStyle, Qt::RoundCap, Qt::RoundJoin);
    painter.setPen(pen1);
    painter.setBrush(Qt::NoBrush);
    painter.drawPath(path1);

    // 2. Вторичная кривая Безье: больший джиттер (roughness * 3.8px) с пониженной непрозрачностью
    const double jitter2 = roughness * 3.5;
    const double offsetStart = rng.nextInRange(-jitter2 * 0.3, jitter2 * 0.3);
    const double offsetEnd   = rng.nextInRange(-jitter2 * 0.3, jitter2 * 0.3);

    QPointF start2 = p1 + QPointF(ux * offsetStart, uy * offsetStart)
                     + QPointF(nx * rng.nextInRange(-jitter2 * 0.25, jitter2 * 0.25),
                               ny * rng.nextInRange(-jitter2 * 0.25, jitter2 * 0.25));
    QPointF end2   = p2 + QPointF(ux * offsetEnd, uy * offsetEnd)
                     + QPointF(nx * rng.nextInRange(-jitter2 * 0.25, jitter2 * 0.25),
                               ny * rng.nextInRange(-jitter2 * 0.25, jitter2 * 0.25));

    QPointF cp3 = p1 + QPointF(ux * (len * 0.35), uy * (len * 0.35))
                  + QPointF(nx * rng.nextInRange(-jitter2, jitter2),
                            ny * rng.nextInRange(-jitter2, jitter2));
    QPointF cp4 = p1 + QPointF(ux * (len * 0.65), uy * (len * 0.65))
                  + QPointF(nx * rng.nextInRange(-jitter2, jitter2),
                            ny * rng.nextInRange(-jitter2, jitter2));

    QPainterPath path2;
    path2.moveTo(start2);
    path2.cubicTo(cp3, cp4, end2);

    QPen pen2(applyOpacity(props.strokeColor, props.opacity * 0.72),
              std::max(0.7, props.strokeWidth * 0.85),
              props.strokeStyle, Qt::RoundCap, Qt::RoundJoin);
    painter.setPen(pen2);
    painter.drawPath(path2);
}

void RoughRenderer::drawHachureFill(QPainter& painter, const QPainterPath& clipPath,
                                    const QRectF& bounds, const ItemProperties& props,
                                    SeededRandom& rng, double angleDegrees)
{
    if (props.fillColor == Qt::transparent || props.fillColor.alpha() == 0) {
        return;
    }

    painter.save();
    painter.setClipPath(clipPath);

    // Параметры штриховки
    const double angleRad = angleDegrees * (Pi / 180.0);
    const double gap = std::max(4.0, props.strokeWidth * 4.0);

    const double cosA = std::cos(angleRad);
    const double sinA = std::sin(angleRad);

    const QPointF center = bounds.center();
    const double maxDim = std::hypot(bounds.width(), bounds.height()) * 0.65 + 20.0;

    ItemProperties fillLineProps = props;
    fillLineProps.strokeColor = props.fillColor;
    fillLineProps.strokeWidth = std::max(1.0, props.strokeWidth * 0.8);
    fillLineProps.strokeStyle = Qt::SolidLine;

    // Направление перпендикулярного шага — (-sinA, cosA), направление линии — (cosA, sinA)
    for (double offset = -maxDim; offset <= maxDim; offset += gap) {
        QPointF mid = center + QPointF(-sinA * offset, cosA * offset);

        // Небольшой джиттер конечных точек (+- 2px)
        const double jitter = (props.roughness > 0) ? rng.nextInRange(-2.0, 2.0) : 0.0;
        QPointF p1 = mid - QPointF(cosA * (maxDim + jitter), sinA * (maxDim + jitter));
        QPointF p2 = mid + QPointF(cosA * (maxDim - jitter), sinA * (maxDim - jitter));

        drawRoughSegment(painter, p1, p2, fillLineProps, rng);
    }

    painter.restore();
}

void RoughRenderer::drawRect(QPainter& painter, const QRectF& rect, const ItemProperties& props)
{
    SeededRandom rng(props.seed);

    QPainterPath path;
    path.addRect(rect);

    // 1. Рисуем заливку
    if (props.fillStyle == FillStyle::Solid) {
        painter.fillPath(path, applyOpacity(props.fillColor, props.opacity));
    } else if (props.fillStyle == FillStyle::Hachure) {
        drawHachureFill(painter, path, rect, props, rng, 45.0);
    } else if (props.fillStyle == FillStyle::CrossHachure) {
        drawHachureFill(painter, path, rect, props, rng, 45.0);
        drawHachureFill(painter, path, rect, props, rng, -45.0);
    }

    // 2. Рисуем 4 грубые стороны с лёгким выходом за границы
    const QPointF tl = rect.topLeft();
    const QPointF tr = rect.topRight();
    const QPointF br = rect.bottomRight();
    const QPointF bl = rect.bottomLeft();

    drawRoughSegment(painter, tl, tr, props, rng);
    drawRoughSegment(painter, tr, br, props, rng);
    drawRoughSegment(painter, br, bl, props, rng);
    drawRoughSegment(painter, bl, tl, props, rng);
}

void RoughRenderer::drawRoughArc(QPainter& painter, const QPointF& center, double rx, double ry,
                                 double startAngle, double sweepAngle,
                                 const ItemProperties& props, SeededRandom& rng,
                                 double jitterScale, double opacityMultiplier)
{
    const int segments = 12;
    const double step = sweepAngle / segments;

    QPainterPath path;
    bool started = false;

    for (int i = 0; i < segments; ++i) {
        double a1 = startAngle + i * step;
        double a2 = startAngle + (i + 1) * step;
        double da = a2 - a1;

        double r1x = rx + rng.nextInRange(-jitterScale, jitterScale);
        double r1y = ry + rng.nextInRange(-jitterScale, jitterScale);
        double r2x = rx + rng.nextInRange(-jitterScale, jitterScale);
        double r2y = ry + rng.nextInRange(-jitterScale, jitterScale);

        QPointF p1(center.x() + r1x * std::cos(a1), center.y() + r1y * std::sin(a1));
        QPointF p2(center.x() + r2x * std::cos(a2), center.y() + r2y * std::sin(a2));

        if (!started) {
            path.moveTo(p1);
            started = true;
        }

        double k = (4.0 / 3.0) * std::tan(da / 4.0);
        QPointF cp1 = p1 + QPointF(-k * r1x * std::sin(a1), k * r1y * std::cos(a1));
        QPointF cp2 = p2 - QPointF(-k * r2x * std::sin(a2), k * r2y * std::cos(a2));

        // Добавляем лёгкий джиттер к управляющим точкам
        cp1 += QPointF(rng.nextInRange(-jitterScale * 0.5, jitterScale * 0.5),
                       rng.nextInRange(-jitterScale * 0.5, jitterScale * 0.5));
        cp2 += QPointF(rng.nextInRange(-jitterScale * 0.5, jitterScale * 0.5),
                       rng.nextInRange(-jitterScale * 0.5, jitterScale * 0.5));

        path.cubicTo(cp1, cp2, p2);
    }

    QPen pen(applyOpacity(props.strokeColor, props.opacity * opacityMultiplier),
             std::max(0.8, props.strokeWidth * (opacityMultiplier < 1.0 ? 0.85 : 1.0)),
             props.strokeStyle, Qt::RoundCap, Qt::RoundJoin);
    painter.setPen(pen);
    painter.setBrush(Qt::NoBrush);
    painter.drawPath(path);
}

void RoughRenderer::drawEllipse(QPainter& painter, const QRectF& rect, const ItemProperties& props)
{
    SeededRandom rng(props.seed);

    QPainterPath path;
    path.addEllipse(rect);

    // 1. Рисуем заливку
    if (props.fillStyle == FillStyle::Solid) {
        painter.fillPath(path, applyOpacity(props.fillColor, props.opacity));
    } else if (props.fillStyle == FillStyle::Hachure) {
        drawHachureFill(painter, path, rect, props, rng, 45.0);
    } else if (props.fillStyle == FillStyle::CrossHachure) {
        drawHachureFill(painter, path, rect, props, rng, 45.0);
        drawHachureFill(painter, path, rect, props, rng, -45.0);
    }

    const QPointF center = rect.center();
    const double rx = rect.width() / 2.0;
    const double ry = rect.height() / 2.0;

    if (rx < 0.5 || ry < 0.5) {
        return;
    }

    if (props.roughness < 0.05) {
        QPen pen(applyOpacity(props.strokeColor, props.opacity), props.strokeWidth, props.strokeStyle, Qt::RoundCap, Qt::RoundJoin);
        painter.setPen(pen);
        painter.setBrush(Qt::NoBrush);
        painter.drawEllipse(rect);
        return;
    }

    // Грубый эллипс: 2 перекрывающихся витка со случайным стартом и лёгким выходом за границы
    const double startA1 = rng.nextInRange(0.0, 0.5);
    const double sweepA1 = 2.0 * Pi + rng.nextInRange(0.1, 0.25);
    drawRoughArc(painter, center, rx, ry, startA1, sweepA1, props, rng, props.roughness * 1.5, 1.0);

    const double startA2 = rng.nextInRange(0.0, 0.5);
    const double sweepA2 = 2.0 * Pi + rng.nextInRange(0.1, 0.25);
    drawRoughArc(painter, center, rx, ry, startA2, sweepA2, props, rng, props.roughness * 2.8, 0.72);
}

void RoughRenderer::drawDiamond(QPainter& painter, const QRectF& rect, const ItemProperties& props)
{
    SeededRandom rng(props.seed);

    const QPointF top(rect.center().x(), rect.top());
    const QPointF right(rect.right(), rect.center().y());
    const QPointF bottom(rect.center().x(), rect.bottom());
    const QPointF left(rect.left(), rect.center().y());

    QPainterPath path;
    path.moveTo(top);
    path.lineTo(right);
    path.lineTo(bottom);
    path.lineTo(left);
    path.closeSubpath();

    // 1. Заливка
    if (props.fillStyle == FillStyle::Solid) {
        painter.fillPath(path, applyOpacity(props.fillColor, props.opacity));
    } else if (props.fillStyle == FillStyle::Hachure) {
        drawHachureFill(painter, path, rect, props, rng, 45.0);
    } else if (props.fillStyle == FillStyle::CrossHachure) {
        drawHachureFill(painter, path, rect, props, rng, 45.0);
        drawHachureFill(painter, path, rect, props, rng, -45.0);
    }

    // 2. Обводка 4 грубых рёбер
    drawRoughSegment(painter, top, right, props, rng);
    drawRoughSegment(painter, right, bottom, props, rng);
    drawRoughSegment(painter, bottom, left, props, rng);
    drawRoughSegment(painter, left, top, props, rng);
}

void RoughRenderer::drawLine(QPainter& painter, const QPointF& p1, const QPointF& p2, const ItemProperties& props)
{
    SeededRandom rng(props.seed);
    drawRoughSegment(painter, p1, p2, props, rng);
}

void RoughRenderer::drawArrow(QPainter& painter, const QPointF& p1, const QPointF& p2, const ItemProperties& props)
{
    SeededRandom rng(props.seed);

    // Рисуем основной стержень стрелки
    drawRoughSegment(painter, p1, p2, props, rng);

    // Вычисляем наконечник стрелки
    const double dx = p2.x() - p1.x();
    const double dy = p2.y() - p1.y();
    const double len = std::hypot(dx, dy);

    if (len < 2.0) {
        return;
    }

    const double angle = std::atan2(dy, dx);
    const double headLen = std::clamp(len * 0.25, 12.0, 26.0);
    constexpr double headAngle = 30.0 * (Pi / 180.0);

    const QPointF wing1 = p2 - QPointF(headLen * std::cos(angle - headAngle),
                                       headLen * std::sin(angle - headAngle));
    const QPointF wing2 = p2 - QPointF(headLen * std::cos(angle + headAngle),
                                       headLen * std::sin(angle + headAngle));

    drawRoughSegment(painter, p2, wing1, props, rng);
    drawRoughSegment(painter, p2, wing2, props, rng);
}

void RoughRenderer::drawFreehand(QPainter& painter, const QVector<QPointF>& points, const ItemProperties& props)
{
    if (points.size() < 2) {
        if (points.size() == 1) {
            painter.setBrush(applyOpacity(props.strokeColor, props.opacity));
            painter.setPen(Qt::NoPen);
            painter.drawEllipse(points[0], props.strokeWidth * 0.5, props.strokeWidth * 0.5);
        }
        return;
    }

    // Преобразование Catmull-Rom → кубический Безье.
    // Для каждой внутренней точки Pi две управляющие точки Безье:
    //   исходящая контрольная точка: CP_out = Pi + (P_{i+1} - P_{i-1}) / 6
    //   входящая контрольная точка:  CP_in  = P_{i+1} - (P_{i+2} - Pi) / 6
    // На границах: зажимаем фантомные точки (дублируем первую/последнюю).
    // В основном проходе используется НУЛЕВОЙ джиттер — плавность обеспечивается
    // исключительно интерполяцией сплайна, поэтому углы всегда красиво скруглены.
    // Шероховатость влияет только на необязательный второй «призрачный» проход с пониженной непрозрачностью.
    auto buildCatmullRom = [&](double jitterAmt) -> QPainterPath {
        SeededRandom rng(props.seed + 7);   // другое зерно, отличное от фигур

        // Формируем рабочий список точек, при необходимости возмущая каждую
        QVector<QPointF> pts;
        pts.reserve(points.size());
        for (const auto& p : points) {
            if (jitterAmt > 0.01) {
                pts.append(p + QPointF(rng.nextInRange(-jitterAmt, jitterAmt),
                                       rng.nextInRange(-jitterAmt, jitterAmt)));
            } else {
                pts.append(p);
            }
        }

        const int n = pts.size();
        QPainterPath path;
        path.moveTo(pts[0]);

        for (int i = 0; i < n - 1; ++i) {
            // Зажимаем индекс в допустимом диапазоне для фантомных граничных точек
            const QPointF& p0 = pts[std::max(0, i - 1)];
            const QPointF& p1 = pts[i];
            const QPointF& p2 = pts[i + 1];
            const QPointF& p3 = pts[std::min(n - 1, i + 2)];

            // Catmull-Rom → кубический Безье
            const QPointF cp1 = p1 + (p2 - p0) / 6.0;
            const QPointF cp2 = p2 - (p3 - p1) / 6.0;
            path.cubicTo(cp1, cp2, p2);
        }
        return path;
    };

    const double roughness = std::max(0.0, props.roughness);

    // ── Основной проход: всегда гладкий (нулевой джиттер) ────────────────────────────
    QPen pen1(applyOpacity(props.strokeColor, props.opacity),
              props.strokeWidth,
              props.strokeStyle,
              Qt::RoundCap,   // плавные завершения линий
              Qt::RoundJoin); // плавные угловые соединения
    painter.setPen(pen1);
    painter.setBrush(Qt::NoBrush);
    painter.drawPath(buildCatmullRom(0.0));

    // ── Вторичный проход: эскизный «призрак» (только при значимой шероховатости) ────
    if (roughness > 0.5) {
        // Смещение призрака пропорционально шероховатости, но остаётся едва заметным
        const double ghostJitter = roughness * 1.2;
        QPen pen2(applyOpacity(props.strokeColor, props.opacity * 0.45),
                  std::max(0.5, props.strokeWidth * 0.7),
                  props.strokeStyle,
                  Qt::RoundCap,
                  Qt::RoundJoin);
        painter.setPen(pen2);
        painter.drawPath(buildCatmullRom(ghostJitter));
    }
}

} // namespace Whiteboard
