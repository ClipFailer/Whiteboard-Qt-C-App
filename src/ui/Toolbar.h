#pragma once

#include "tools/ToolManager.h"
#include <QWidget>
#include <QButtonGroup>
#include <QToolButton>
#include <QUndoStack>

namespace Whiteboard {

class Toolbar : public QWidget {
    Q_OBJECT

public:
    explicit Toolbar(ToolManager* toolManager, QUndoStack* undoStack, QWidget* parent = nullptr);

    void updateTheme(bool isDark);

private slots:
    void onToolButtonClicked(QAbstractButton* button);
    void onToolChanged(Tool tool);

private:
    void setupUI();
    void createToolButton(Tool tool, const QString& tooltip, const QString& shortcut, const QString& iconName);
    QIcon createIcon(const QString& name, const QColor& color);

    ToolManager* m_toolManager;
    QUndoStack* m_undoStack;
    QButtonGroup* m_buttonGroup;
    QMap<Tool, QToolButton*> m_toolButtons;
    QToolButton* m_undoBtn{nullptr};
    QToolButton* m_redoBtn{nullptr};
    bool m_isDark{false};
};

} // namespace Whiteboard
