# unused-result-check

Warns about the unused return value of const member functions. For example:  
```cpp
    class A : public QObject
{
    Q_OBJECT
public:
    int foo() const {
        return 5;
    }
    void bar() const {
        foo();  // Warn [Result of const member function is unused]
    }
};

int main(int argc, char *argv[]) {
    A a;
    a.bar();
    return 0; 
}

```
