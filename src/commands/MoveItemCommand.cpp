#include "commands/MoveItemCommand.h"
#include "items/BaseItem.h"
#include "canvas/CanvasScene.h"

namespace Whiteboard {

MoveItemCommand::MoveItemCommand(const QList<ItemMoveRecord>& records, QUndoCommand* parent)
    : QUndoCommand(parent)
    , m_records(records)
{
    setText(QStringLiteral("Move Item(s)"));
}

MoveItemCommand::MoveItemCommand(BaseItem* item, const QPointF& oldPos, const QPointF& newPos, QUndoCommand* parent)
    : QUndoCommand(parent)
{
    m_records.append({item, oldPos, newPos});
    setText(QStringLiteral("Move Item"));
}

void MoveItemCommand::undo()
{
    QList<BaseItem*> moved;
    for (const auto& rec : m_records) {
        if (rec.item) {
            rec.item->setPos(rec.oldPos);
            moved.append(rec.item);
        }
    }
    if (!moved.isEmpty() && moved.first()->scene()) {
        if (auto* sc = dynamic_cast<CanvasScene*>(moved.first()->scene())) {
            sc->updateConnectorsForItems(moved);
        }
    }
}

void MoveItemCommand::redo()
{
    QList<BaseItem*> moved;
    for (const auto& rec : m_records) {
        if (rec.item) {
            rec.item->setPos(rec.newPos);
            moved.append(rec.item);
        }
    }
    if (!moved.isEmpty() && moved.first()->scene()) {
        if (auto* sc = dynamic_cast<CanvasScene*>(moved.first()->scene())) {
            sc->updateConnectorsForItems(moved);
        }
    }
}

} // namespace Whiteboard
