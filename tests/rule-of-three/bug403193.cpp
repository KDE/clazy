class APrivate
{
public:
    int i = 3;
};

class A
{
public:
    A() : d(new APrivate()) {}
    A(const A &other) : d(new APrivate(*other.d)) {}
    ~A() { delete d; }
private:
    APrivate *const d;
};

void test()
{
    A a;
    A a2;
}
