#include "commands/AddItemCommand.h"
#include "items/BaseItem.h"
#include <QGraphicsScene>

namespace Whiteboard {

AddItemCommand::AddItemCommand(QGraphicsScene* scene, BaseItem* item, QUndoCommand* parent)
    : QUndoCommand(parent)
    , m_scene(scene)
    , m_item(item)
{
    setText(QStringLiteral("Add Item"));
}

AddItemCommand::~AddItemCommand()
{
    if (m_item && m_item->scene() == nullptr) {
        delete m_item;
    }
}

void AddItemCommand::undo()
{
    if (m_scene && m_item && m_item->scene() == m_scene) {
        m_scene->removeItem(m_item);
    }
}

void AddItemCommand::redo()
{
    if (m_scene && m_item && m_item->scene() == nullptr) {
        m_scene->addItem(m_item);
    }
}

} // namespace Whiteboard
