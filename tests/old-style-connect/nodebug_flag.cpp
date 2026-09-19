#include <QtCore/QObject>

void foo()
{
    QObject *o1;
    QObject *o2;
    o1->connect(o1, SIGNAL(destroyed(QObject *)), o2, SLOT(deleteLater()));
}

