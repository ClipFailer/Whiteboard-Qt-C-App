#pragma once

#include "items/BaseItem.h"
#include <QImage>

namespace Whiteboard {

class ImageItem : public BaseItem {
public:
    explicit ImageItem(const QImage& image = QImage(),
                       const QPointF& pos = QPointF(0, 0),
                       QGraphicsItem* parent = nullptr);

    QRectF boundingRect() const override;
    void paint(QPainter* painter, const QStyleOptionGraphicsItem* option, QWidget* widget) override;

    QRectF geometryRect() const override { return m_rect; }
    void setGeometryRect(const QRectF& rect) override;

    // Масштабирует изображение с сохранением пропорций при перетаскивании маркеров
    void resizeByHandle(Handle handle, const QPointF& itemPos) override;
    void finishResize() override;

    const QImage& image() const { return m_image; }
    void setImage(const QImage& image);

    std::unique_ptr<BaseItem> clone() const override;
    QJsonObject toJson() const override;
    void fromJson(const QJsonObject& json) override;

private:
    QImage  m_image;
    QRectF  m_rect;

    // Параметры сеанса изменения размера маркерами
    bool    m_isResizing{false};
    Handle  m_activeResizeHandle{Handle::None};
    QRectF  m_startRect;
    QPointF m_startPos;
};

} // namespace Whiteboard
