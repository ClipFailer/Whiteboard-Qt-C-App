#pragma once

#include "items/BaseItem.h"
#include "tools/ToolManager.h"
#include <QGraphicsScene>
#include <QUndoStack>
#include <QList>
#include <memory>

class QGraphicsRectItem;

namespace Whiteboard {

class TextItem;
class StickyNoteItem;
class LineItem;

class CanvasScene : public QGraphicsScene {
    Q_OBJECT

public:
    explicit CanvasScene(ToolManager* toolManager, QUndoStack* undoStack, QObject* parent = nullptr);
    ~CanvasScene() override = default;

    ToolManager* toolManager() const { return m_toolManager; }
    QUndoStack* undoStack() const { return m_undoStack; }

    QList<BaseItem*> whiteboardItems() const;
    QList<BaseItem*> selectedWhiteboardItems() const;

    bool isSnapToGrid() const { return m_snapToGrid; }
    void setSnapToGrid(bool snap) { m_snapToGrid = snap; }

    void deleteSelectedItems();
    void duplicateSelectedItems();
    void selectAllItems();
    void bringSelectedToFront();
    void sendSelectedToBack();

    void groupSelectedItems();
    void ungroupSelectedItems();
    void toggleLockSelectedItems();

    // Выравнивание и распределение элементов
    void alignLeft();
    void alignCenterH();
    void alignRight();
    void alignTop();
    void alignCenterV();
    void alignBottom();
    void distributeHorizontally();
    void distributeVertically();

    // Обновление динамических соединителей (стрелок/линий)
    void updateConnectorsForItems(const QList<BaseItem*>& movedItems);

    struct AnchorMatch {
        BaseItem* item{nullptr};
        int anchorIndex{-1};
        QPointF scenePos;
        bool isValid() const { return item != nullptr && anchorIndex >= 0; }
    };
    AnchorMatch findNearestAnchor(const QPointF& scenePos, double maxDist = 20.0, BaseItem* excludeItem = nullptr) const;

signals:
    void editTextRequested(TextItem* item);
    void editStickyNoteRequested(StickyNoteItem* item);
    void createTextRequested(const QPointF& scenePos);

protected:
    void mousePressEvent(QGraphicsSceneMouseEvent* event) override;
    void mouseMoveEvent(QGraphicsSceneMouseEvent* event) override;
    void mouseReleaseEvent(QGraphicsSceneMouseEvent* event) override;
    void mouseDoubleClickEvent(QGraphicsSceneMouseEvent* event) override;
    void drawForeground(QPainter* painter, const QRectF& rect) override;

private:
    void handleSelectPress(QGraphicsSceneMouseEvent* event);
    void handleSelectMove(QGraphicsSceneMouseEvent* event);
    void handleSelectRelease(QGraphicsSceneMouseEvent* event);

    void handleShapePress(QGraphicsSceneMouseEvent* event);
    void handleShapeMove(QGraphicsSceneMouseEvent* event);
    void handleShapeRelease(QGraphicsSceneMouseEvent* event);

    void handleFreehandPress(QGraphicsSceneMouseEvent* event);
    void handleFreehandMove(QGraphicsSceneMouseEvent* event);
    void handleFreehandRelease(QGraphicsSceneMouseEvent* event);

    void handleEraserAt(const QPointF& scenePos);

    ToolManager* m_toolManager;
    QUndoStack* m_undoStack;

    // Состояния взаимодействия
    enum class DragMode { None, Move, Resize, RubberBand, DrawingShape, DrawingFreehand, Erasing };
    DragMode m_dragMode{DragMode::None};

    QPointF m_startPoint;
    QPointF m_lastPoint;

    // Изменение размера
    BaseItem* m_resizingItem{nullptr};
    Handle m_currentHandle{Handle::None};
    QRectF m_origGeometryRect;
    QPointF m_origItemPos;

    // Перемещение
    struct MoveInfo {
        BaseItem* item;
        QPointF startPos;
    };
    QList<MoveInfo> m_movingItems;

    // Рамка выделения
    QGraphicsRectItem* m_rubberBandItem{nullptr};

    // Предварительный просмотр рисуемой фигуры
    BaseItem* m_previewItem{nullptr};

    // Привязка к сетке, умные направляющие и точки привязки
    bool m_snapToGrid{true};
    QList<QLineF> m_smartGuides;
    bool m_hasActiveSnapAnchor{false};
    QPointF m_activeSnapAnchorPoint;
    AnchorMatch m_drawingStartAnchor;
};

} // namespace Whiteboard
