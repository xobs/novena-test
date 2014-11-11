#include "keyboardmousetest.h"

#include <QInputDialog>
#include <QEventLoop>
#include <QPushButton>
#include <QDialogButtonBox>

KeyboardMouseTest::KeyboardMouseTest()
{
    QPushButton *todelete = NULL;
    QPushButton *okbutton = NULL;

    name = "Keyboard and mouse test";

    window = new QInputDialog();
    window->setCancelButtonText("Deleteme");
    window->setOkButtonText("Fixme");
    window->setWindowTitle("Kbd/Mouse test");
    window->setLabelText("Type anything into the box and hit \"Okay\"");
    window->setFocusPolicy(Qt::NoFocus);

    foreach (const QObject *child, window->children()) {
        if (child->inherits("QDialogButtonBox")) {
            foreach (const QObject *childchild, child->children()) {
                if (childchild->inherits("QPushButton")) {
                    if (((QPushButton *)childchild)->text() == "Deleteme")
                        todelete = (QPushButton *)childchild;
                    else if (((QPushButton *)childchild)->text() == "Fixme") {
                        okbutton = (QPushButton *)childchild;
                    }
                }
            }

            if (todelete) {
                ((QDialogButtonBox *)child)->removeButton(todelete);
            }
            if (okbutton) {
                okbutton->setFocusPolicy(Qt::NoFocus);
                okbutton->setDefault(false);
                okbutton->setText("Okay");
                ((QDialogButtonBox *)child)->addButton(okbutton, QDialogButtonBox::ApplyRole);
            }
        }
    }

    if (!okbutton) {
        qFatal("Couldn't find ok button");
        window = NULL;
        return;
    }

    connect(okbutton, SIGNAL(clicked()), window, SLOT(accept()));
    connect(this, SIGNAL(showWindow()), window, SLOT(open()));
}

void KeyboardMouseTest::runTest()
{
    bool finished;
    QEventLoop loop;

    if (!window) {
        testError("Test window not found, cannot continue");
        return;
    }

    QObject::connect(window, SIGNAL(finished(int)), &loop, SLOT(quit()));


    finished = false;

    while (!finished) {
        emit showWindow();

        testDebug("Waiting for window to close...");
        // Execute the event loop here, waiting for the window to close.
        loop.exec();

        testInfo(QString("Window closed.  Result: %1  Value: %2").arg(window->result()).arg(window->textValue()));

        if (window->result() && window->textValue() != "")
            finished = true;
    }
}
