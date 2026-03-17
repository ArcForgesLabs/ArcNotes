#ifndef TST_NOTEVIEW_H
#define TST_NOTEVIEW_H

#include <QtTest>

class tst_NoteView : public QObject {
    Q_OBJECT

public:
    tst_NoteView() = default;

private Q_SLOTS:
    void initTestCase();
    void cleanupTestCase();
};

#endif  // TST_NOTEVIEW_H
