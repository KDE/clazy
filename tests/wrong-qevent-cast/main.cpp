#include <QtCore/QObject>
#include <QtCore/QString>
#include <QtCore/QEvent>
#include <QtGui/QKeyEvent>

void test(QEvent *ev)
{

    switch (ev->type()) {
        case QEvent::MouseMove: {
            auto a = static_cast<QKeyEvent*>(ev); // Warn
            auto b = static_cast<QMouseEvent*>(ev); // OK
            break;
        }
        case QEvent::KeyPress: {
            auto a = static_cast<QKeyEvent*>(ev); // OK
            auto b = static_cast<QMouseEvent*>(ev); // Warn

            int val = 0;
            switch (val) { // unrelated switch
                case 1000: {
                    auto a = static_cast<QKeyEvent*>(ev); // OK
                    auto b = static_cast<QMouseEvent*>(ev); // Warn
                }
            }
            break;
        }

        case QEvent::Paint:
        case QEvent::MetaCall: {
            if (ev->type() == QEvent::Paint)
                auto pe = static_cast<QPaintEvent*>(ev); // OK
            break;
        }

        default:
            break;
    }


}
