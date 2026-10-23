#pragma once

#include "items/BaseItem.h"
#include <QObject>

namespace Whiteboard {

enum class Tool {
    Select,
    Hand,
    Rectangle,
    Ellipse,
    Diamond,
    Line,
    Arrow,
    Freehand,
    Text,
    StickyNote,
    Eraser
};

class ToolManager : public QObject {
    Q_OBJECT

public:
    explicit ToolManager(QObject* parent = nullptr);

    Tool currentTool() const { return m_currentTool; }
    void setTool(Tool tool);

    const ItemProperties& defaultProperties() const { return m_defaultProperties; }
    void setDefaultProperties(const ItemProperties& props);

    static QString toolName(Tool tool);

signals:
    void toolChanged(Tool tool);
    void defaultPropertiesChanged(const ItemProperties& props);

private:
    Tool m_currentTool{Tool::Select};
    ItemProperties m_defaultProperties;
};

} // namespace Whiteboard
