# implicit-casts

Finds places with unwanted implicit casts in function calls.

#### Supported cases

* pointer->bool cast in functions accepting bool and pointers, example:

        MyWidget(bool b, QObject *parent = nullptr) {}
        MyWidget(parent);

* bool->int

        void func(int duration);
        func(someBool);

This last case is disabled due to false positives when calling C code.
You can enable it by with:
`export CLAZY_EXTRA_OPTIONS=implicit-casts-bool-to-int`
