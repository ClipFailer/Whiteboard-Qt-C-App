#include "core/AppSettings.h"

namespace Whiteboard {

namespace {
constexpr auto KeyDarkMode = "ui/darkMode";
constexpr auto KeyGridVisible = "canvas/gridVisible";
constexpr auto KeyLastOpenDir = "io/lastOpenDir";
constexpr auto KeyRecentFiles = "io/recentFiles";
constexpr int MaxRecentFiles = 10;
} // namespace

AppSettings::AppSettings()
    : m_settings("WhiteboardQt", "Whiteboard")
{
}

bool AppSettings::isDarkMode() const
{
    return m_settings.value(KeyDarkMode, false).toBool();
}

void AppSettings::setDarkMode(bool dark)
{
    m_settings.setValue(KeyDarkMode, dark);
}

bool AppSettings::isGridVisible() const
{
    return m_settings.value(KeyGridVisible, true).toBool();
}

void AppSettings::setGridVisible(bool visible)
{
    m_settings.setValue(KeyGridVisible, visible);
}

QString AppSettings::lastOpenDirectory() const
{
    return m_settings.value(KeyLastOpenDir, QString()).toString();
}

void AppSettings::setLastOpenDirectory(const QString& dir)
{
    m_settings.setValue(KeyLastOpenDir, dir);
}

QStringList AppSettings::recentFiles() const
{
    return m_settings.value(KeyRecentFiles, QStringList()).toStringList();
}

void AppSettings::addRecentFile(const QString& filePath)
{
    if (filePath.trimmed().isEmpty()) {
        return;
    }

    QStringList recents = recentFiles();
    recents.removeAll(filePath);
    recents.prepend(filePath);
    while (recents.size() > MaxRecentFiles) {
        recents.removeLast();
    }
    m_settings.setValue(KeyRecentFiles, recents);
}

} // namespace Whiteboard
