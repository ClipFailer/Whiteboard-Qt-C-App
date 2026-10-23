#include "ui/MainWindow.h"
#include "items/ImageItem.h"
#include "items/TextItem.h"
#include "commands/AddItemCommand.h"
#include <QMenuBar>
#include <QStatusBar>
#include <QFileDialog>
#include <QMessageBox>
#include <QHBoxLayout>
#include <QShortcut>
#include <QKeySequence>
#include <QCloseEvent>
#include <QApplication>
#include <QClipboard>
#include <QMimeData>
#include <QPalette>

namespace Whiteboard {

MainWindow::MainWindow(QWidget* parent)
    : QMainWindow(parent)
    , m_toolManager(new ToolManager(this))
    , m_undoStack(new QUndoStack(this))
{
    setupUI();
    createActions();
    createMenus();
    createStatusBar();
    setupShortcuts();

    // Устанавливаем подходящий для темы цвет обводки по умолчанию до вызова applyTheme, чтобы
    // панель свойств отображала правильный образец цвета при первом запуске.
    {
        ItemProperties props = m_toolManager->defaultProperties();
        props.strokeColor = m_settings.isDarkMode()
                            ? QColor(0xe0, 0xe0, 0xe0)
                            : QColor(0x1e, 0x1e, 0x1e);
        m_toolManager->setDefaultProperties(props);
    }

    applyTheme(m_settings.isDarkMode());
    m_view->setGridVisible(m_settings.isGridVisible());
    updateWindowTitle();

    resize(1280, 800);
}

void MainWindow::setupUI()
{
    auto* centralWidget = new QWidget(this);
    auto* layout = new QHBoxLayout(centralWidget);
    layout->setContentsMargins(6, 6, 6, 6);
    layout->setSpacing(6);

    m_scene = new CanvasScene(m_toolManager, m_undoStack, this);
    m_view = new CanvasView(m_scene, centralWidget);

    m_toolbar = new Toolbar(m_toolManager, m_undoStack, centralWidget);
    m_propsPanel = new PropertiesPanel(m_toolManager, m_scene, centralWidget);

    layout->addWidget(m_toolbar, 0, Qt::AlignTop);
    layout->addWidget(m_view, 1);
    layout->addWidget(m_propsPanel, 0);

    setCentralWidget(centralWidget);

    connect(m_view, &CanvasView::zoomChanged, this, &MainWindow::updateZoomLabel);
    connect(m_scene, &QGraphicsScene::selectionChanged, this, &MainWindow::updateSelectionStatus);
    connect(m_toolManager, &ToolManager::toolChanged, this, [this](Tool tool) {
        m_toolStatusLabel->setText(ToolManager::toolName(tool));
    });
    connect(m_undoStack, &QUndoStack::cleanChanged, this, [this](bool clean) {
        m_document.setModified(!clean);
        updateWindowTitle();
    });
}

void MainWindow::createActions()
{
    // Файл
    m_actNew = new QAction(QStringLiteral("&New"), this);
    m_actNew->setShortcut(QKeySequence::New);
    connect(m_actNew, &QAction::triggered, this, &MainWindow::newDocument);

    m_actOpen = new QAction(QStringLiteral("&Open..."), this);
    m_actOpen->setShortcut(QKeySequence::Open);
    connect(m_actOpen, &QAction::triggered, this, &MainWindow::openDocument);

    m_actSave = new QAction(QStringLiteral("&Save"), this);
    m_actSave->setShortcut(QKeySequence::Save);
    connect(m_actSave, &QAction::triggered, this, &MainWindow::saveDocument);

    m_actSaveAs = new QAction(QStringLiteral("Save &As..."), this);
    m_actSaveAs->setShortcut(QKeySequence::SaveAs);
    connect(m_actSaveAs, &QAction::triggered, this, &MainWindow::saveDocumentAs);

    m_actInsertImage = new QAction(QStringLiteral("&Insert Image..."), this);
    m_actInsertImage->setShortcut(QKeySequence(QStringLiteral("Ctrl+I")));
    connect(m_actInsertImage, &QAction::triggered, this, &MainWindow::insertImage);

    m_actExportPng = new QAction(QStringLiteral("Export &PNG..."), this);
    m_actExportPng->setShortcut(QKeySequence(QStringLiteral("Ctrl+E")));
    connect(m_actExportPng, &QAction::triggered, this, &MainWindow::exportPng);

    m_actExportSvg = new QAction(QStringLiteral("Export &SVG..."), this);
    connect(m_actExportSvg, &QAction::triggered, this, &MainWindow::exportSvg);

    m_actExit = new QAction(QStringLiteral("E&xit"), this);
    m_actExit->setShortcut(QKeySequence::Quit);
    connect(m_actExit, &QAction::triggered, this, &QWidget::close);

    // Правка
    m_actUndo = m_undoStack->createUndoAction(this, QStringLiteral("&Undo"));
    m_actUndo->setShortcuts({QKeySequence::Undo, QKeySequence(QStringLiteral("Ctrl+Z"))});

    m_actRedo = m_undoStack->createRedoAction(this, QStringLiteral("&Redo"));
    m_actRedo->setShortcuts({QKeySequence::Redo, QKeySequence(QStringLiteral("Ctrl+Y")), QKeySequence(QStringLiteral("Ctrl+Shift+Z"))});

    m_actPaste = new QAction(QStringLiteral("&Paste"), this);
    m_actPaste->setShortcut(QKeySequence::Paste);
    connect(m_actPaste, &QAction::triggered, this, &MainWindow::paste);

    m_actDuplicate = new QAction(QStringLiteral("Du&plicate"), this);
    m_actDuplicate->setShortcut(QKeySequence(QStringLiteral("Ctrl+D")));
    connect(m_actDuplicate, &QAction::triggered, this, &MainWindow::duplicateSelected);

    m_actCopyAsPng = new QAction(QStringLiteral("Copy to Clipboard as &PNG"), this);
    m_actCopyAsPng->setShortcut(QKeySequence(QStringLiteral("Ctrl+Shift+C")));
    connect(m_actCopyAsPng, &QAction::triggered, this, &MainWindow::copyAsPng);

    m_actDelete = new QAction(QStringLiteral("&Delete"), this);
    m_actDelete->setShortcuts({QKeySequence::Delete, QKeySequence(Qt::Key_Backspace)});
    connect(m_actDelete, &QAction::triggered, this, &MainWindow::deleteSelected);

    m_actSelectAll = new QAction(QStringLiteral("Select &All"), this);
    m_actSelectAll->setShortcut(QKeySequence::SelectAll);
    connect(m_actSelectAll, &QAction::triggered, this, &MainWindow::selectAll);

    m_actGroup = new QAction(QStringLiteral("&Group"), this);
    m_actGroup->setShortcut(QKeySequence(QStringLiteral("Ctrl+G")));
    connect(m_actGroup, &QAction::triggered, m_scene, &CanvasScene::groupSelectedItems);

    m_actUngroup = new QAction(QStringLiteral("&Ungroup"), this);
    m_actUngroup->setShortcut(QKeySequence(QStringLiteral("Ctrl+Shift+G")));
    connect(m_actUngroup, &QAction::triggered, m_scene, &CanvasScene::ungroupSelectedItems);

    m_actToggleLock = new QAction(QStringLiteral("&Lock / Unlock"), this);
    m_actToggleLock->setShortcut(QKeySequence(QStringLiteral("Ctrl+L")));
    connect(m_actToggleLock, &QAction::triggered, m_scene, &CanvasScene::toggleLockSelectedItems);

    // Вид
    m_actZoomIn = new QAction(QStringLiteral("Zoom &In"), this);
    m_actZoomIn->setShortcuts({QKeySequence::ZoomIn, QKeySequence(QStringLiteral("Ctrl+=")), QKeySequence(QStringLiteral("Ctrl++"))});
    connect(m_actZoomIn, &QAction::triggered, m_view, &CanvasView::zoomIn);

    m_actZoomOut = new QAction(QStringLiteral("Zoom &Out"), this);
    m_actZoomOut->setShortcuts({QKeySequence::ZoomOut, QKeySequence(QStringLiteral("Ctrl+-"))});
    connect(m_actZoomOut, &QAction::triggered, m_view, &CanvasView::zoomOut);

    m_actResetZoom = new QAction(QStringLiteral("&Reset Zoom (100%)"), this);
    m_actResetZoom->setShortcut(QKeySequence(QStringLiteral("Ctrl+0")));
    connect(m_actResetZoom, &QAction::triggered, m_view, &CanvasView::resetZoom);

    m_actZoomToFit = new QAction(QStringLiteral("Zoom to &Fit"), this);
    m_actZoomToFit->setShortcut(QKeySequence(QStringLiteral("Shift+1")));
    connect(m_actZoomToFit, &QAction::triggered, m_view, &CanvasView::zoomToFit);

    m_actDarkMode = new QAction(QStringLiteral("&Dark Mode"), this);
    m_actDarkMode->setCheckable(true);
    m_actDarkMode->setChecked(m_settings.isDarkMode());
    connect(m_actDarkMode, &QAction::triggered, this, &MainWindow::toggleDarkMode);

    m_actGrid = new QAction(QStringLiteral("Show &Grid"), this);
    m_actGrid->setCheckable(true);
    m_actGrid->setChecked(m_settings.isGridVisible());
    connect(m_actGrid, &QAction::triggered, this, &MainWindow::toggleGrid);

    m_actSnapGrid = new QAction(QStringLiteral("Snap to &Grid"), this);
    m_actSnapGrid->setCheckable(true);
    m_actSnapGrid->setChecked(m_scene->isSnapToGrid());
    connect(m_actSnapGrid, &QAction::toggled, m_scene, &CanvasScene::setSnapToGrid);

    m_actToggleProps = new QAction(QStringLiteral("Toggle &Properties Panel"), this);
    connect(m_actToggleProps, &QAction::triggered, this, &MainWindow::togglePropertiesPanel);

    m_actFullScreen = new QAction(QStringLiteral("&Fullscreen"), this);
    m_actFullScreen->setShortcut(QKeySequence(Qt::Key_F11));
    connect(m_actFullScreen, &QAction::triggered, this, [this]() {
        if (isFullScreen()) {
            showNormal();
        } else {
            showFullScreen();
        }
    });

    // Справка
    m_actAbout = new QAction(QStringLiteral("&About Whiteboard..."), this);
    connect(m_actAbout, &QAction::triggered, this, [this]() {
        QMessageBox::about(this, QStringLiteral("About Whiteboard"),
                           QStringLiteral("<b>WhiteboardQt</b> - Excalidraw Clone in Qt 6 / C++20<br>"
                                          "Handwritten sketchy diagramming with Rough.js algorithm.<br><br>"
                                          "Built with Qt 6.8.0, Modern C++20, and MinGW_64."));
    });
}

void MainWindow::createMenus()
{
    auto* mb = menuBar();

    auto* fileMenu = mb->addMenu(QStringLiteral("&File"));
    fileMenu->addAction(m_actNew);
    fileMenu->addAction(m_actOpen);
    fileMenu->addAction(m_actSave);
    fileMenu->addAction(m_actSaveAs);
    fileMenu->addSeparator();
    fileMenu->addAction(m_actInsertImage);
    fileMenu->addSeparator();
    fileMenu->addAction(m_actExportPng);
    fileMenu->addAction(m_actExportSvg);
    fileMenu->addSeparator();
    fileMenu->addAction(m_actExit);

    auto* editMenu = mb->addMenu(QStringLiteral("&Edit"));
    editMenu->addAction(m_actUndo);
    editMenu->addAction(m_actRedo);
    editMenu->addSeparator();
    editMenu->addAction(m_actPaste);
    editMenu->addAction(m_actDuplicate);
    editMenu->addAction(m_actCopyAsPng);
    editMenu->addSeparator();
    editMenu->addAction(m_actGroup);
    editMenu->addAction(m_actUngroup);
    editMenu->addAction(m_actToggleLock);
    editMenu->addSeparator();
    editMenu->addAction(m_actDelete);
    editMenu->addAction(m_actSelectAll);

    auto* viewMenu = mb->addMenu(QStringLiteral("&View"));
    viewMenu->addAction(m_actZoomIn);
    viewMenu->addAction(m_actZoomOut);
    viewMenu->addAction(m_actResetZoom);
    viewMenu->addAction(m_actZoomToFit);
    viewMenu->addSeparator();
    viewMenu->addAction(m_actDarkMode);
    viewMenu->addAction(m_actGrid);
    viewMenu->addAction(m_actSnapGrid);
    viewMenu->addAction(m_actToggleProps);
    viewMenu->addAction(m_actFullScreen);

    auto* helpMenu = mb->addMenu(QStringLiteral("&Help"));
    helpMenu->addAction(m_actAbout);
}

void MainWindow::createStatusBar()
{
    auto* sb = statusBar();

    m_zoomStatusLabel = new QLabel(QStringLiteral("100%"), sb);
    m_zoomStatusLabel->setMinimumWidth(60);
    sb->addWidget(m_zoomStatusLabel);

    m_toolStatusLabel = new QLabel(ToolManager::toolName(m_toolManager->currentTool()), sb);
    m_toolStatusLabel->setMinimumWidth(120);
    sb->addWidget(m_toolStatusLabel);

    m_selectionStatusLabel = new QLabel(QStringLiteral("0 items selected"), sb);
    sb->addPermanentWidget(m_selectionStatusLabel);
}

void MainWindow::setupShortcuts()
{
    // Горячие клавиши инструментов: цифровые клавиши и буквы
    auto bindTool = [this](int key, Tool tool) {
        auto* sc = new QShortcut(QKeySequence(key), this);
        connect(sc, &QShortcut::activated, this, [this, tool]() { m_toolManager->setTool(tool); });
    };

    bindTool(Qt::Key_1, Tool::Select);
    bindTool(Qt::Key_V, Tool::Select);
    bindTool(Qt::Key_2, Tool::Hand);
    bindTool(Qt::Key_H, Tool::Hand);
    bindTool(Qt::Key_3, Tool::Rectangle);
    bindTool(Qt::Key_R, Tool::Rectangle);
    bindTool(Qt::Key_4, Tool::Ellipse);
    bindTool(Qt::Key_E, Tool::Ellipse);
    bindTool(Qt::Key_5, Tool::Diamond);
    bindTool(Qt::Key_6, Tool::Line);
    bindTool(Qt::Key_L, Tool::Line);
    bindTool(Qt::Key_7, Tool::Arrow);
    bindTool(Qt::Key_A, Tool::Arrow);
    bindTool(Qt::Key_8, Tool::Freehand);
    bindTool(Qt::Key_P, Tool::Freehand);
    bindTool(Qt::Key_9, Tool::Text);
    bindTool(Qt::Key_T, Tool::Text);
    bindTool(Qt::Key_S, Tool::StickyNote);
    bindTool(Qt::Key_0, Tool::Eraser);

    // Горячая клавиша тёмного режима 'D'
    auto* scDark = new QShortcut(QKeySequence(Qt::Key_D), this);
    connect(scDark, &QShortcut::activated, this, &MainWindow::toggleDarkMode);
}

void MainWindow::newDocument()
{
    if (!maybeSave()) return;

    for (auto* item : m_scene->whiteboardItems()) {
        m_scene->removeItem(item);
        delete item;
    }
    m_undoStack->clear();
    m_document.setCurrentFilePath(QString());
    m_document.setModified(false);
    updateWindowTitle();
}

void MainWindow::openDocument()
{
    if (!maybeSave()) return;

    QString dir = m_settings.lastOpenDirectory();
    QString filePath = QFileDialog::getOpenFileName(this, QStringLiteral("Open Whiteboard File"), dir,
                                                    QStringLiteral("Whiteboard Files (*.whiteboard *.json);;All Files (*.*)"));
    if (filePath.isEmpty()) return;

    ViewportInfo vp;
    if (m_document.loadFromFile(filePath, m_scene, vp)) {
        m_settings.setLastOpenDirectory(QFileInfo(filePath).absolutePath());
        m_settings.addRecentFile(filePath);
        m_view->setZoom(vp.zoom > 0 ? vp.zoom : 1.0);
        updateWindowTitle();
    } else {
        QMessageBox::warning(this, QStringLiteral("Error"), QStringLiteral("Could not load file."));
    }
}

bool MainWindow::saveDocument()
{
    if (m_document.currentFilePath().isEmpty()) {
        return saveDocumentAs();
    }

    ViewportInfo vp;
    vp.zoom = m_view->zoomFactor();

    if (m_document.saveToFile(m_document.currentFilePath(), m_scene->whiteboardItems(), vp)) {
        m_undoStack->setClean();
        updateWindowTitle();
        return true;
    }

    QMessageBox::warning(this, QStringLiteral("Error"), QStringLiteral("Could not save file."));
    return false;
}

bool MainWindow::saveDocumentAs()
{
    QString dir = m_settings.lastOpenDirectory();
    QString filePath = QFileDialog::getSaveFileName(this, QStringLiteral("Save Whiteboard File"), dir,
                                                    QStringLiteral("Whiteboard Files (*.whiteboard);;JSON Files (*.json)"));
    if (filePath.isEmpty()) return false;

    m_document.setCurrentFilePath(filePath);
    m_settings.setLastOpenDirectory(QFileInfo(filePath).absolutePath());
    return saveDocument();
}

void MainWindow::exportPng()
{
    QString dir = m_settings.lastOpenDirectory();
    QString filePath = QFileDialog::getSaveFileName(this, QStringLiteral("Export to PNG Image"), dir,
                                                    QStringLiteral("PNG Images (*.png)"));
    if (filePath.isEmpty()) return;

    bool selectedOnly = !m_scene->selectedWhiteboardItems().isEmpty();
    if (Document::exportPng(filePath, m_scene, selectedOnly)) {
        statusBar()->showMessage(QStringLiteral("Exported PNG successfully"), 3000);
    } else {
        QMessageBox::warning(this, QStringLiteral("Error"), QStringLiteral("Failed to export PNG. Ensure scene is not empty."));
    }
}

void MainWindow::exportSvg()
{
    QString dir = m_settings.lastOpenDirectory();
    QString filePath = QFileDialog::getSaveFileName(this, QStringLiteral("Export to SVG Vector Graphic"), dir,
                                                    QStringLiteral("SVG Files (*.svg)"));
    if (filePath.isEmpty()) return;

    bool selectedOnly = !m_scene->selectedWhiteboardItems().isEmpty();
    if (Document::exportSvg(filePath, m_scene, selectedOnly)) {
        statusBar()->showMessage(QStringLiteral("Exported SVG successfully"), 3000);
    } else {
        QMessageBox::warning(this, QStringLiteral("Error"), QStringLiteral("Failed to export SVG. Ensure scene is not empty."));
    }
}

void MainWindow::insertImage()
{
    QString dir = m_settings.lastOpenDirectory();
    QString filePath = QFileDialog::getOpenFileName(
        this,
        QStringLiteral("Выберите изображение"),
        dir,
        QStringLiteral("Изображения (*.png *.jpg *.jpeg *.bmp *.webp *.gif);;Все файлы (*.*)"));

    if (filePath.isEmpty()) {
        return;
    }

    m_settings.setLastOpenDirectory(QFileInfo(filePath).absolutePath());

    QImage img(filePath);
    if (img.isNull()) {
        QMessageBox::warning(this, QStringLiteral("Ошибка"), QStringLiteral("Не удалось загрузить выбранное изображение."));
        return;
    }

    auto* imageItem = new ImageItem(img);
    QPoint centerView = m_view->viewport()->rect().center();
    QPointF sceneCenter = m_view->mapToScene(centerView);
    imageItem->setPos(sceneCenter - QPointF(imageItem->geometryRect().width() / 2,
                                            imageItem->geometryRect().height() / 2));

    m_scene->clearSelection();
    m_undoStack->push(new AddItemCommand(m_scene, imageItem));
    imageItem->setSelected(true);
    statusBar()->showMessage(QStringLiteral("Изображение добавлено на холст"), 3000);
}

void MainWindow::paste()
{
    const QClipboard* clipboard = QApplication::clipboard();
    if (!clipboard) {
        return;
    }

    const QMimeData* mime = clipboard->mimeData();
    if (!mime) {
        return;
    }

    QPoint centerView = m_view->viewport()->rect().center();
    QPointF sceneCenter = m_view->mapToScene(centerView);

    // 1. Проверяем наличие изображения в буфере обмена
    if (!clipboard->image().isNull() || mime->hasImage()) {
        QImage img = clipboard->image();
        if (img.isNull() && mime->hasImage()) {
            img = qvariant_cast<QImage>(mime->imageData());
        }
        if (!img.isNull()) {
            auto* imageItem = new ImageItem(img);
            imageItem->setPos(sceneCenter - QPointF(imageItem->geometryRect().width() / 2,
                                                    imageItem->geometryRect().height() / 2));
            m_scene->clearSelection();
            m_undoStack->push(new AddItemCommand(m_scene, imageItem));
            imageItem->setSelected(true);
            statusBar()->showMessage(QStringLiteral("Изображение вставлено из буфера обмена"), 3000);
            return;
        }
    }

    // 2. Проверяем локальные файлы изображений
    if (mime->hasUrls()) {
        bool handled = false;
        QPointF curPos = sceneCenter;
        for (const QUrl& url : mime->urls()) {
            if (url.isLocalFile()) {
                QString path = url.toLocalFile();
                QImage img(path);
                if (!img.isNull()) {
                    auto* imageItem = new ImageItem(img);
                    imageItem->setPos(curPos - QPointF(imageItem->geometryRect().width() / 2,
                                                        imageItem->geometryRect().height() / 2));
                    m_scene->clearSelection();
                    m_undoStack->push(new AddItemCommand(m_scene, imageItem));
                    imageItem->setSelected(true);
                    curPos += QPointF(30, 30);
                    handled = true;
                }
            }
        }
        if (handled) {
            statusBar()->showMessage(QStringLiteral("Изображение(я) вставлены из файлов"), 3000);
            return;
        }
    }

    // 3. Если в буфере обмена текст — создаем текстовый блок
    if (mime->hasText()) {
        QString text = mime->text().trimmed();
        if (!text.isEmpty()) {
            auto* textItem = new TextItem(text, sceneCenter);
            textItem->setProperties(m_toolManager->defaultProperties());
            m_scene->clearSelection();
            m_undoStack->push(new AddItemCommand(m_scene, textItem));
            textItem->setSelected(true);
            statusBar()->showMessage(QStringLiteral("Текст вставлен из буфера обмена"), 3000);
            return;
        }
    }

    // 4. Если в буфере пусто, но выделены элементы — дублируем выделенные
    if (!m_scene->selectedWhiteboardItems().isEmpty()) {
        duplicateSelected();
    }
}

void MainWindow::copyAsPng()
{
    bool selectedOnly = !m_scene->selectedWhiteboardItems().isEmpty();
    // Высокое разрешение (scale factor 2.0x / HiDPI)
    QImage img = Document::renderToImage(m_scene, selectedOnly, true, 20.0, 2.0);

    if (img.isNull()) {
        statusBar()->showMessage(QStringLiteral("Нет объектов для копирования"), 3000);
        return;
    }

    QApplication::clipboard()->setImage(img);
    if (selectedOnly) {
        statusBar()->showMessage(QStringLiteral("Выделенные элементы скопированы в буфер как PNG"), 3500);
    } else {
        statusBar()->showMessage(QStringLiteral("Вся доска скопирована в буфер как PNG"), 3500);
    }
}

void MainWindow::deleteSelected()
{
    m_scene->deleteSelectedItems();
}

void MainWindow::duplicateSelected()
{
    m_scene->duplicateSelectedItems();
}

void MainWindow::selectAll()
{
    m_scene->selectAllItems();
}

void MainWindow::toggleDarkMode()
{
    bool dark = !m_settings.isDarkMode();
    m_settings.setDarkMode(dark);
    m_actDarkMode->setChecked(dark);
    applyTheme(dark);

    // Обновляем цвет обводки по умолчанию в соответствии с новой темой, чтобы новые фигуры
    // были видны на фоне холста по умолчанию.
    ItemProperties props = m_toolManager->defaultProperties();
    props.strokeColor = dark ? QColor(0xe0, 0xe0, 0xe0) : QColor(0x1e, 0x1e, 0x1e);
    m_toolManager->setDefaultProperties(props);
    m_propsPanel->updateFromSelection(); // обновляем панель, чтобы отобразить новое значение по умолчанию
}

void MainWindow::toggleGrid()
{
    bool grid = !m_settings.isGridVisible();
    m_settings.setGridVisible(grid);
    m_actGrid->setChecked(grid);
    m_view->setGridVisible(grid);
}

void MainWindow::togglePropertiesPanel()
{
    m_propsPanel->setVisible(!m_propsPanel->isVisible());
}

void MainWindow::updateZoomLabel(double factor)
{
    m_zoomStatusLabel->setText(QStringLiteral("%1%").arg(static_cast<int>(std::round(factor * 100.0))));
}

void MainWindow::updateSelectionStatus()
{
    int count = m_scene->selectedWhiteboardItems().size();
    if (count == 0) {
        m_selectionStatusLabel->setText(QStringLiteral("0 items selected"));
    } else if (count == 1) {
        m_selectionStatusLabel->setText(QStringLiteral("1 item selected"));
    } else {
        m_selectionStatusLabel->setText(QStringLiteral("%1 items selected").arg(count));
    }
}

void MainWindow::updateWindowTitle()
{
    QString fileName = m_document.currentFilePath().isEmpty()
                           ? QStringLiteral("Untitled")
                           : QFileInfo(m_document.currentFilePath()).fileName();

    if (m_document.isModified()) {
        fileName.append(QStringLiteral("*"));
    }

    setWindowTitle(QStringLiteral("%1 - Whiteboard").arg(fileName));
}

void MainWindow::applyTheme(bool isDark)
{
    m_view->setDarkMode(isDark);
    m_toolbar->updateTheme(isDark);
    m_propsPanel->updateTheme(isDark);

    // Формируем полную QPalette приложения, чтобы каждый виджет — меню, строка состояния,
    // области прокрутки, метки, выпадающие списки — наследовал правильные цвета без
    // виджет-специфичных QSS-переопределений.
    QPalette pal;
    if (isDark) {
        const QColor bg(0x18, 0x18, 0x18);         // основной фон
        const QColor bgAlt(0x22, 0x22, 0x22);      // чередующиеся строки / панели
        const QColor panel(0x1e, 0x1e, 0x1e);      // фон группы / панели
        const QColor text(0xe0, 0xe0, 0xe0);        // основной текст
        const QColor textDisabled(0x70, 0x70, 0x70);
        const QColor highlight(0x1c, 0x7e, 0xd6);  // синий цвет выделения
        const QColor highlightText(0xff, 0xff, 0xff);
        const QColor border(0x40, 0x40, 0x40);
        const QColor button(0x2a, 0x2a, 0x2a);

        pal.setColor(QPalette::Window,          bg);
        pal.setColor(QPalette::WindowText,      text);
        pal.setColor(QPalette::Base,            bgAlt);
        pal.setColor(QPalette::AlternateBase,   panel);
        pal.setColor(QPalette::Text,            text);
        pal.setColor(QPalette::Button,          button);
        pal.setColor(QPalette::ButtonText,      text);
        pal.setColor(QPalette::Highlight,       highlight);
        pal.setColor(QPalette::HighlightedText, highlightText);
        pal.setColor(QPalette::ToolTipBase,     panel);
        pal.setColor(QPalette::ToolTipText,     text);
        pal.setColor(QPalette::PlaceholderText, textDisabled);
        pal.setColor(QPalette::Disabled, QPalette::WindowText, textDisabled);
        pal.setColor(QPalette::Disabled, QPalette::Text,       textDisabled);
        pal.setColor(QPalette::Disabled, QPalette::ButtonText, textDisabled);

        // Минимальный QSS только для декоративных элементов (рамки, скругления, состояние checked)
        setStyleSheet(QStringLiteral(
            "QMenuBar { background: #181818; color: #e0e0e0; }"
            "QMenuBar::item:selected { background: #2a2a2a; }"
            "QMenu { background: #1e1e1e; color: #e0e0e0; border: 1px solid #404040; }"
            "QMenu::item:selected { background: #1c7ed6; color: #ffffff; }"
            "QStatusBar { background: #181818; color: #a0a0a0; }"
            "QToolTip { background: #1e1e1e; color: #e0e0e0; border: 1px solid #404040; }"));
    } else {
        const QColor bg(0xf8, 0xf9, 0xfa);
        const QColor bgAlt(0xff, 0xff, 0xff);
        const QColor text(0x21, 0x25, 0x29);
        const QColor textDisabled(0xa0, 0xa0, 0xa0);
        const QColor highlight(0x19, 0x71, 0xc2);
        const QColor highlightText(0xff, 0xff, 0xff);
        const QColor button(0xff, 0xff, 0xff);

        pal.setColor(QPalette::Window,          bg);
        pal.setColor(QPalette::WindowText,      text);
        pal.setColor(QPalette::Base,            bgAlt);
        pal.setColor(QPalette::AlternateBase,   bg);
        pal.setColor(QPalette::Text,            text);
        pal.setColor(QPalette::Button,          button);
        pal.setColor(QPalette::ButtonText,      text);
        pal.setColor(QPalette::Highlight,       highlight);
        pal.setColor(QPalette::HighlightedText, highlightText);
        pal.setColor(QPalette::ToolTipBase,     QColor(0xff, 0xff, 0xe0));
        pal.setColor(QPalette::ToolTipText,     text);
        pal.setColor(QPalette::Disabled, QPalette::WindowText, textDisabled);
        pal.setColor(QPalette::Disabled, QPalette::Text,       textDisabled);
        pal.setColor(QPalette::Disabled, QPalette::ButtonText, textDisabled);

        setStyleSheet(QStringLiteral(
            "QMenuBar { background: #f8f9fa; color: #212529; }"
            "QMenuBar::item:selected { background: #e9ecef; }"
            "QMenu { background: #ffffff; color: #212529; border: 1px solid #dee2e6; }"
            "QMenu::item:selected { background: #1971c2; color: #ffffff; }"
            "QStatusBar { background: #f1f3f5; color: #6c757d; }"
            "QToolTip { background: #ffffff; color: #212529; border: 1px solid #ced4da; }"));
    }

    // Применяем палитру ко всему приложению — она распространяется на каждый дочерний виджет,
    // включая меню, строку состояния, области прокрутки, выпадающие списки и метки.
    qApp->setPalette(pal);
}

bool MainWindow::maybeSave()
{
    if (!m_document.isModified()) {
        return true;
    }

    auto ret = QMessageBox::warning(this, QStringLiteral("Unsaved Changes"),
                                    QStringLiteral("The whiteboard has unsaved changes.\nDo you want to save your changes?"),
                                    QMessageBox::Save | QMessageBox::Discard | QMessageBox::Cancel);

    if (ret == QMessageBox::Save) {
        return saveDocument();
    }
    if (ret == QMessageBox::Cancel) {
        return false;
    }
    return true;
}

} // namespace Whiteboard
