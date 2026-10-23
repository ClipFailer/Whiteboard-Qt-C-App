#pragma once

#include "items/BaseItem.h"
#include <QUndoCommand>
#include <QList>

namespace Whiteboard {

struct PropertyChangeRecord {
    BaseItem* item;
    ItemProperties oldProps;
    ItemProperties newProps;
};

class ChangePropertyCommand : public QUndoCommand {
public:
    ChangePropertyCommand(const QList<PropertyChangeRecord>& records, QUndoCommand* parent = nullptr);
    ChangePropertyCommand(BaseItem* item, const ItemProperties& oldProps, const ItemProperties& newProps,
                          QUndoCommand* parent = nullptr);

    void undo() override;
    void redo() override;

private:
    QList<PropertyChangeRecord> m_records;
};

} // namespace Whiteboard
