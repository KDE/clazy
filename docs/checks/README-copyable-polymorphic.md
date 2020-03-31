# copyable-polymorphic

Finds polymorphic classes that are copyable.
Classes with virtual methods are usually handled by pointer, as passing
then by value would allow up-casting to the base-class and slicing off the vtable.
Example:

```
struct Base {
    virtual int getNumber() const { return 41; }
};

struct Derived : public Base {
    int getNumber() const override { return 42; }
};

void printNumber(Base b)
{
    qDebug() << b.getNumber(); // Always prints 41!
}

(...)
Derived d;
printNumber(d);

```

To fix these warnings use `Q_DISABLE_COPY` or delete the copy-ctor yourself.
