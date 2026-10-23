#include "core/Document.h"
#include "items/RectangleItem.h"
#include "items/EllipseItem.h"
#include "items/DiamondItem.h"
#include "items/LineItem.h"
#include "items/ArrowItem.h"
#include "items/FreehandItem.h"
#include "items/TextItem.h"
#include "items/ImageItem.h"
#include "items/StickyNoteItem.h"

#include <QFile>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <QImage>
#include <QPainter>
#include <QSvgGenerator>

namespace Whiteboard {

Document::Document() = default;

bool Document::saveToFile(const QString& filePath, const QList<BaseItem*>& items, const ViewportInfo& viewport)
{
    QJsonObject root;
    root[QStringLiteral("version")] = 1;
    root[QStringLiteral("appName")] = QStringLiteral("WhiteboardQt");

    QJsonObject vpObj;
    vpObj[QStringLiteral("scrollX")] = viewport.scrollX;
    vpObj[QStringLiteral("scrollY")] = viewport.scrollY;
    vpObj[QStringLiteral("zoom")] = viewport.zoom;
    root[QStringLiteral("viewport")] = vpObj;

    QJsonArray elementsArr;
    for (const auto* item : items) {
        if (item) {
            elementsArr.append(item->toJson());
        }
    }
    root[QStringLiteral("elements")] = elementsArr;

    QFile file(filePath);
    if (!file.open(QIODevice::WriteOnly | QIODevice::Truncate)) {
        return false;
    }

    QJsonDocument doc(root);
    file.write(doc.toJson(QJsonDocument::Indented));
    file.close();

    m_currentFilePath = filePath;
    m_modified = false;
    return true;
}

bool Document::loadFromFile(const QString& filePath, CanvasScene* scene, ViewportInfo& viewport)
{
    if (!scene) return false;

    QFile file(filePath);
    if (!file.open(QIODevice::ReadOnly)) {
        return false;
    }

    QByteArray data = file.readAll();
    file.close();

    QJsonParseError parseError;
    QJsonDocument doc = QJsonDocument::fromJson(data, &parseError);
    if (parseError.error != QJsonParseError::NoError || !doc.isObject()) {
        return false;
    }

    QJsonObject root = doc.object();

    if (root.contains(QStringLiteral("viewport"))) {
        QJsonObject vpObj = root[QStringLiteral("viewport")].toObject();
        viewport.scrollX = vpObj[QStringLiteral("scrollX")].toDouble(0.0);
        viewport.scrollY = vpObj[QStringLiteral("scrollY")].toDouble(0.0);
        viewport.zoom = vpObj[QStringLiteral("zoom")].toDouble(1.0);
    }

    // Очищаем существующие элементы доски
    for (auto* item : scene->whiteboardItems()) {
        scene->removeItem(item);
        delete item;
    }
    scene->undoStack()->clear();

    QJsonArray elementsArr = root[QStringLiteral("elements")].toArray();
    for (const auto& elemVal : elementsArr) {
        QJsonObject obj = elemVal.toObject();
        QString type = obj[QStringLiteral("type")].toString();

        std::unique_ptr<BaseItem> item;
        if (type == QStringLiteral("rectangle")) {
            item = std::make_unique<RectangleItem>();
        } else if (type == QStringLiteral("ellipse")) {
            item = std::make_unique<EllipseItem>();
        } else if (type == QStringLiteral("diamond")) {
            item = std::make_unique<DiamondItem>();
        } else if (type == QStringLiteral("line")) {
            item = std::make_unique<LineItem>();
        } else if (type == QStringLiteral("arrow")) {
            item = std::make_unique<ArrowItem>();
        } else if (type == QStringLiteral("freehand")) {
            item = std::make_unique<FreehandItem>();
        } else if (type == QStringLiteral("text")) {
            item = std::make_unique<TextItem>();
        } else if (type == QStringLiteral("sticky_note")) {
            item = std::make_unique<StickyNoteItem>();
        } else if (type == QStringLiteral("image")) {
            item = std::make_unique<ImageItem>();
        }

        if (item) {
            item->fromJson(obj);
            scene->addItem(item.release());
        }
    }

    m_currentFilePath = filePath;
    m_modified = false;
    return true;
}

QImage Document::renderToImage(CanvasScene* scene, bool selectedOnly, bool transparentBackground, double margin, double scale)
{
    if (!scene) return QImage();

    auto items = selectedOnly ? scene->selectedWhiteboardItems() : scene->whiteboardItems();
    if (items.isEmpty()) {
        return QImage();
    }

    QRectF totalRect;
    for (auto* it : items) {
        totalRect = totalRect.united(it->sceneBoundingRect());
    }
    QRectF exportRect = totalRect.adjusted(-margin, -margin, margin, margin);
    if (exportRect.width() <= 0 || exportRect.height() <= 0) {
        return QImage();
    }

    scale = std::clamp(scale, 1.0, 5.0);
    const QSize targetSize(static_cast<int>(std::round(exportRect.width() * scale)),
                           static_cast<int>(std::round(exportRect.height() * scale)));

    QImage image(targetSize, QImage::Format_ARGB32_Premultiplied);
    if (transparentBackground) {
        image.fill(Qt::transparent);
    } else {
        image.fill(Qt::white);
    }

    // Временно снимаем выделение, чтобы не рисовались маркеры
    auto selectedItems = scene->selectedWhiteboardItems();
    for (auto* it : selectedItems) {
        it->setSelected(false);
    }

    {
        QPainter painter(&image);
        painter.setRenderHint(QPainter::Antialiasing);
        painter.setRenderHint(QPainter::SmoothPixmapTransform);
        painter.setRenderHint(QPainter::TextAntialiasing);

        scene->render(&painter, QRectF(QPointF(0, 0), targetSize), exportRect);
    }

    // Восстанавливаем выделение
    for (auto* it : selectedItems) {
        it->setSelected(true);
    }

    return image;
}

bool Document::exportPng(const QString& filePath, CanvasScene* scene, bool selectedOnly, bool transparentBackground)
{
    QImage img = renderToImage(scene, selectedOnly, transparentBackground, 40.0);
    if (img.isNull()) {
        return false;
    }
    return img.save(filePath, "PNG");
}

bool Document::exportSvg(const QString& filePath, CanvasScene* scene, bool selectedOnly)
{
    if (!scene) return false;

    auto items = selectedOnly ? scene->selectedWhiteboardItems() : scene->whiteboardItems();
    if (items.isEmpty()) {
        return false;
    }

    QRectF totalRect;
    for (auto* it : items) {
        totalRect = totalRect.united(it->sceneBoundingRect());
    }
    constexpr double margin = 40.0;
    QRectF exportRect = totalRect.adjusted(-margin, -margin, margin, margin);

    QSvgGenerator generator;
    generator.setFileName(filePath);
    generator.setSize(exportRect.size().toSize());
    generator.setViewBox(QRect(0, 0, static_cast<int>(exportRect.width()), static_cast<int>(exportRect.height())));
    generator.setTitle(QStringLiteral("Whiteboard Drawing"));

    // Временно снимаем выделение
    auto selectedItems = scene->selectedWhiteboardItems();
    for (auto* it : selectedItems) {
        it->setSelected(false);
    }

    {
        QPainter painter(&generator);
        painter.setRenderHint(QPainter::Antialiasing);
        painter.setRenderHint(QPainter::SmoothPixmapTransform);
        painter.setRenderHint(QPainter::TextAntialiasing);

        scene->render(&painter, QRectF(QPointF(0, 0), exportRect.size()), exportRect);
    }

    // Восстанавливаем выделение
    for (auto* it : selectedItems) {
        it->setSelected(true);
    }

    return true;
}

} // namespace Whiteboard
