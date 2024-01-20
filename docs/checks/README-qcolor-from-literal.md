# qcolor-from-literal

Warns when a `QColor` is being constructed from a string literal such as "#RRGGBB".
Next to the constructor call, this includes QColor::setNamedColor and the static QColor::fromString method.
This is less performant than calling the ctor that takes `int`s, since it creates temporary `QString`s.
Also, the pattern is checked in regards to it only containing hexadecimal values and being of a supported length.

### Fixits
This check provides fixits for #RGB, #RRGGBB and #AARRGGBB patterns.
For QColor object needing more precision, you should manually create a QRgba64 object.

For example:
```
testfile.cpp:92:16: warning: The QColor ctor taking RGB int value is cheaper than one taking string literals [-Wclazy-qcolor-from-literal]
        QColor("#123");
               ^~~~~~
               0x112233

testfile.cpp:93:16: warning: The QColor ctor taking RGB int value is cheaper than one taking string literals [-Wclazy-qcolor-from-lite
        QColor("#112233");
               ^~~~~~~~~
               0x112233

testfile.cpp:92:16: warning: The QColor ctor taking ints is cheaper than one taking string literals [-Wclazy-qcolor-from-literal]
        QColor("#9931363b");
               ^~~~~~~~~~~
               0x31, 0x36, 0x3b, 0x99
```
