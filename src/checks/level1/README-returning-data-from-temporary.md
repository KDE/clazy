# returning-data-from-temporary

Warns when returning the data from a `QByteArray` that will soon be destroyed.

## Examples
```
QByteArray b = ...;
return b.data();
```
```
return funcReturningByteArray().data();
```


```
const char * getFoo()
{
    QByteArray b = ...;
    return b; // QByteArray can implicitly cast to char*
}
```
