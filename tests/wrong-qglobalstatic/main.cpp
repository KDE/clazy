#include <QtCore/qglobal.h>

struct Pod
{
};

struct NonPod
{
    NonPod() {}
};

Q_GLOBAL_STATIC(Pod, p); // Warning
Q_GLOBAL_STATIC(NonPod, p2);  // OK
Q_GLOBAL_STATIC(int, p3); // Warning


extern int foo();
Q_GLOBAL_STATIC_WITH_ARGS(int, p4, (foo())); // OK
