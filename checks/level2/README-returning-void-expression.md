# returning-void-expression

Warns when returning a void expression.

Example:
```
void doStuff()
{
    if (cond)
        return // Oops, forgot the ; but it still compiles since processStuff() returns void.

    processStuff();
}
```
