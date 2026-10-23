#include "ui/MainWindow.h"
#include <QApplication>

int main(int argc, char *argv[])
{
    QApplication a(argc, argv);
    a.setApplicationName(QStringLiteral("WhiteboardQt"));
    a.setOrganizationName(QStringLiteral("WhiteboardQt"));

    Whiteboard::MainWindow w;
    w.show();
    return QApplication::exec();
}
