# qcolor-from-literal

Warns when a `QColor` is being constructed from a string literal such as "#RRGGBB".
This is less performant than calling the ctor that takes `int`s, since it creates temporary `QString`s.

Example:

`QColor c("#000000");` // Use QColor c(0, 0, 0) instead

`c.setNamedColor("#001122");` // Use c = QColor(0, 0x11, 0x22) instead
