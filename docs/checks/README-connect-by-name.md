# connect-by-name

Warns when "auto-connection slots" are used. They're also known as "connect by name", an old and unpopular feature which isn't recommended. Consult the [official documentation](http://doc.qt.io/qt-5/qobject.html#auto-connection) for more information.

These types of connections are very brittle, as a simple object rename would break your code.
In Qt 5 the *pointer-to-member-function* connect syntax is recommended as it catches errors at compile time.

This check simply warns for any slot named like on_*_*, because even if you're not using .ui files this naming is misleading and not good for readability, as the reader would think you're using auto-connection.
