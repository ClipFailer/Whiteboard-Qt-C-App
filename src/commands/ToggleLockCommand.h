#pragma once

#include <QUndoCommand>
#include <QList>
#include <QMap>

namespace Whiteboard {

class BaseItem;
class CanvasScene;

// Команда изменения состояния блокировки элементов
class ToggleLockCommand : public QUndoCommand {
public:
    ToggleLockCommand(CanvasScene* scene, const QList<BaseItem*>& items, bool lock, QUndoCommand* parent = nullptr);

    void undo() override;
    void redo() override;

private:
    CanvasScene* m_scene;
    QList<BaseItem*> m_items;
    bool m_newLockState;
    QMap<BaseItem*, bool> m_oldStates;
};

} // namespace Whiteboard
