#pragma once

#include "items/BaseItem.h"
#include <QFont>
#include <QString>
#include <QColor>

namespace Whiteboard {

class StickyNoteItem : public BaseItem {
public:
    explicit StickyNoteItem(const QString& text = QStringLiteral("Заметка"),
                            const QPointF& pos = QPointF(0, 0),
                            QGraphicsItem* parent = nullptr);

    QRectF boundingRect() const override;
    void paint(QPainter* painter, const QStyleOptionGraphicsItem* option, QWidget* widget) override;

    QRectF geometryRect() const override { return m_rect; }
    void setGeometryRect(const QRectF& rect) override;

    void resizeByHandle(Handle handle, const QPointF& itemPos) override;

    QString text() const { return m_text; }
    void setText(const QString& text);

    QFont font() const { return m_font; }
    void setFont(const QFont& font);

    Qt::Alignment textAlignment() const { return m_alignment; }
    void setTextAlignment(Qt::Alignment align);

    QColor noteColor() const { return m_noteColor; }
    void setNoteColor(const QColor& color);

    bool isEditing() const { return m_isEditing; }
    void setEditing(bool editing);

    std::unique_ptr<BaseItem> clone() const override;
    QJsonObject toJson() const override;
    void fromJson(const QJsonObject& json) override;

    // Палитра пастельных цветов для стикеров
    static const QList<QColor>& pastelPalette();

protected:
    void mouseDoubleClickEvent(QGraphicsSceneMouseEvent* event) override;

private:
    QString       m_text;
    QFont         m_font;
    QRectF        m_rect;
    Qt::Alignment m_alignment{Qt::AlignCenter};
    QColor        m_noteColor{QColor("#fff3bf")}; // Жёлтый пастельный по умолчанию
    bool          m_isEditing{false};
};

} // namespace Whiteboard
