#include "commands/ChangePropertyCommand.h"

namespace Whiteboard {

ChangePropertyCommand::ChangePropertyCommand(const QList<PropertyChangeRecord>& records, QUndoCommand* parent)
    : QUndoCommand(parent)
    , m_records(records)
{
    setText(QStringLiteral("Change Properties"));
}

ChangePropertyCommand::ChangePropertyCommand(BaseItem* item, const ItemProperties& oldProps,
                                             const ItemProperties& newProps, QUndoCommand* parent)
    : QUndoCommand(parent)
{
    m_records.append({item, oldProps, newProps});
    setText(QStringLiteral("Change Properties"));
}

void ChangePropertyCommand::undo()
{
    for (const auto& rec : m_records) {
        if (rec.item) {
            rec.item->setProperties(rec.oldProps);
        }
    }
}

void ChangePropertyCommand::redo()
{
    for (const auto& rec : m_records) {
        if (rec.item) {
            rec.item->setProperties(rec.newProps);
        }
    }
}

} // namespace Whiteboard
