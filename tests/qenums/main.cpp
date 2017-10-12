#include <QtCore/QObject>

class B
{
public:
    enum BB {

    };
};

class A : public QObject
{
public:
    enum E {

    };
    Q_ENUMS(E) // Warning

    enum E2 {

    };
    Q_ENUM(E2)

    Q_ENUMS(B::BB) // OK

};
