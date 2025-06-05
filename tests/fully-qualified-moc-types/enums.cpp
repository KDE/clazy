#include <QtCore/QObject>

namespace NS
{
class TestClass : QObject
{
  Q_OBJECT
    enum E {
    };
    enum class EC {
    };
Q_SIGNALS:
    void function(NS::TestClass::E value); // OK
    void function2(TestClass::E value); // WARN
    void function3(E value); // WARN

    void function(NS::TestClass::EC value); // OK
    void function2(TestClass::EC value); // WARN
    void function3(EC value); // WARN
};
}
