#pragma once

#include "canvas/CanvasScene.h"
#include <QString>
#include <QPointF>
#include <QImage>

namespace Whiteboard {

struct ViewportInfo {
    double scrollX{0.0};
    double scrollY{0.0};
    double zoom{1.0};
};

class Document {
public:
    Document();

    QString currentFilePath() const { return m_currentFilePath; }
    void setCurrentFilePath(const QString& path) { m_currentFilePath = path; }

    bool isModified() const { return m_modified; }
    void setModified(bool mod) { m_modified = mod; }

    bool saveToFile(const QString& filePath, const QList<BaseItem*>& items, const ViewportInfo& viewport);
    bool loadFromFile(const QString& filePath, CanvasScene* scene, ViewportInfo& viewport);

    // Рендерит элементы сцены в изображение QImage (scaleFactor 2.0x для высокого разрешения / HiDPI)
    static QImage renderToImage(CanvasScene* scene, bool selectedOnly = false,
                                bool transparentBackground = false, double margin = 40.0, double scale = 2.0);

    static bool exportPng(const QString& filePath, CanvasScene* scene, bool selectedOnly = false,
                          bool transparentBackground = false);
    static bool exportSvg(const QString& filePath, CanvasScene* scene, bool selectedOnly = false);

private:
    QString m_currentFilePath;
    bool m_modified{false};
};

} // namespace Whiteboard
