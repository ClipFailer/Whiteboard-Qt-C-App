#include "commands/GroupCommand.h"
#include "items/BaseItem.h"
#include "canvas/CanvasScene.h"

namespace Whiteboard {

GroupCommand::GroupCommand(CanvasScene* scene, const QList<BaseItem*>& items, Action action, QUndoCommand* parent)
    : QUndoCommand(parent)
    , m_scene(scene)
    , m_items(items)
    , m_action(action)
    , m_newGroupId(action == Action::Group ? QUuid::createUuid() : QUuid())
{
    if (m_action == Action::Group) {
        setText(QStringLiteral("Группировка"));
    } else {
        setText(QStringLiteral("Разгруппировка"));
    }

    for (auto* it : m_items) {
        if (it) {
            m_oldGroupIds.insert(it, it->groupId());
        }
    }
}

void GroupCommand::redo()
{
    for (auto* it : m_items) {
        if (it) {
            it->setGroupId(m_newGroupId);
        }
    }
    if (m_scene) {
        m_scene->update();
    }
}

void GroupCommand::undo()
{
    for (auto* it : m_items) {
        if (it && m_oldGroupIds.contains(it)) {
            it->setGroupId(m_oldGroupIds.value(it));
        }
    }
    if (m_scene) {
        m_scene->update();
    }
}

} // namespace Whiteboard
