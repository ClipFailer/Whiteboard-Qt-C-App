#include "canvas/CanvasView.h"
#include "items/TextItem.h"
#include "items/StickyNoteItem.h"
#include "items/ImageItem.h"
#include "commands/AddItemCommand.h"
#include <QWheelEvent>
#include <QScrollBar>
#include <QKeyEvent>
#include <QDragEnterEvent>
#include <QDropEvent>
#include <QMimeData>
#include <QFileInfo>
#include <cmath>
#include <algorithm>

namespace Whiteboard {

namespace {
constexpr double MinZoom = 0.1;
constexpr double MaxZoom = 10.0;
constexpr double GridSpacing = 20.0;
} // namespace

CanvasView::CanvasView(CanvasScene* scene, QWidget* parent)
    : QGraphicsView(scene, parent)
    , m_scene(scene)
{
    setRenderHint(QPainter::Antialiasing);
    setRenderHint(QPainter::SmoothPixmapTransform);
    setRenderHint(QPainter::TextAntialiasing);

    // FullViewportUpdate: перерисовывает весь вьюпорт при каждом изменении.
    // Это правильный режим для холста рисования — режимы частичного обновления
    // (Minimal / BoundingRect / Smart) оставляют «призраки» при перемещении объектов,
    // потому что Qt не отслеживает *предыдущий* ограничивающий прямоугольник перемещённых элементов.
    // Влияние на производительность пренебрежимо мало при типичных размерах холста.
    setViewportUpdateMode(QGraphicsView::FullViewportUpdate);
    setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    setTransformationAnchor(QGraphicsView::AnchorUnderMouse);
    setDragMode(QGraphicsView::NoDrag);
    setFrameShape(QFrame::NoFrame);
    setAcceptDrops(true);

    // Центрируем вид на начале координат сцены
    centerOn(0, 0);

    // Встроенный текстовый редактор
    m_textEditor = new QTextEdit(this);
    m_textEditor->hide();
    m_textEditor->setStyleSheet(QStringLiteral(
        "QTextEdit { background: transparent; border: 1.5px dashed #1971c2; padding: 2px; }"));
    m_textEditor->setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    m_textEditor->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);

    connect(m_scene, &CanvasScene::editTextRequested, this, &CanvasView::startTextEditing);
    connect(m_scene, &CanvasScene::editStickyNoteRequested, this, &CanvasView::startStickyNoteEditing);
    connect(m_scene, &CanvasScene::createTextRequested, this, &CanvasView::createTextAt);
}

void CanvasView::setZoom(double factor)
{
    factor = std::clamp(factor, MinZoom, MaxZoom);
    double currentScale = transform().m11();
    if (std::abs(currentScale - factor) > 0.001) {
        double scaleChange = factor / currentScale;
        scale(scaleChange, scaleChange);
        m_zoomFactor = transform().m11();
        emit zoomChanged(m_zoomFactor);
    }
}

void CanvasView::zoomIn()
{
    setZoom(m_zoomFactor * 1.15);
}

void CanvasView::zoomOut()
{
    setZoom(m_zoomFactor / 1.15);
}

void CanvasView::resetZoom()
{
    setZoom(1.0);
}

void CanvasView::zoomToFit()
{
    auto items = m_scene->whiteboardItems();
    if (items.isEmpty()) {
        resetZoom();
        centerOn(0, 0);
        return;
    }

    QRectF totalRect;
    for (auto* it : items) {
        totalRect = totalRect.united(it->sceneBoundingRect());
    }
    totalRect.adjust(-40, -40, 40, 40);
    fitInView(totalRect, Qt::KeepAspectRatio);
    m_zoomFactor = transform().m11();
    emit zoomChanged(m_zoomFactor);
}

void CanvasView::setGridVisible(bool visible)
{
    m_gridVisible = visible;
    viewport()->update();
}

void CanvasView::setDarkMode(bool dark)
{
    m_darkMode = dark;
    viewport()->update();
}

void CanvasView::wheelEvent(QWheelEvent* event)
{
    if (m_editingItem) {
        finishTextEditing();
    }

    // Масштабирование колесом мыши (плавный зум)
    double numDegrees = event->angleDelta().y();
    if (numDegrees != 0) {
        double factor = (numDegrees > 0) ? 1.12 : (1.0 / 1.12);
        double targetZoom = std::clamp(m_zoomFactor * factor, MinZoom, MaxZoom);
        double actualFactor = targetZoom / m_zoomFactor;
        scale(actualFactor, actualFactor);
        m_zoomFactor = transform().m11();
        emit zoomChanged(m_zoomFactor);
        event->accept();
        return;
    }

    QGraphicsView::wheelEvent(event);
}

void CanvasView::keyPressEvent(QKeyEvent* event)
{
    if (event->key() == Qt::Key_Space && !event->isAutoRepeat()) {
        m_spacePressed = true;
        setCursor(Qt::OpenHandCursor);
        event->accept();
        return;
    }

    if (m_editingItem) {
        if (event->key() == Qt::Key_Escape) {
            finishTextEditing();
            event->accept();
            return;
        }
    }

    QGraphicsView::keyPressEvent(event);
}

void CanvasView::keyReleaseEvent(QKeyEvent* event)
{
    if (event->key() == Qt::Key_Space && !event->isAutoRepeat()) {
        m_spacePressed = false;
        if (!m_isPanning) {
            setCursor(Qt::ArrowCursor);
        }
        event->accept();
        return;
    }
    QGraphicsView::keyReleaseEvent(event);
}

void CanvasView::mousePressEvent(QMouseEvent* event)
{
    if (m_editingItem && !m_textEditor->geometry().contains(event->pos())) {
        finishTextEditing();
    }

    const bool isHandTool = (m_scene->toolManager()->currentTool() == Tool::Hand);
    const bool isPanTrigger = (event->button() == Qt::MiddleButton) ||
                              (event->button() == Qt::LeftButton && (m_spacePressed || isHandTool));

    if (isPanTrigger) {
        m_isPanning = true;
        m_lastPanPoint = event->pos();
        setCursor(Qt::ClosedHandCursor);
        event->accept();
        return;
    }

    QGraphicsView::mousePressEvent(event);
}

void CanvasView::mouseMoveEvent(QMouseEvent* event)
{
    if (m_isPanning) {
        QPoint delta = event->pos() - m_lastPanPoint;
        m_lastPanPoint = event->pos();

        horizontalScrollBar()->setValue(horizontalScrollBar()->value() - delta.x());
        verticalScrollBar()->setValue(verticalScrollBar()->value() - delta.y());
        event->accept();
        return;
    }

    QGraphicsView::mouseMoveEvent(event);
}

void CanvasView::mouseReleaseEvent(QMouseEvent* event)
{
    if (m_isPanning) {
        m_isPanning = false;
        if (m_spacePressed || m_scene->toolManager()->currentTool() == Tool::Hand) {
            setCursor(Qt::OpenHandCursor);
        } else {
            setCursor(Qt::ArrowCursor);
        }
        event->accept();
        return;
    }

    QGraphicsView::mouseReleaseEvent(event);
}

void CanvasView::drawBackground(QPainter* painter, const QRectF& rect)
{
    // Заливка фона
    const QColor bgColor = m_darkMode ? QColor(18, 18, 18) : QColor(255, 255, 255);
    painter->fillRect(rect, bgColor);

    if (!m_gridVisible) {
        return;
    }

    // Точечная сетка
    const QColor dotColor = m_darkMode ? QColor(50, 50, 50) : QColor(215, 215, 215);
    painter->setPen(dotColor);

    const double left = std::floor(rect.left() / GridSpacing) * GridSpacing;
    const double top = std::floor(rect.top() / GridSpacing) * GridSpacing;
    const double right = rect.right();
    const double bottom = rect.bottom();

    for (double x = left; x <= right; x += GridSpacing) {
        for (double y = top; y <= bottom; y += GridSpacing) {
            painter->drawPoint(QPointF(x, y));
        }
    }
}

void CanvasView::resizeEvent(QResizeEvent* event)
{
    QGraphicsView::resizeEvent(event);
    if (m_editingItem) {
        updateEditorGeometry();
    }
}

void CanvasView::startTextEditing(TextItem* item)
{
    if (!item) return;

    if (m_editingStickyNote) {
        finishTextEditing();
    }

    m_editingItem = item;
    m_editingStickyNote = nullptr;
    m_isCreatingNewText = false;
    m_editingItem->setEditing(true);

    m_textEditor->setFont(item->font());
    m_textEditor->setTextColor(item->properties().strokeColor);
    m_textEditor->setPlainText(item->text());
    m_textEditor->selectAll();

    updateEditorGeometry();
    m_textEditor->show();
    m_textEditor->setFocus();
}

void CanvasView::startStickyNoteEditing(StickyNoteItem* note)
{
    if (!note) return;

    if (m_editingItem) {
        finishTextEditing();
    }

    m_editingStickyNote = note;
    m_editingItem = nullptr;
    m_isCreatingNewText = false;
    m_editingStickyNote->setEditing(true);

    m_textEditor->setFont(note->font());
    m_textEditor->setTextColor(note->properties().strokeColor);
    m_textEditor->setPlainText(note->text());
    m_textEditor->selectAll();

    updateEditorGeometry();
    m_textEditor->show();
    m_textEditor->setFocus();
}

void CanvasView::createTextAt(const QPointF& scenePos)
{
    m_newTextScenePos = scenePos;
    m_isCreatingNewText = true;

    auto* textItem = new TextItem(QStringLiteral(""), scenePos);
    textItem->setProperties(m_scene->toolManager()->defaultProperties());
    m_editingItem = textItem;
    m_editingStickyNote = nullptr;
    m_editingItem->setEditing(true);

    m_textEditor->setFont(textItem->font());
    m_textEditor->setTextColor(textItem->properties().strokeColor);
    m_textEditor->setPlainText(QString());

    updateEditorGeometry();
    m_textEditor->show();
    m_textEditor->setFocus();
}

void CanvasView::updateEditorGeometry()
{
    if (m_editingStickyNote) {
        QPoint viewPos = mapFromScene(m_editingStickyNote->pos());
        QRectF r = m_editingStickyNote->geometryRect().adjusted(14, 14, -20, -28);
        int w = std::max(60, static_cast<int>(r.width() * m_zoomFactor));
        int h = std::max(40, static_cast<int>(r.height() * m_zoomFactor));
        m_textEditor->setGeometry(viewPos.x() + static_cast<int>(14 * m_zoomFactor),
                                  viewPos.y() + static_cast<int>(14 * m_zoomFactor), w, h);
        return;
    }

    if (!m_editingItem) return;

    QPoint viewPos = mapFromScene(m_editingItem->pos());
    QRectF itemRect = m_editingItem->geometryRect();
    int w = std::max(120, static_cast<int>(itemRect.width() * m_zoomFactor) + 20);
    int h = std::max(40, static_cast<int>(itemRect.height() * m_zoomFactor) + 20);
    m_textEditor->setGeometry(viewPos.x(), viewPos.y(), w, h);
}

void CanvasView::finishTextEditing()
{
    if (m_editingStickyNote) {
        QString text = m_textEditor->toPlainText().trimmed();
        m_editingStickyNote->setText(text);
        m_editingStickyNote->setEditing(false);
        m_editingStickyNote = nullptr;
        m_textEditor->hide();
        return;
    }

    if (!m_editingItem) return;

    QString text = m_textEditor->toPlainText().trimmed();
    m_textEditor->hide();

    if (m_isCreatingNewText) {
        if (!text.isEmpty()) {
            m_editingItem->setText(text);
            m_editingItem->setEditing(false);
            m_scene->undoStack()->push(new AddItemCommand(m_scene, m_editingItem));
            m_scene->clearSelection();
            m_editingItem->setSelected(true);
        } else {
            delete m_editingItem;
        }
    } else {
        if (!text.isEmpty()) {
            m_editingItem->setText(text);
        }
        m_editingItem->setEditing(false);
    }

    m_editingItem = nullptr;
    m_isCreatingNewText = false;
}

void CanvasView::dragEnterEvent(QDragEnterEvent* event)
{
    const QMimeData* mime = event->mimeData();
    if (mime && (mime->hasImage() || mime->hasUrls())) {
        event->acceptProposedAction();
        return;
    }
    QGraphicsView::dragEnterEvent(event);
}

void CanvasView::dragMoveEvent(QDragMoveEvent* event)
{
    const QMimeData* mime = event->mimeData();
    if (mime && (mime->hasImage() || mime->hasUrls())) {
        event->acceptProposedAction();
        return;
    }
    QGraphicsView::dragMoveEvent(event);
}

void CanvasView::dropEvent(QDropEvent* event)
{
    const QMimeData* mime = event->mimeData();
    if (!mime) {
        QGraphicsView::dropEvent(event);
        return;
    }

    QPointF sceneDropPos = mapToScene(event->position().toPoint());

    // 1. Если данные содержат прямое изображение
    if (mime->hasImage()) {
        QImage img = qvariant_cast<QImage>(mime->imageData());
        if (!img.isNull()) {
            auto* imageItem = new ImageItem(img);
            imageItem->setPos(sceneDropPos - QPointF(imageItem->geometryRect().width() / 2,
                                                    imageItem->geometryRect().height() / 2));
            m_scene->clearSelection();
            m_scene->undoStack()->push(new AddItemCommand(m_scene, imageItem));
            imageItem->setSelected(true);
            event->acceptProposedAction();
            return;
        }
    }

    // 2. Если перетащили файлы (URL)
    if (mime->hasUrls()) {
        bool handled = false;
        QPointF curPos = sceneDropPos;
        for (const QUrl& url : mime->urls()) {
            if (url.isLocalFile()) {
                QString filePath = url.toLocalFile();
                QFileInfo fi(filePath);
                QString ext = fi.suffix().toLower();
                if (ext == QStringLiteral("png")  || ext == QStringLiteral("jpg") ||
                    ext == QStringLiteral("jpeg") || ext == QStringLiteral("webp") ||
                    ext == QStringLiteral("bmp")  || ext == QStringLiteral("gif")) {
                    QImage img(filePath);
                    if (!img.isNull()) {
                        auto* imageItem = new ImageItem(img);
                        imageItem->setPos(curPos);
                        m_scene->clearSelection();
                        m_scene->undoStack()->push(new AddItemCommand(m_scene, imageItem));
                        imageItem->setSelected(true);
                        curPos += QPointF(30, 30);
                        handled = true;
                    }
                }
            }
        }
        if (handled) {
            event->acceptProposedAction();
            return;
        }
    }

    QGraphicsView::dropEvent(event);
}

} // namespace Whiteboard
