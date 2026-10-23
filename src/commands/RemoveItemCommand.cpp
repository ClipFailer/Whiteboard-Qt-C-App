#include "commands/RemoveItemCommand.h"
#include "items/BaseItem.h"
#include <QGraphicsScene>

namespace Whiteboard {

RemoveItemCommand::RemoveItemCommand(QGraphicsScene* scene, const QList<BaseItem*>& items, QUndoCommand* parent)
    : QUndoCommand(parent)
    , m_scene(scene)
    , m_items(items)
{
    setText(QStringLiteral("Delete Item(s)"));
}

RemoveItemCommand::RemoveItemCommand(QGraphicsScene* scene, BaseItem* item, QUndoCommand* parent)
    : QUndoCommand(parent)
    , m_scene(scene)
{
    if (item) {
        m_items.append(item);
    }
    setText(QStringLiteral("Delete Item"));
}

RemoveItemCommand::~RemoveItemCommand()
{
    for (auto* item : m_items) {
        if (item && item->scene() == nullptr) {
            delete item;
        }
    }
}

void RemoveItemCommand::undo()
{
    if (!m_scene) return;
    for (auto* item : m_items) {
        if (item && item->scene() == nullptr) {
            m_scene->addItem(item);
            item->setSelected(true);
        }
    }
}

void RemoveItemCommand::redo()
{
    if (!m_scene) return;
    for (auto* item : m_items) {
        if (item && item->scene() == m_scene) {
            m_scene->removeItem(item);
        }
    }
}

} // namespace Whiteboard
