#include <QtCore/QObject>

class A : public QObject
{
public:
    enum E {

    };
    Q_ENUMS(E) // Warning

    enum E2 {

    };
    Q_ENUM(E2)

};
