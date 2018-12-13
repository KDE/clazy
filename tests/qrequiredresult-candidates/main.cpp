#include <QtCore/QObject>
#include <QtCore/QString>
class Foo
{
public:
    Foo();
    Foo& operator=(Foo&);
    Foo& change();
    Foo changed() const; // Warn
    Foo chang2ed() const { return *this; }  // Warn
    Foo chang3ed() const; // Warn

    [[nodiscard]] Foo chang4ed() const;
    Foo toBar() const;
private:
    Foo chang5ed() const { return *this; }

    class Bar {
    public:
        Bar changed() const { return *this; }
    };
};

Foo Foo::chang3ed() const // OK
{
    return *this;
}
