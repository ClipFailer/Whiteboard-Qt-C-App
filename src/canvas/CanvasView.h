#pragma once

#include "canvas/CanvasScene.h"
#include <QGraphicsView>
#include <QTextEdit>

namespace Whiteboard {

class TextItem;
class StickyNoteItem;

class CanvasView : public QGraphicsView {
    Q_OBJECT

public:
    explicit CanvasView(CanvasScene* scene, QWidget* parent = nullptr);

    double zoomFactor() const { return m_zoomFactor; }
    void setZoom(double factor);
    void zoomIn();
    void zoomOut();
    void resetZoom();
    void zoomToFit();

    bool isGridVisible() const { return m_gridVisible; }
    void setGridVisible(bool visible);

    bool isDarkMode() const { return m_darkMode; }
    void setDarkMode(bool dark);

signals:
    void zoomChanged(double factor);

public slots:
    void startTextEditing(TextItem* item);
    void startStickyNoteEditing(StickyNoteItem* note);
    void createTextAt(const QPointF& scenePos);

protected:
    void wheelEvent(QWheelEvent* event) override;
    void keyPressEvent(QKeyEvent* event) override;
    void keyReleaseEvent(QKeyEvent* event) override;
    void mousePressEvent(QMouseEvent* event) override;
    void mouseMoveEvent(QMouseEvent* event) override;
    void mouseReleaseEvent(QMouseEvent* event) override;
    void drawBackground(QPainter* painter, const QRectF& rect) override;
    void resizeEvent(QResizeEvent* event) override;
    void dragEnterEvent(QDragEnterEvent* event) override;
    void dragMoveEvent(QDragMoveEvent* event) override;
    void dropEvent(QDropEvent* event) override;

private:
    void finishTextEditing();
    void updateEditorGeometry();

    CanvasScene* m_scene;
    double m_zoomFactor{1.0};
    bool m_gridVisible{true};
    bool m_darkMode{false};

    // Состояние панорамирования
    bool m_isPanning{false};
    bool m_spacePressed{false};
    QPoint m_lastPanPoint;

    // Встроенный текстовый редактор
    QTextEdit* m_textEditor{nullptr};
    TextItem* m_editingItem{nullptr};
    StickyNoteItem* m_editingStickyNote{nullptr};
    bool m_isCreatingNewText{false};
    QPointF m_newTextScenePos;
};

} // namespace Whiteboard
