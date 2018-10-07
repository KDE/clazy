# qstring-varagars

This implements the equivalent of `-Wnon-pod-varargs` but only for `QString`.

This check is disabled by default and is only useful in cases where you don't want to enable `-Wnon-pod-varargs`. For example on projects with thousands of benign warnings (like with MFC's `CString`), where you might only want to fix the `QString` cases.

Example
```
QString s = (...)
LogError("error %s", s);
```
