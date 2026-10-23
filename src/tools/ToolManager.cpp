#include "tools/ToolManager.h"

namespace Whiteboard {

ToolManager::ToolManager(QObject* parent)
    : QObject(parent)
{
    m_defaultProperties.strokeColor = Qt::black;
    m_defaultProperties.fillColor = Qt::transparent;
    m_defaultProperties.strokeWidth = 2.0;
    m_defaultProperties.roughness = 1.5;
    m_defaultProperties.fillStyle = FillStyle::None;
    m_defaultProperties.strokeStyle = Qt::SolidLine;
    m_defaultProperties.opacity = 1.0;
}

void ToolManager::setTool(Tool tool)
{
    if (m_currentTool != tool) {
        m_currentTool = tool;
        emit toolChanged(m_currentTool);
    }
}

void ToolManager::setDefaultProperties(const ItemProperties& props)
{
    m_defaultProperties = props;
    emit defaultPropertiesChanged(m_defaultProperties);
}

QString ToolManager::toolName(Tool tool)
{
    switch (tool) {
    case Tool::Select: return QStringLiteral("Select (V)");
    case Tool::Hand: return QStringLiteral("Hand (H)");
    case Tool::Rectangle: return QStringLiteral("Rectangle (R)");
    case Tool::Ellipse: return QStringLiteral("Ellipse (E)");
    case Tool::Diamond: return QStringLiteral("Diamond (5)");
    case Tool::Line: return QStringLiteral("Line (L)");
    case Tool::Arrow: return QStringLiteral("Arrow (A)");
    case Tool::Freehand: return QStringLiteral("Pencil (P)");
    case Tool::Text: return QStringLiteral("Text (T)");
    case Tool::StickyNote: return QStringLiteral("Sticky Note (S)");
    case Tool::Eraser: return QStringLiteral("Eraser (0)");
    }
    return QStringLiteral("Select");
}

} // namespace Whiteboard
