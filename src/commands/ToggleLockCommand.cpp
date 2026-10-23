#include "commands/ToggleLockCommand.h"
#include "items/BaseItem.h"
#include "canvas/CanvasScene.h"

namespace Whiteboard {

ToggleLockCommand::ToggleLockCommand(CanvasScene* scene, const QList<BaseItem*>& items, bool lock, QUndoCommand* parent)
    : QUndoCommand(parent)
    , m_scene(scene)
    , m_items(items)
    , m_newLockState(lock)
{
    setText(lock ? QStringLiteral("Блокировка") : QStringLiteral("Разблокировка"));

    for (auto* it : m_items) {
        if (it) {
            m_oldStates.insert(it, it->isLocked());
        }
    }
}

void ToggleLockCommand::redo()
{
    for (auto* it : m_items) {
        if (it) {
            it->setLocked(m_newLockState);
        }
    }
    if (m_scene) {
        m_scene->update();
    }
}

void ToggleLockCommand::undo()
{
    for (auto* it : m_items) {
        if (it && m_oldStates.contains(it)) {
            it->setLocked(m_oldStates.value(it));
        }
    }
    if (m_scene) {
        m_scene->update();
    }
}

} // namespace Whiteboard
