#ifndef TST_NOTEDATA_H
#define TST_NOTEDATA_H

#include <QtTest>

#include "nodedata.h"

class tst_NoteData : public QObject {
    Q_OBJECT

public:
    tst_NoteData() = default;

private Q_SLOTS:
    void initTestCase();
    void cleanupTestCase();
};

#endif  // TST_NOTEDATA_H
