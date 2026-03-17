#ifndef TAGDATA_H
#define TAGDATA_H

#include <QMetaClassInfo>
#include <QString>

namespace {
auto constexpr INVALID_TAG_ID = -1;
}

class TagData {
public:
    TagData();

    [[nodiscard]] int id() const;
    void setId(int newId);

    [[nodiscard]] const QString& name() const;
    void setName(const QString& newName);

    [[nodiscard]] const QString& color() const;
    void setColor(const QString& newColor);

    [[nodiscard]] int relativePosition() const;
    void setRelativePosition(int newRelativePosition);

    [[nodiscard]] int childNotesCount() const;
    void setChildNotesCount(int newChildCount);

private:
    int m_id;
    QString m_name;
    QString m_color;
    int m_relativePosition;
    int m_childNotesCount;
};

Q_DECLARE_METATYPE(TagData)

#endif  // TAGDATA_H
