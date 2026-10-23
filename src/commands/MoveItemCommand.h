#pragma once

#include <QUndoCommand>
#include <QList>
#include <QPointF>

namespace Whiteboard {

class BaseItem;

struct ItemMoveRecord {
    BaseItem* item;
    QPointF oldPos;
    QPointF newPos;
};

class MoveItemCommand : public QUndoCommand {
public:
    MoveItemCommand(const QList<ItemMoveRecord>& records, QUndoCommand* parent = nullptr);
    MoveItemCommand(BaseItem* item, const QPointF& oldPos, const QPointF& newPos, QUndoCommand* parent = nullptr);

    void undo() override;
    void redo() override;

private:
    QList<ItemMoveRecord> m_records;
};

} // namespace Whiteboard
