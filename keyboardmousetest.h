#ifndef KEYBOARDMOUSETEST_H
#define KEYBOARDMOUSETEST_H
#include "novenatest.h"

class QInputDialog;

class KeyboardMouseTest : public NovenaTest
{
    Q_OBJECT

    QInputDialog *window;

public:
    KeyboardMouseTest();
    void runTest();

signals:
    void showWindow(void);
};

#endif // KEYBOARDMOUSETEST_H
