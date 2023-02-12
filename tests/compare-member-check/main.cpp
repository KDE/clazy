#include <tuple>

class ExampleClass
{
public:
    int a;
    int b;
    int c;

    bool operator==(const ExampleClass &other) const
    {
        return a == other.a || b == other.b || c == other.c; // No Warning, Using all the member variables in comparison
    }

    bool operator>(const ExampleClass &other) const
    {
        return a > other.a || b > other.b; // Warning, Using only two member variables in the comparison
    }

    bool operator<(const ExampleClass &rhs) const
    {
        return std::tie(a, b, c) < std::tie(rhs.a, rhs.b, rhs.c); // No Warning, Using all the member variables in comparison
    }
    bool operator>=(const ExampleClass &rhs) const
    {
        return std::tie(a) >= std::tie(rhs.a); // Warning, Using only one member variable in comparison
    }
};

int main()
{
    ExampleClass object1;
    ExampleClass object2;

    object1.a = 5;
    object1.b = 10;
    object1.c = 15;

    object2.a = 5;
    object2.b = 20;
    object2.c = 30;

    if (object1 == object2) { // Should not trigger a warning from the checker
    }

    if (object1 > object2) { // Should trigger a warning from the checker
    }
    if (object1 < object2) { // Should not trigger a warning from the checker
    }
    if (object1 >= object2) { // Should trigger a warning from the checker
    }

    return 0;
}