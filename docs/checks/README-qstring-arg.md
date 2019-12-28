# qstring-arg

Implements three warnings:

1. Detects when you're using chained `QString::arg()` calls and should instead use the multi-arg overload to save memory allocations

        QString("%1 %2").arg(a).arg(b);
        QString("%1 %2").arg(a, b); // one less temporary heap allocation

2. Detects when you're passing an integer to QLatin1String::arg() as that gets implicitly cast to QChar.
It's preferable to state your intention and cast to QChar explicitly.

3. Detects when you're using misleading `QString::arg()` overloads

        QString arg(qlonglong a, int fieldwidth = 0, int base = 10, QChar fillChar = QLatin1Char(' ')) const
        QString arg(qulonglong a, int fieldwidth = 0, int base = 10, QChar fillChar = QLatin1Char(' ')) const
        QString arg(long a, int fieldwidth = 0, int base=10, QChar fillChar = QLatin1Char(' ')) const
        QString arg(ulong a, int fieldwidth = 0, int base=10, QChar fillChar = QLatin1Char(' ')) const
        QString arg(int a, int fieldWidth = 0, int base = 10, QChar fillChar = QLatin1Char(' ')) const
        QString arg(uint a, int fieldWidth = 0, int base = 10, QChar fillChar = QLatin1Char(' ')) const
        QString arg(short a, int fieldWidth = 0, int base = 10, QChar fillChar = QLatin1Char(' ')) const
        QString arg(ushort a, int fieldWidth = 0, int base = 10, QChar fillChar = QLatin1Char(' ')) const
        QString arg(double a, int fieldWidth = 0, char fmt = 'g', int prec = -1, QChar fillChar = QLatin1Char(' ')) const
        QString arg(char a, int fieldWidth = 0, QChar fillChar = QLatin1Char(' ')) const
        QString arg(QChar a, int fieldWidth = 0, QChar fillChar = QLatin1Char(' ')) const
        QString arg(const QString &a, int fieldWidth = 0, QChar fillChar = QLatin1Char(' ')) const

because they are commonly misused, for example:

        int hours = ...;
        int minutes = ...;
        // This won't do what you think it would at first glance.
        QString s("The time is %1:%2").arg(hours, minutes);

To reduce false positives, some cases won't be warned about:

        str.arg(hours, 2); // User explicitly used a integer literal, it's probably fine
        str.arg(foo); // We're only after cases where the second argument (or further) is specified, so this is safe
        str.arg(foo, width); // Second argument is named width, or contains the name "width", it's safe. Same for third argument and "base".

Using these misleading overloads is perfectly valid, so only warning (1) is enabled by default.
To enable warning (2), `export CLAZY_EXTRA_OPTIONS="qstring-arg-fillChar-overloads"`
