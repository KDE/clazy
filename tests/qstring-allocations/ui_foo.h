#include <QtCore/QString>
#include <QtWidgets/QPushButton>
#include <QtGui/QKeySequence>

void foo(QString) {}
void bar()
{
    foo("test"); // OK, since it's in a UI file
    QPushButton b;
    b.setShortcut(QString("Alt+F4")); // OK, since it's in a UI file
}
