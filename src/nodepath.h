#pragma once

#include <QList>
#include <QString>

auto constexpr PATH_SEPARATOR = '/';
auto constexpr FOLDER_MIME = "application/x-foldernode";
auto constexpr TAG_MIME = "application/x-tagnode";
auto constexpr NOTE_MIME = "application/x-notenode";

class NodePath {
public:
    // cppcheck-suppress noExplicitConstructor
    NodePath(QString path);
    [[nodiscard]] QStringList separate() const;

    [[nodiscard]] const QString& path() const;
    [[nodiscard]] NodePath parentPath() const;
    static QString getAllNoteFolderPath();
    static QString getTrashFolderPath();

private:
    QString m_path;
};
