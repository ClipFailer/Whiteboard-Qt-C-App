#pragma once

#include <QUndoCommand>
#include <QList>
#include <QMap>
#include <QUuid>

namespace Whiteboard {

class BaseItem;
class CanvasScene;

// Команда группировки и разгруппировки элементов на доске
class GroupCommand : public QUndoCommand {
public:
    enum class Action {
        Group,
        Ungroup
    };

    GroupCommand(CanvasScene* scene, const QList<BaseItem*>& items, Action action, QUndoCommand* parent = nullptr);

    void undo() override;
    void redo() override;

private:
    CanvasScene* m_scene;
    QList<BaseItem*> m_items;
    Action m_action;
    QUuid m_newGroupId;
    QMap<BaseItem*, QUuid> m_oldGroupIds;
};

} // namespace Whiteboard
