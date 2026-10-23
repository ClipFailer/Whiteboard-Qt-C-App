#include "items/ImageItem.h"
#include <QPainter>
#include <QBuffer>
#include <QGuiApplication>
#include <algorithm>

namespace Whiteboard {

ImageItem::ImageItem(const QImage& image, const QPointF& pos, QGraphicsItem* parent)
    : BaseItem(ItemType::Image, parent)
    , m_image(image)
{
    setPos(pos);

    if (!m_image.isNull()) {
        constexpr double maxInitialSize = 600.0;
        QSizeF origSize = m_image.size();
        if (origSize.width() > maxInitialSize || origSize.height() > maxInitialSize) {
            origSize.scale(maxInitialSize, maxInitialSize, Qt::KeepAspectRatio);
        }
        m_rect = QRectF(0, 0, std::max(20.0, origSize.width()), std::max(20.0, origSize.height()));
    } else {
        m_rect = QRectF(0, 0, 100, 100);
    }
}

QRectF ImageItem::boundingRect() const
{
    const double margin = std::max(8.0, HandleSize);
    return m_rect.adjusted(-margin, -margin, margin, margin);
}

void ImageItem::paint(QPainter* painter, const QStyleOptionGraphicsItem* option, QWidget* widget)
{
    Q_UNUSED(option);
    Q_UNUSED(widget);

    painter->save();
    painter->setRenderHint(QPainter::Antialiasing);
    painter->setRenderHint(QPainter::SmoothPixmapTransform);
    painter->setOpacity(std::clamp(m_properties.opacity, 0.0, 1.0));

    if (!m_image.isNull()) {
        painter->drawImage(m_rect, m_image);
    } else {
        // Заглушка, если изображение отсутствует
        painter->setPen(QPen(Qt::gray, 1.0, Qt::DashLine));
        painter->setBrush(QColor(240, 240, 240));
        painter->drawRect(m_rect);
        painter->setPen(Qt::darkGray);
        painter->drawText(m_rect, Qt::AlignCenter, QStringLiteral("Нет изображения"));
    }

    painter->restore();

    if (isSelected()) {
        paintSelectionHandles(painter);
    }
}

void ImageItem::setGeometryRect(const QRectF& rect)
{
    prepareGeometryChange();
    m_rect = rect;
    update();
}

void ImageItem::setImage(const QImage& image)
{
    prepareGeometryChange();
    m_image = image;
    if (!m_image.isNull()) {
        m_rect = QRectF(0, 0, m_image.width(), m_image.height());
    }
    update();
}

void ImageItem::resizeByHandle(Handle handle, const QPointF& itemPos)
{
    if (!m_isResizing || m_activeResizeHandle != handle) {
        m_isResizing = true;
        m_activeResizeHandle = handle;
        m_startRect = m_rect;
        m_startPos = pos();
    }

    const QPointF scenePos = mapToScene(itemPos);
    const QPointF curPos = scenePos - m_startPos;

    const double w0 = std::max(10.0, m_startRect.width());
    const double h0 = std::max(10.0, m_startRect.height());

    const bool isShift = (QGuiApplication::keyboardModifiers() & Qt::ShiftModifier);
    const bool isCorner = (handle == Handle::TopLeft || handle == Handle::TopRight ||
                           handle == Handle::BottomLeft || handle == Handle::BottomRight);
    // Сохранение пропорций активно при зажатом Shift либо по умолчанию для угловых маркеров
    const bool keepAspect = isShift || isCorner;

    double wNew = w0;
    double hNew = h0;

    enum class Anchor {
        TopLeft,
        BottomRight,
        TopRight,
        BottomLeft,
        MidLeft,
        TopCenter
    };
    Anchor anchor = Anchor::TopLeft;

    if (keepAspect) {
        double scale = 1.0;
        switch (handle) {
        case Handle::BottomRight:
            scale = (curPos.x() * w0 + curPos.y() * h0) / (w0 * w0 + h0 * h0);
            anchor = Anchor::TopLeft;
            break;
        case Handle::TopLeft:
            scale = ((w0 - curPos.x()) * w0 + (h0 - curPos.y()) * h0) / (w0 * w0 + h0 * h0);
            anchor = Anchor::BottomRight;
            break;
        case Handle::TopRight:
            scale = (curPos.x() * w0 + (h0 - curPos.y()) * h0) / (w0 * w0 + h0 * h0);
            anchor = Anchor::BottomLeft;
            break;
        case Handle::BottomLeft:
            scale = ((w0 - curPos.x()) * w0 + curPos.y() * h0) / (w0 * w0 + h0 * h0);
            anchor = Anchor::TopRight;
            break;
        case Handle::MidRight:
            scale = curPos.x() / w0;
            anchor = Anchor::TopLeft;
            break;
        case Handle::MidLeft:
            scale = (w0 - curPos.x()) / w0;
            anchor = Anchor::MidLeft;
            break;
        case Handle::BottomCenter:
            scale = curPos.y() / h0;
            anchor = Anchor::TopLeft;
            break;
        case Handle::TopCenter:
            scale = (h0 - curPos.y()) / h0;
            anchor = Anchor::TopCenter;
            break;
        default:
            return;
        }
        scale = std::clamp(scale, 0.05, 20.0);
        wNew = std::max(10.0, w0 * scale);
        hNew = std::max(10.0, h0 * scale);
    } else {
        // Свободное растяжение боковых маркеров без зажатого Shift
        switch (handle) {
        case Handle::MidRight:
            wNew = std::max(10.0, curPos.x());
            anchor = Anchor::TopLeft;
            break;
        case Handle::MidLeft:
            wNew = std::max(10.0, w0 - curPos.x());
            anchor = Anchor::MidLeft;
            break;
        case Handle::BottomCenter:
            hNew = std::max(10.0, curPos.y());
            anchor = Anchor::TopLeft;
            break;
        case Handle::TopCenter:
            hNew = std::max(10.0, h0 - curPos.y());
            anchor = Anchor::TopCenter;
            break;
        default:
            break;
        }
    }

    prepareGeometryChange();
    m_rect = QRectF(0, 0, wNew, hNew);

    switch (anchor) {
    case Anchor::TopLeft:
        setPos(m_startPos);
        break;
    case Anchor::BottomRight:
        setPos(m_startPos + QPointF(w0 - wNew, h0 - hNew));
        break;
    case Anchor::TopRight:
        setPos(m_startPos + QPointF(w0 - wNew, 0.0));
        break;
    case Anchor::BottomLeft:
        setPos(m_startPos + QPointF(0.0, h0 - hNew));
        break;
    case Anchor::MidLeft:
        setPos(m_startPos + QPointF(w0 - wNew, 0.0));
        break;
    case Anchor::TopCenter:
        setPos(m_startPos + QPointF(0.0, h0 - hNew));
        break;
    }

    update();
}

void ImageItem::finishResize()
{
    m_isResizing = false;
    m_activeResizeHandle = Handle::None;
}

std::unique_ptr<BaseItem> ImageItem::clone() const
{
    auto copy = std::make_unique<ImageItem>(m_image, pos());
    copy->setProperties(m_properties);
    copy->setGeometryRect(m_rect);
    copy->setRotation(rotation());
    copy->setZValue(zValue());
    return copy;
}

QJsonObject ImageItem::toJson() const
{
    QJsonObject json = BaseItem::toJson();
    json[QStringLiteral("type")] = QStringLiteral("image");
    json[QStringLiteral("width")] = m_rect.width();
    json[QStringLiteral("height")] = m_rect.height();

    if (!m_image.isNull()) {
        QByteArray bytes;
        QBuffer buffer(&bytes);
        buffer.open(QIODevice::WriteOnly);
        m_image.save(&buffer, "PNG");
        json[QStringLiteral("data")] = QString::fromLatin1(bytes.toBase64());
    }

    return json;
}

void ImageItem::fromJson(const QJsonObject& json)
{
    BaseItem::fromJson(json);
    double w = json[QStringLiteral("width")].toDouble(100.0);
    double h = json[QStringLiteral("height")].toDouble(100.0);
    m_rect = QRectF(0, 0, w, h);

    if (json.contains(QStringLiteral("data"))) {
        QByteArray bytes = QByteArray::fromBase64(json[QStringLiteral("data")].toString().toLatin1());
        m_image.loadFromData(bytes, "PNG");
    }
}

} // namespace Whiteboard
