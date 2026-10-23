#include "canvas/CanvasScene.h"
#include "items/RectangleItem.h"
#include "items/EllipseItem.h"
#include "items/DiamondItem.h"
#include "items/LineItem.h"
#include "items/ArrowItem.h"
#include "items/FreehandItem.h"
#include "items/TextItem.h"
#include "items/StickyNoteItem.h"
#include "items/ImageItem.h"
#include "commands/AddItemCommand.h"
#include "commands/RemoveItemCommand.h"
#include "commands/MoveItemCommand.h"
#include "commands/ChangePropertyCommand.h"
#include "commands/GroupCommand.h"
#include "commands/ToggleLockCommand.h"

#include <QPainter>
#include <QGraphicsSceneMouseEvent>
#include <QGraphicsRectItem>
#include <QKeyEvent>
#include <cmath>
#include <algorithm>

namespace Whiteboard {

CanvasScene::CanvasScene(ToolManager* toolManager, QUndoStack* undoStack, QObject* parent)
    : QGraphicsScene(parent)
    , m_toolManager(toolManager)
    , m_undoStack(undoStack)
{
    // Бесконечные границы сцены
    setSceneRect(-50000.0, -50000.0, 100000.0, 100000.0);
}

QList<BaseItem*> CanvasScene::whiteboardItems() const
{
    QList<BaseItem*> result;
    for (auto* item : items()) {
        if (item == m_previewItem || item == m_rubberBandItem) {
            continue;
        }
        if (auto* wbItem = dynamic_cast<BaseItem*>(item)) {
            result.append(wbItem);
        }
    }
    return result;
}

QList<BaseItem*> CanvasScene::selectedWhiteboardItems() const
{
    QList<BaseItem*> result;
    for (auto* item : selectedItems()) {
        if (item == m_previewItem || item == m_rubberBandItem) {
            continue;
        }
        if (auto* wbItem = dynamic_cast<BaseItem*>(item)) {
            result.append(wbItem);
        }
    }
    return result;
}

void CanvasScene::deleteSelectedItems()
{
    auto selected = selectedWhiteboardItems();
    QList<BaseItem*> toDelete;
    for (auto* it : selected) {
        if (!it->isLocked()) {
            toDelete.append(it);
        }
    }
    if (toDelete.isEmpty()) {
        return;
    }
    m_undoStack->push(new RemoveItemCommand(this, toDelete));
}

void CanvasScene::duplicateSelectedItems()
{
    auto selected = selectedWhiteboardItems();
    if (selected.isEmpty()) {
        return;
    }

    clearSelection();
    m_undoStack->beginMacro(QStringLiteral("Duplicate"));
    for (auto* orig : selected) {
        auto copy = orig->clone();
        copy->setPos(orig->pos() + QPointF(20, 20));
        BaseItem* rawCopy = copy.release();
        m_undoStack->push(new AddItemCommand(this, rawCopy));
        rawCopy->setSelected(true);
    }
    m_undoStack->endMacro();
}

void CanvasScene::selectAllItems()
{
    for (auto* item : whiteboardItems()) {
        item->setSelected(true);
    }
}

void CanvasScene::bringSelectedToFront()
{
    double maxZ = 0.0;
    for (auto* item : whiteboardItems()) {
        maxZ = std::max(maxZ, item->zValue());
    }
    for (auto* item : selectedWhiteboardItems()) {
        item->setZValue(maxZ + 1.0);
    }
}

void CanvasScene::sendSelectedToBack()
{
    double minZ = 0.0;
    for (auto* item : whiteboardItems()) {
        minZ = std::min(minZ, item->zValue());
    }
    for (auto* item : selectedWhiteboardItems()) {
        item->setZValue(minZ - 1.0);
    }
}

void CanvasScene::mousePressEvent(QGraphicsSceneMouseEvent* event)
{
    if (event->button() != Qt::LeftButton) {
        QGraphicsScene::mousePressEvent(event);
        return;
    }

    m_startPoint = event->scenePos();
    m_lastPoint = m_startPoint;

    const Tool tool = m_toolManager->currentTool();

    switch (tool) {
    case Tool::Select:
        handleSelectPress(event);
        break;
    case Tool::Hand:
        QGraphicsScene::mousePressEvent(event);
        break;
    case Tool::Rectangle:
    case Tool::Ellipse:
    case Tool::Diamond:
    case Tool::Line:
    case Tool::Arrow:
        handleShapePress(event);
        break;
    case Tool::Freehand:
        handleFreehandPress(event);
        break;
    case Tool::Text:
        emit createTextRequested(event->scenePos());
        break;
    case Tool::StickyNote: {
        auto* note = new StickyNoteItem(QStringLiteral(""), event->scenePos() - QPointF(100, 100));
        m_undoStack->push(new AddItemCommand(this, note));
        clearSelection();
        note->setSelected(true);
        m_toolManager->setTool(Tool::Select);
        emit editStickyNoteRequested(note);
        break;
    }
    case Tool::Eraser:
        m_dragMode = DragMode::Erasing;
        handleEraserAt(event->scenePos());
        break;
    }
}

void CanvasScene::handleSelectPress(QGraphicsSceneMouseEvent* event)
{
    // 1. Проверяем, не нажат ли маркер уже выделенного элемента
    for (auto* item : selectedWhiteboardItems()) {
        QPointF itemPos = item->mapFromScene(event->scenePos());
        Handle handle = item->handleAt(itemPos);
        if (handle != Handle::None) {
            m_dragMode = DragMode::Resize;
            m_resizingItem = item;
            m_currentHandle = handle;
            m_origGeometryRect = item->geometryRect();
            m_origItemPos = item->pos();
            return;
        }
    }

    // 2. Проверяем, не нажат ли какой-либо элемент
    auto* clickedItem = itemAt(event->scenePos(), QTransform());
    auto* wbItem = dynamic_cast<BaseItem*>(clickedItem);

    if (wbItem) {
        if (!wbItem->isSelected() && !(event->modifiers() & (Qt::ShiftModifier | Qt::ControlModifier))) {
            clearSelection();
            wbItem->setSelected(true);
        } else if (event->modifiers() & (Qt::ShiftModifier | Qt::ControlModifier)) {
            wbItem->setSelected(!wbItem->isSelected());
        }

        // Если элемент входит в группу, выделяем все элементы этой группы
        if (wbItem->isSelected() && !wbItem->groupId().isNull()) {
            for (auto* it : whiteboardItems()) {
                if (it->groupId() == wbItem->groupId()) {
                    it->setSelected(true);
                }
            }
        }

        m_dragMode = DragMode::Move;
        m_movingItems.clear();
        for (auto* sel : selectedWhiteboardItems()) {
            if (!sel->isLocked()) {
                m_movingItems.append({sel, sel->pos()});
            }
        }
    } else {
        // 3. Клик по пустому месту -> начинаем рамку выделения
        if (!(event->modifiers() & (Qt::ShiftModifier | Qt::ControlModifier))) {
            clearSelection();
        }
        m_dragMode = DragMode::RubberBand;
        m_rubberBandItem = new QGraphicsRectItem();
        m_rubberBandItem->setPen(QPen(QColor(25, 113, 194, 200), 1.0, Qt::DashLine));
        m_rubberBandItem->setBrush(QColor(25, 113, 194, 30));
        m_rubberBandItem->setRect(QRectF(m_startPoint, m_startPoint));
        addItem(m_rubberBandItem);
    }
}

void CanvasScene::handleShapePress(QGraphicsSceneMouseEvent* event)
{
    m_dragMode = DragMode::DrawingShape;
    const ItemProperties props = m_toolManager->defaultProperties();

    // Проверяем начальную точку на примагничивание к фигуре (для стрелок и линий)
    const Tool curTool = m_toolManager->currentTool();
    if (curTool == Tool::Line || curTool == Tool::Arrow) {
        auto startAnchor = findNearestAnchor(m_startPoint, 20.0);
        if (startAnchor.isValid()) {
            m_drawingStartAnchor = startAnchor;
            m_startPoint = startAnchor.scenePos;
        } else {
            m_drawingStartAnchor = {};
        }
    } else {
        m_drawingStartAnchor = {};
    }

    switch (curTool) {
    case Tool::Rectangle:
        m_previewItem = new RectangleItem(QRectF(0, 0, 1, 1));
        break;
    case Tool::Ellipse:
        m_previewItem = new EllipseItem(QRectF(0, 0, 1, 1));
        break;
    case Tool::Diamond:
        m_previewItem = new DiamondItem(QRectF(0, 0, 1, 1));
        break;
    case Tool::Line:
        m_previewItem = new LineItem(QPointF(0, 0), QPointF(1, 1));
        break;
    case Tool::Arrow:
        m_previewItem = new ArrowItem(QPointF(0, 0), QPointF(1, 1));
        break;
    default:
        return;
    }

    m_previewItem->setProperties(props);
    m_previewItem->setPos(m_startPoint);
    addItem(m_previewItem);
}

void CanvasScene::handleFreehandPress(QGraphicsSceneMouseEvent* event)
{
    m_dragMode = DragMode::DrawingFreehand;
    auto* freehand = new FreehandItem();
    freehand->setProperties(m_toolManager->defaultProperties());
    freehand->setPos(0, 0);
    freehand->addPoint(event->scenePos());
    m_previewItem = freehand;
    addItem(m_previewItem);
}

void CanvasScene::handleEraserAt(const QPointF& scenePos)
{
    auto hitItems = items(QRectF(scenePos.x() - 10, scenePos.y() - 10, 20, 20));
    QList<BaseItem*> toDelete;
    for (auto* it : hitItems) {
        if (auto* wb = dynamic_cast<BaseItem*>(it)) {
            toDelete.append(wb);
        }
    }
    if (!toDelete.isEmpty()) {
        m_undoStack->push(new RemoveItemCommand(this, toDelete));
    }
}

void CanvasScene::mouseMoveEvent(QGraphicsSceneMouseEvent* event)
{
    const QPointF currentPos = event->scenePos();

    switch (m_dragMode) {
    case DragMode::Resize: {
        if (m_resizingItem) {
            QPointF targetItemPos = currentPos;
            if (auto* line = dynamic_cast<LineItem*>(m_resizingItem)) {
                // Магнитное прилипание к точкам привязки при изменении концов стрелки
                auto anchor = findNearestAnchor(currentPos, 20.0, line);
                if (anchor.isValid()) {
                    targetItemPos = anchor.scenePos;
                    m_hasActiveSnapAnchor = true;
                    m_activeSnapAnchorPoint = anchor.scenePos;
                } else {
                    m_hasActiveSnapAnchor = false;
                }
            } else if (m_snapToGrid) {
                // Привязка к сетке 20px при растяжении фигуры
                constexpr double grid = 20.0;
                double snappedX = std::round(targetItemPos.x() / grid) * grid;
                double snappedY = std::round(targetItemPos.y() / grid) * grid;
                if (std::hypot(targetItemPos.x() - snappedX, targetItemPos.y() - snappedY) <= 8.0) {
                    targetItemPos = QPointF(snappedX, snappedY);
                }
            }
            m_resizingItem->resizeByHandle(m_currentHandle, m_resizingItem->mapFromScene(targetItemPos));
            update();
        }
        break;
    }
    case DragMode::Move: {
        QPointF delta = currentPos - m_startPoint;

        // 1. Привязка к сетке (Snap to Grid)
        if (m_snapToGrid && !m_movingItems.isEmpty()) {
            constexpr double grid = 20.0;
            constexpr double snapThreshold = 8.0;
            QPointF candidatePos = m_movingItems.first().startPos + delta;
            double nearestX = std::round(candidatePos.x() / grid) * grid;
            double nearestY = std::round(candidatePos.y() / grid) * grid;
            if (std::abs(candidatePos.x() - nearestX) <= snapThreshold) {
                delta.setX(nearestX - m_movingItems.first().startPos.x());
            }
            if (std::abs(candidatePos.y() - nearestY) <= snapThreshold) {
                delta.setY(nearestY - m_movingItems.first().startPos.y());
            }
        }

        // 2. Умные направляющие (Smart Guides)
        m_smartGuides.clear();
        if (!m_movingItems.isEmpty()) {
            QRectF dragRect;
            for (const auto& info : m_movingItems) {
                QRectF box = info.item->mapToScene(info.item->geometryRect()).boundingRect();
                box.translate(delta);
                dragRect = dragRect.isEmpty() ? box : dragRect.united(box);
            }

            constexpr double guideSnapThresh = 6.0;
            double snapXDelta = 0.0;
            double snapYDelta = 0.0;
            bool snappedX = false;
            bool snappedY = false;

            const double dragXCoords[3] = {dragRect.left(), dragRect.center().x(), dragRect.right()};
            const double dragYCoords[3] = {dragRect.top(), dragRect.center().y(), dragRect.bottom()};

            for (auto* other : whiteboardItems()) {
                if (!other || other->isSelected()) continue;
                QRectF oBox = other->mapToScene(other->geometryRect()).boundingRect();
                if (oBox.isEmpty() || oBox.width() <= 0) continue;

                const double oXCoords[3] = {oBox.left(), oBox.center().x(), oBox.right()};
                const double oYCoords[3] = {oBox.top(), oBox.center().y(), oBox.bottom()};

                if (!snappedX) {
                    for (double dx : dragXCoords) {
                        for (double ox : oXCoords) {
                            if (std::abs(dx - ox) <= guideSnapThresh) {
                                snapXDelta = ox - dx;
                                snappedX = true;
                                double minY = std::min(dragRect.top(), oBox.top()) - 20.0;
                                double maxY = std::max(dragRect.bottom(), oBox.bottom()) + 20.0;
                                m_smartGuides.append(QLineF(ox, minY, ox, maxY));
                                break;
                            }
                        }
                        if (snappedX) break;
                    }
                }

                if (!snappedY) {
                    for (double dy : dragYCoords) {
                        for (double oy : oYCoords) {
                            if (std::abs(dy - oy) <= guideSnapThresh) {
                                snapYDelta = oy - dy;
                                snappedY = true;
                                double minX = std::min(dragRect.left(), oBox.left()) - 20.0;
                                double maxX = std::max(dragRect.right(), oBox.right()) + 20.0;
                                m_smartGuides.append(QLineF(minX, oy, maxX, oy));
                                break;
                            }
                        }
                        if (snappedY) break;
                    }
                }
            }

            if (snappedX) delta.rx() += snapXDelta;
            if (snappedY) delta.ry() += snapYDelta;
        }

        QList<BaseItem*> movedList;
        for (const auto& info : m_movingItems) {
            info.item->setPos(info.startPos + delta);
            movedList.append(info.item);
        }

        // Обновляем прикрепленные стрелки/линии в реальном времени
        updateConnectorsForItems(movedList);
        update();
        break;
    }
    case DragMode::RubberBand: {
        if (m_rubberBandItem) {
            QRectF rect = QRectF(m_startPoint, currentPos).normalized();
            m_rubberBandItem->setRect(rect);
            for (auto* wbItem : whiteboardItems()) {
                wbItem->setSelected(rect.intersects(wbItem->mapToScene(wbItem->geometryRect()).boundingRect()));
            }
        }
        break;
    }
    case DragMode::DrawingShape: {
        if (m_previewItem) {
            QPointF effectiveEnd = currentPos;
            if (auto* line = dynamic_cast<LineItem*>(m_previewItem)) {
                // Магнитное прилипание к точкам привязки других фигур
                auto anchor = findNearestAnchor(currentPos, 20.0, line);
                if (anchor.isValid()) {
                    effectiveEnd = anchor.scenePos;
                    m_hasActiveSnapAnchor = true;
                    m_activeSnapAnchorPoint = anchor.scenePos;
                } else {
                    m_hasActiveSnapAnchor = false;
                }
                const QPointF localEnd = effectiveEnd - m_startPoint;
                line->setPoints(QPointF(0, 0), localEnd);
            } else {
                const QPointF localEnd = effectiveEnd - m_startPoint;
                QRectF rect = QRectF(QPointF(0, 0), localEnd).normalized();
                m_previewItem->setPos(QPointF(std::min(m_startPoint.x(), effectiveEnd.x()),
                                              std::min(m_startPoint.y(), effectiveEnd.y())));
                m_previewItem->setGeometryRect(QRectF(0, 0, rect.width(), rect.height()));
            }
            update();
        }
        break;
    }
    case DragMode::DrawingFreehand: {
        if (auto* freehand = dynamic_cast<FreehandItem*>(m_previewItem)) {
            constexpr double MinPointDist = 4.0;
            const auto& pts = freehand->points();
            if (pts.isEmpty()) {
                freehand->addPoint(currentPos);
            } else {
                const QPointF delta = currentPos - pts.last();
                if ((delta.x() * delta.x() + delta.y() * delta.y()) >= MinPointDist * MinPointDist) {
                    freehand->addPoint(currentPos);
                }
            }
        }
        break;
    }
    case DragMode::Erasing: {
        handleEraserAt(currentPos);
        break;
    }
    default:
        QGraphicsScene::mouseMoveEvent(event);
        break;
    }

    m_lastPoint = currentPos;
}

void CanvasScene::mouseReleaseEvent(QGraphicsSceneMouseEvent* event)
{
    if (event->button() != Qt::LeftButton) {
        QGraphicsScene::mouseReleaseEvent(event);
        return;
    }

    m_smartGuides.clear();
    m_hasActiveSnapAnchor = false;

    switch (m_dragMode) {
    case DragMode::Resize: {
        if (m_resizingItem) {
            if (auto* line = dynamic_cast<LineItem*>(m_resizingItem)) {
                auto anchor = findNearestAnchor(event->scenePos(), 20.0, line);
                if (m_currentHandle == Handle::StartPoint) {
                    if (anchor.isValid()) {
                        line->setStartBinding({anchor.item->id(), anchor.anchorIndex});
                    } else {
                        line->setStartBinding({});
                    }
                } else if (m_currentHandle == Handle::EndPoint) {
                    if (anchor.isValid()) {
                        line->setEndBinding({anchor.item->id(), anchor.anchorIndex});
                    } else {
                        line->setEndBinding({});
                    }
                }
            }
            m_resizingItem->finishResize();
        }
        m_resizingItem = nullptr;
        m_currentHandle = Handle::None;
        update();
        break;
    }
    case DragMode::Move: {
        QList<ItemMoveRecord> records;
        for (const auto& info : m_movingItems) {
            if (info.item && (info.item->pos() - info.startPos).manhattanLength() > 2.0) {
                records.append({info.item, info.startPos, info.item->pos()});
            }
        }
        if (!records.isEmpty()) {
            m_undoStack->push(new MoveItemCommand(records));
        }
        m_movingItems.clear();
        update();
        break;
    }
    case DragMode::RubberBand: {
        if (m_rubberBandItem) {
            QRectF dirtyRect = m_rubberBandItem->sceneBoundingRect().adjusted(-4, -4, 4, 4);
            removeItem(m_rubberBandItem);
            delete m_rubberBandItem;
            m_rubberBandItem = nullptr;
            invalidate(dirtyRect, QGraphicsScene::AllLayers);
        }
        break;
    }
    case DragMode::DrawingShape: {
        if (m_previewItem) {
            bool valid = false;
            if (auto* line = dynamic_cast<LineItem*>(m_previewItem)) {
                double len = std::hypot(line->p2().x() - line->p1().x(), line->p2().y() - line->p1().y());
                valid = (len >= 5.0);
                if (valid) {
                    // Привязываем начальную и конечную магнитные точки
                    if (m_drawingStartAnchor.isValid()) {
                        line->setStartBinding({m_drawingStartAnchor.item->id(), m_drawingStartAnchor.anchorIndex});
                    }
                    auto endAnchor = findNearestAnchor(event->scenePos(), 20.0, line);
                    if (endAnchor.isValid()) {
                        line->setEndBinding({endAnchor.item->id(), endAnchor.anchorIndex});
                    }
                }
            } else {
                QRectF rect = m_previewItem->geometryRect();
                valid = (rect.width() >= 5.0 && rect.height() >= 5.0);
            }

            m_drawingStartAnchor = {};

            if (valid) {
                removeItem(m_previewItem);
                m_undoStack->push(new AddItemCommand(this, m_previewItem));
                clearSelection();
                m_previewItem->setSelected(true);
                m_previewItem = nullptr;
                m_toolManager->setTool(Tool::Select);
            } else {
                removeItem(m_previewItem);
                delete m_previewItem;
                m_previewItem = nullptr;
            }
        }
        break;
    }
    case DragMode::DrawingFreehand: {
        if (auto* freehand = dynamic_cast<FreehandItem*>(m_previewItem)) {
            if (freehand->points().size() >= 2) {
                removeItem(m_previewItem);
                m_undoStack->push(new AddItemCommand(this, m_previewItem));
                clearSelection();
                m_previewItem->setSelected(true);
                m_previewItem = nullptr;
            } else {
                removeItem(m_previewItem);
                delete m_previewItem;
                m_previewItem = nullptr;
            }
        }
        break;
    }
    default:
        break;
    }

    m_dragMode = DragMode::None;
    update();
    QGraphicsScene::mouseReleaseEvent(event);
}

void CanvasScene::mouseDoubleClickEvent(QGraphicsSceneMouseEvent* event)
{
    auto* clicked = itemAt(event->scenePos(), QTransform());
    if (auto* textItem = dynamic_cast<TextItem*>(clicked)) {
        emit editTextRequested(textItem);
        return;
    }
    if (auto* noteItem = dynamic_cast<StickyNoteItem*>(clicked)) {
        emit editStickyNoteRequested(noteItem);
        return;
    }
    QGraphicsScene::mouseDoubleClickEvent(event);
}

void CanvasScene::drawForeground(QPainter* painter, const QRectF& rect)
{
    Q_UNUSED(rect);

    // 1. Умные направляющие (Smart Guides)
    if (!m_smartGuides.isEmpty()) {
        painter->save();
        QPen guidePen(QColor(230, 73, 128), 1.2, Qt::DashLine);
        painter->setPen(guidePen);
        for (const auto& line : m_smartGuides) {
            painter->drawLine(line);
            painter->setBrush(QColor(230, 73, 128));
            painter->drawEllipse(line.p1(), 2.5, 2.5);
            painter->drawEllipse(line.p2(), 2.5, 2.5);
        }
        painter->restore();
    }

    // 2. Магнитная точка привязки (Snap Anchor Target)
    if (m_hasActiveSnapAnchor) {
        painter->save();
        painter->setRenderHint(QPainter::Antialiasing);

        painter->setPen(QPen(QColor(25, 113, 194, 220), 2.0));
        painter->setBrush(QColor(25, 113, 194, 45));
        painter->drawEllipse(m_activeSnapAnchorPoint, 8.0, 8.0);

        painter->setPen(Qt::NoPen);
        painter->setBrush(QColor(25, 113, 194));
        painter->drawEllipse(m_activeSnapAnchorPoint, 3.0, 3.0);

        painter->restore();
    }
}

CanvasScene::AnchorMatch CanvasScene::findNearestAnchor(const QPointF& scenePos, double maxDist, BaseItem* excludeItem) const
{
    AnchorMatch bestMatch;
    double bestDist = maxDist;

    for (auto* it : whiteboardItems()) {
        if (!it || it == excludeItem) continue;
        const auto anchors = it->anchorPointsScene();
        for (int i = 0; i < anchors.size(); ++i) {
            double dist = std::hypot(scenePos.x() - anchors[i].x(), scenePos.y() - anchors[i].y());
            if (dist < bestDist) {
                bestDist = dist;
                bestMatch = {it, i, anchors[i]};
            }
        }
    }
    return bestMatch;
}

void CanvasScene::updateConnectorsForItems(const QList<BaseItem*>& movedItems)
{
    if (movedItems.isEmpty()) return;

    QSet<QUuid> movedIds;
    for (auto* it : movedItems) {
        if (it) movedIds.insert(it->id());
    }

    for (auto* it : whiteboardItems()) {
        auto* line = dynamic_cast<LineItem*>(it);
        if (!line) continue;

        bool updated = false;
        QPointF p1 = line->p1();
        QPointF p2 = line->p2();

        if (line->startBinding().isValid() && movedIds.contains(line->startBinding().targetItemId)) {
            for (auto* target : whiteboardItems()) {
                if (target && target->id() == line->startBinding().targetItemId) {
                    const auto anchors = target->anchorPointsScene();
                    if (line->startBinding().anchorIndex >= 0 && line->startBinding().anchorIndex < anchors.size()) {
                        p1 = line->mapFromScene(anchors[line->startBinding().anchorIndex]);
                        updated = true;
                    }
                    break;
                }
            }
        }

        if (line->endBinding().isValid() && movedIds.contains(line->endBinding().targetItemId)) {
            for (auto* target : whiteboardItems()) {
                if (target && target->id() == line->endBinding().targetItemId) {
                    const auto anchors = target->anchorPointsScene();
                    if (line->endBinding().anchorIndex >= 0 && line->endBinding().anchorIndex < anchors.size()) {
                        p2 = line->mapFromScene(anchors[line->endBinding().anchorIndex]);
                        updated = true;
                    }
                    break;
                }
            }
        }

        if (updated) {
            line->setPoints(p1, p2);
        }
    }
}

void CanvasScene::groupSelectedItems()
{
    auto selected = selectedWhiteboardItems();
    if (selected.size() < 2) {
        return;
    }
    m_undoStack->push(new GroupCommand(this, selected, GroupCommand::Action::Group));
}

void CanvasScene::ungroupSelectedItems()
{
    auto selected = selectedWhiteboardItems();
    if (selected.isEmpty()) {
        return;
    }
    m_undoStack->push(new GroupCommand(this, selected, GroupCommand::Action::Ungroup));
}

void CanvasScene::toggleLockSelectedItems()
{
    auto selected = selectedWhiteboardItems();
    if (selected.isEmpty()) {
        return;
    }
    // Если хотя бы один элемент разблокирован, блокируем все выбранные. Иначе разблокируем.
    bool shouldLock = false;
    for (auto* it : selected) {
        if (!it->isLocked()) {
            shouldLock = true;
            break;
        }
    }
    m_undoStack->push(new ToggleLockCommand(this, selected, shouldLock));
}

void CanvasScene::alignLeft()
{
    auto selected = selectedWhiteboardItems();
    if (selected.size() < 2) return;

    double minLeft = std::numeric_limits<double>::max();
    for (auto* it : selected) {
        QRectF sceneBox = it->mapToScene(it->geometryRect()).boundingRect();
        minLeft = std::min(minLeft, sceneBox.left());
    }

    QList<ItemMoveRecord> records;
    for (auto* it : selected) {
        if (it->isLocked()) continue;
        QRectF sceneBox = it->mapToScene(it->geometryRect()).boundingRect();
        double deltaX = minLeft - sceneBox.left();
        if (std::abs(deltaX) > 0.01) {
            QPointF newPos = it->pos() + QPointF(deltaX, 0);
            records.append({it, it->pos(), newPos});
            it->setPos(newPos);
        }
    }
    if (!records.isEmpty()) {
        m_undoStack->push(new MoveItemCommand(records));
    }
}

void CanvasScene::alignCenterH()
{
    auto selected = selectedWhiteboardItems();
    if (selected.size() < 2) return;

    QRectF totalRect;
    for (auto* it : selected) {
        totalRect = totalRect.united(it->mapToScene(it->geometryRect()).boundingRect());
    }
    double targetCenter = totalRect.center().x();

    QList<ItemMoveRecord> records;
    for (auto* it : selected) {
        if (it->isLocked()) continue;
        QRectF sceneBox = it->mapToScene(it->geometryRect()).boundingRect();
        double deltaX = targetCenter - sceneBox.center().x();
        if (std::abs(deltaX) > 0.01) {
            QPointF newPos = it->pos() + QPointF(deltaX, 0);
            records.append({it, it->pos(), newPos});
            it->setPos(newPos);
        }
    }
    if (!records.isEmpty()) {
        m_undoStack->push(new MoveItemCommand(records));
    }
}

void CanvasScene::alignRight()
{
    auto selected = selectedWhiteboardItems();
    if (selected.size() < 2) return;

    double maxRight = std::numeric_limits<double>::lowest();
    for (auto* it : selected) {
        QRectF sceneBox = it->mapToScene(it->geometryRect()).boundingRect();
        maxRight = std::max(maxRight, sceneBox.right());
    }

    QList<ItemMoveRecord> records;
    for (auto* it : selected) {
        if (it->isLocked()) continue;
        QRectF sceneBox = it->mapToScene(it->geometryRect()).boundingRect();
        double deltaX = maxRight - sceneBox.right();
        if (std::abs(deltaX) > 0.01) {
            QPointF newPos = it->pos() + QPointF(deltaX, 0);
            records.append({it, it->pos(), newPos});
            it->setPos(newPos);
        }
    }
    if (!records.isEmpty()) {
        m_undoStack->push(new MoveItemCommand(records));
    }
}

void CanvasScene::alignTop()
{
    auto selected = selectedWhiteboardItems();
    if (selected.size() < 2) return;

    double minTop = std::numeric_limits<double>::max();
    for (auto* it : selected) {
        QRectF sceneBox = it->mapToScene(it->geometryRect()).boundingRect();
        minTop = std::min(minTop, sceneBox.top());
    }

    QList<ItemMoveRecord> records;
    for (auto* it : selected) {
        if (it->isLocked()) continue;
        QRectF sceneBox = it->mapToScene(it->geometryRect()).boundingRect();
        double deltaY = minTop - sceneBox.top();
        if (std::abs(deltaY) > 0.01) {
            QPointF newPos = it->pos() + QPointF(0, deltaY);
            records.append({it, it->pos(), newPos});
            it->setPos(newPos);
        }
    }
    if (!records.isEmpty()) {
        m_undoStack->push(new MoveItemCommand(records));
    }
}

void CanvasScene::alignCenterV()
{
    auto selected = selectedWhiteboardItems();
    if (selected.size() < 2) return;

    QRectF totalRect;
    for (auto* it : selected) {
        totalRect = totalRect.united(it->mapToScene(it->geometryRect()).boundingRect());
    }
    double targetCenter = totalRect.center().y();

    QList<ItemMoveRecord> records;
    for (auto* it : selected) {
        if (it->isLocked()) continue;
        QRectF sceneBox = it->mapToScene(it->geometryRect()).boundingRect();
        double deltaY = targetCenter - sceneBox.center().y();
        if (std::abs(deltaY) > 0.01) {
            QPointF newPos = it->pos() + QPointF(0, deltaY);
            records.append({it, it->pos(), newPos});
            it->setPos(newPos);
        }
    }
    if (!records.isEmpty()) {
        m_undoStack->push(new MoveItemCommand(records));
    }
}

void CanvasScene::alignBottom()
{
    auto selected = selectedWhiteboardItems();
    if (selected.size() < 2) return;

    double maxBottom = std::numeric_limits<double>::lowest();
    for (auto* it : selected) {
        QRectF sceneBox = it->mapToScene(it->geometryRect()).boundingRect();
        maxBottom = std::max(maxBottom, sceneBox.bottom());
    }

    QList<ItemMoveRecord> records;
    for (auto* it : selected) {
        if (it->isLocked()) continue;
        QRectF sceneBox = it->mapToScene(it->geometryRect()).boundingRect();
        double deltaY = maxBottom - sceneBox.bottom();
        if (std::abs(deltaY) > 0.01) {
            QPointF newPos = it->pos() + QPointF(0, deltaY);
            records.append({it, it->pos(), newPos});
            it->setPos(newPos);
        }
    }
    if (!records.isEmpty()) {
        m_undoStack->push(new MoveItemCommand(records));
    }
}

void CanvasScene::distributeHorizontally()
{
    auto selected = selectedWhiteboardItems();
    if (selected.size() < 3) return;

    struct ItemInfo {
        BaseItem* item;
        QRectF box;
    };
    QList<ItemInfo> list;
    for (auto* it : selected) {
        list.append({it, it->mapToScene(it->geometryRect()).boundingRect()});
    }

    std::sort(list.begin(), list.end(), [](const ItemInfo& a, const ItemInfo& b) {
        return a.box.left() < b.box.left();
    });

    double totalItemWidth = 0.0;
    for (const auto& info : list) {
        totalItemWidth += info.box.width();
    }

    double span = list.last().box.right() - list.first().box.left();
    double totalGap = span - totalItemWidth;
    double gap = totalGap / static_cast<double>(list.size() - 1);

    QList<ItemMoveRecord> records;
    double currentX = list.first().box.left();

    for (int i = 0; i < list.size(); ++i) {
        auto* it = list[i].item;
        if (!it->isLocked() && i > 0 && i < list.size() - 1) {
            double deltaX = currentX - list[i].box.left();
            if (std::abs(deltaX) > 0.01) {
                QPointF newPos = it->pos() + QPointF(deltaX, 0);
                records.append({it, it->pos(), newPos});
                it->setPos(newPos);
            }
        }
        currentX += list[i].box.width() + gap;
    }

    if (!records.isEmpty()) {
        m_undoStack->push(new MoveItemCommand(records));
    }
}

void CanvasScene::distributeVertically()
{
    auto selected = selectedWhiteboardItems();
    if (selected.size() < 3) return;

    struct ItemInfo {
        BaseItem* item;
        QRectF box;
    };
    QList<ItemInfo> list;
    for (auto* it : selected) {
        list.append({it, it->mapToScene(it->geometryRect()).boundingRect()});
    }

    std::sort(list.begin(), list.end(), [](const ItemInfo& a, const ItemInfo& b) {
        return a.box.top() < b.box.top();
    });

    double totalItemHeight = 0.0;
    for (const auto& info : list) {
        totalItemHeight += info.box.height();
    }

    double span = list.last().box.bottom() - list.first().box.top();
    double totalGap = span - totalItemHeight;
    double gap = totalGap / static_cast<double>(list.size() - 1);

    QList<ItemMoveRecord> records;
    double currentY = list.first().box.top();

    for (int i = 0; i < list.size(); ++i) {
        auto* it = list[i].item;
        if (!it->isLocked() && i > 0 && i < list.size() - 1) {
            double deltaY = currentY - list[i].box.top();
            if (std::abs(deltaY) > 0.01) {
                QPointF newPos = it->pos() + QPointF(0, deltaY);
                records.append({it, it->pos(), newPos});
                it->setPos(newPos);
            }
        }
        currentY += list[i].box.height() + gap;
    }

    if (!records.isEmpty()) {
        m_undoStack->push(new MoveItemCommand(records));
    }
}

} // namespace Whiteboard
