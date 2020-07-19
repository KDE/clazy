#include <QtCore/QHash>
#include <QtCore/QString>

class Employee
{
public:
    Employee() {}
    Employee(const QString &name);
    QString name() {return myName;}
    uint test_var = qHash("blabla", 0);

private:
    QString myName;
};

inline uint qHash(Employee *key, uint seed)
{
    return qHash(key->name(), seed) ^ 4;
}

uint test_var_2 = qHash("blabla", 0);

uint anotherFunction()
{
    int toto_NOT_qhash_related = 3;
    return qHash("blabla", 0);
}

void test()
{
    QString name = "Bob";
    Employee *theemploye = new Employee(name);
    uint foo = qHash(theemploye, 0);
    size_t foo_g = qHash(theemploye, 0);

    unsigned char p[2];
    uint foo_bits = qHashBits(p, 0, 0);
    static const int ints[] = {0, 1, 2, 3, 4, 5};
    uint foo_range = qHashRange(ints, ints);
    uint foo_rangec = qHashRangeCommutative(ints, ints);
}

