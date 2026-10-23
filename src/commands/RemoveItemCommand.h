#pragma once

#include <QUndoCommand>
#include <QList>

class QGraphicsScene;

namespace Whiteboard {

class BaseItem;

class RemoveItemCommand : public QUndoCommand {
public:
    RemoveItemCommand(QGraphicsScene* scene, const QList<BaseItem*>& items, QUndoCommand* parent = nullptr);
    RemoveItemCommand(QGraphicsScene* scene, BaseItem* item, QUndoCommand* parent = nullptr);
    ~RemoveItemCommand() override;

    void undo() override;
    void redo() override;

private:
    QGraphicsScene* m_scene;
    QList<BaseItem*> m_items;
};

} // namespace Whiteboard
