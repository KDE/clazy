#include "main.h"
#include <QtGui/QColor>
#include <QtCore/QSharedDataPointer>

struct Private
{
    ~Private() {};
};

struct AnotherPrivate
{
    AnotherPrivate(const AnotherPrivate &) {};
};


Q_GLOBAL_STATIC(QObject, s_a)

class MyPrivate;
struct WithQSharedDataPointer
{
    ~WithQSharedDataPointer();
    WithQSharedDataPointer& operator=(const WithQSharedDataPointer &);

private:
    QSharedDataPointer<MyPrivate> d;
};
