# qstring-left
Finds places where you're using `QString::left(0)` instead of `QString::at(0)`.
The later form is cheaper.
