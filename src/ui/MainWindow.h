#pragma once

#include "canvas/CanvasScene.h"
#include "canvas/CanvasView.h"
#include "tools/ToolManager.h"
#include "core/Document.h"
#include "core/AppSettings.h"
#include "ui/Toolbar.h"
#include "ui/PropertiesPanel.h"

#include <QMainWindow>
#include <QUndoStack>
#include <QLabel>

namespace Whiteboard {

class MainWindow : public QMainWindow {
    Q_OBJECT

public:
    explicit MainWindow(QWidget* parent = nullptr);
    ~MainWindow() override = default;

private slots:
    // Действия файла
    void newDocument();
    void openDocument();
    bool saveDocument();
    bool saveDocumentAs();
    void insertImage();
    void exportPng();
    void exportSvg();

    // Действия правки
    void deleteSelected();
    void duplicateSelected();
    void selectAll();
    void copyAsPng();
    void paste();

    // Действия вида
    void toggleDarkMode();
    void toggleGrid();
    void togglePropertiesPanel();

    // Обновления UI
    void updateZoomLabel(double factor);
    void updateSelectionStatus();
    void updateWindowTitle();

private:
    void setupUI();
    void createActions();
    void createMenus();
    void createStatusBar();
    void setupShortcuts();
    void applyTheme(bool isDark);
    bool maybeSave();

    AppSettings m_settings;
    Document m_document;
    ToolManager* m_toolManager{nullptr};
    QUndoStack* m_undoStack{nullptr};
    CanvasScene* m_scene{nullptr};
    CanvasView* m_view{nullptr};
    Toolbar* m_toolbar{nullptr};
    PropertiesPanel* m_propsPanel{nullptr};

    // Виджеты строки состояния
    QLabel* m_zoomStatusLabel{nullptr};
    QLabel* m_toolStatusLabel{nullptr};
    QLabel* m_selectionStatusLabel{nullptr};

    // Действия
    QAction* m_actNew{nullptr};
    QAction* m_actOpen{nullptr};
    QAction* m_actSave{nullptr};
    QAction* m_actSaveAs{nullptr};
    QAction* m_actInsertImage{nullptr};
    QAction* m_actExportPng{nullptr};
    QAction* m_actExportSvg{nullptr};
    QAction* m_actExit{nullptr};

    QAction* m_actUndo{nullptr};
    QAction* m_actRedo{nullptr};
    QAction* m_actDelete{nullptr};
    QAction* m_actDuplicate{nullptr};
    QAction* m_actCopyAsPng{nullptr};
    QAction* m_actPaste{nullptr};
    QAction* m_actSelectAll{nullptr};
    QAction* m_actGroup{nullptr};
    QAction* m_actUngroup{nullptr};
    QAction* m_actToggleLock{nullptr};

    QAction* m_actZoomIn{nullptr};
    QAction* m_actZoomOut{nullptr};
    QAction* m_actResetZoom{nullptr};
    QAction* m_actZoomToFit{nullptr};
    QAction* m_actDarkMode{nullptr};
    QAction* m_actGrid{nullptr};
    QAction* m_actSnapGrid{nullptr};
    QAction* m_actToggleProps{nullptr};
    QAction* m_actFullScreen{nullptr};

    QAction* m_actAbout{nullptr};
};

} // namespace Whiteboard
