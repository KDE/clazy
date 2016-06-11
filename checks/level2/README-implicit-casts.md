# implicit-casts

Finds places with unwanted implicit casts in function calls.

#### Supported cases

1. pointer->bool cast in functions accepting bool and pointers, example:
```
   MyWidget(bool b, QObject *parent = nullptr) {}
   ...
   MyWidget(parent);
```

2. bool->int
```
void func(int duration);
func(someBool);
```

This case is disabled due to false positives when calling C code.
You can uncomment it and recompile clazy as it usually finds bugs.
