#ifndef WIDGETSTYLEUTILS_H
#define WIDGETSTYLEUTILS_H

#include <QDebug>
#include <QStyle>
#include <QString>
#include <QWidget>

#include "editorsettingsoptions.h"

inline void setCSSClassesAndUpdate(QWidget* obj, const std::string& classNames) {
    if (obj->styleSheet().isEmpty()) {
        qWarning() << "setCSSClassesAndUpdate: styleSheet is empty for widget with name " << obj->objectName();
    }
    obj->setProperty("class", classNames.c_str());
    obj->style()->polish(obj);
    obj->update();
}

inline void setCSSThemeAndUpdate(QWidget* obj, Theme::Value theme) {
    setCSSClassesAndUpdate(obj, QString::fromStdString(to_string(theme)).toLower().toStdString());
}

#endif  // WIDGETSTYLEUTILS_H
