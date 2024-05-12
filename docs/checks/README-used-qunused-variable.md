# used-qunused-variable

Finds places where you use `Q_UNUSED` or `void` casts even though the variable is still used.
This currently applies to function parameters.

Example:

```cpp
void test(int a)
{
    Q_UNUSED(a)
    int b = a + 5;
}
```
