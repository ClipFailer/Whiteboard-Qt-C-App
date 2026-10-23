#pragma once

#include <QUndoCommand>
#include <QPointer>

class QGraphicsScene;

namespace Whiteboard {

class BaseItem;

class AddItemCommand : public QUndoCommand {
public:
    AddItemCommand(QGraphicsScene* scene, BaseItem* item, QUndoCommand* parent = nullptr);
    ~AddItemCommand() override;

    void undo() override;
    void redo() override;

private:
    QGraphicsScene* m_scene;
    BaseItem* m_item;
};

} // namespace Whiteboard
