#pragma once

#include "items/BaseItem.h"
#include <QFont>
#include <QString>

namespace Whiteboard {

class TextItem : public BaseItem {
public:
    explicit TextItem(const QString& text = QStringLiteral("Text"),
                      const QPointF& pos = QPointF(0, 0),
                      QGraphicsItem* parent = nullptr);

    QRectF boundingRect() const override;
    void paint(QPainter* painter, const QStyleOptionGraphicsItem* option, QWidget* widget) override;

    QRectF geometryRect() const override;
    // setGeometryRect: изменяет размер текстового блока — текст переносится по новой ширине.
    void setGeometryRect(const QRectF& rect) override;

    // resizeByHandle: масштабирует размер шрифта и блока текста при растяжении за маркеры.
    void resizeByHandle(Handle handle, const QPointF& itemPos) override;
    void finishResize() override;

    QString text() const { return m_text; }
    void setText(const QString& text);

    QFont font() const { return m_font; }
    void setFont(const QFont& font);
    qreal fontSize() const { return m_font.pointSizeF() > 0 ? m_font.pointSizeF() : static_cast<qreal>(m_font.pointSize()); }
    void setFontSize(int pointSize);
    void setFontSize(qreal pointSize);

    bool isEditing() const { return m_isEditing; }
    void setEditing(bool editing);

    std::unique_ptr<BaseItem> clone() const override;
    QJsonObject toJson() const override;
    void fromJson(const QJsonObject& json) override;

protected:
    void mouseDoubleClickEvent(QGraphicsSceneMouseEvent* event) override;

private:
    // Пересчитывает m_rect на основе текущих m_text, m_font и m_userWidth.
    // Если m_userWidth <= 0, блок автоматически подстраивается под текст (без переноса).
    // Если m_userWidth > 0, текст переносится по этой ширине.
    void recalcRect();

    QString m_text;
    QFont   m_font;
    QRectF  m_rect;

    // Когда пользователь задаёт ширину блока вручную, сохраняем её,
    // чтобы текст переносился по ней. 0 означает «авто» (размер по содержимому).
    double m_userWidth{0.0};

    bool m_isEditing{false};

    // Параметры активного сеанса растяжения маркеров
    bool   m_isResizing{false};
    Handle m_activeResizeHandle{Handle::None};
    QRectF m_startRect;
    QPointF m_startPos;
    qreal  m_startFontSize{18.0};
    double m_startUserWidth{0.0};
};

} // namespace Whiteboard
