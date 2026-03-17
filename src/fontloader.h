#pragma once

#include <QFont>
#include <QFontDatabase>
#include <QString>

namespace font_loader {

inline QFont loadFont(const QString& family, const QString& style, int pointSize) {
    return QFontDatabase::font(family, style, pointSize);
}
}  // namespace font_loader
