# qgetenv

Warns on innefficient usages of `qgetenv()` which usually allocate memory.
Suggests usage of `qEnvironmentVariableIsSet()`, `qEnvironmentVariableIsEmpty()` and `qEnvironmentVariableIntValue()`.

These replacements are available since Qt 5.5.
