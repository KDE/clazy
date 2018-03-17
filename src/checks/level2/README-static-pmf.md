# static-pmf

Warns when storing a pointer to QObject member function into a static variable.
Passing such variable to a connect is known to fail when using MingW.

Example:

static auto pmf = &QObject::destroyed;
QCOMPARE(pmf, &QObject::destroyed); // fails
