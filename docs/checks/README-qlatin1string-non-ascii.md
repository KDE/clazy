# qlatin1string-non-ascii

Finds places where you're using `QLatin1String` with a non-ascii literal like `QLatin1String("é")`.
This is almost always a mistake, since source files are usually in UTF-8.
