# static-pmf

Warns when storing a pointer to `QObject` member function into a static variable and passing it to a connect statement.
This is known to make the connect statement fail when the code is built with **MingW**.

Example:
```
static auto pmf = &QObject::destroyed;
QCOMPARE(pmf, &QObject::destroyed); // fails
```
