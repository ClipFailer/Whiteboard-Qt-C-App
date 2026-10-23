#pragma once

#include <QSettings>
#include <QString>
#include <QStringList>

namespace Whiteboard {

class AppSettings {
public:
    AppSettings();

    bool isDarkMode() const;
    void setDarkMode(bool dark);

    bool isGridVisible() const;
    void setGridVisible(bool visible);

    QString lastOpenDirectory() const;
    void setLastOpenDirectory(const QString& dir);

    QStringList recentFiles() const;
    void addRecentFile(const QString& filePath);

private:
    QSettings m_settings;
};

} // namespace Whiteboard
