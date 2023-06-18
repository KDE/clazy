# no-module-include

Warns when a Qt module is included using the module wide include.
Prefer include the direct class includes to avoid pulling all the Qt internal dependencies.

See https://www.kdab.com/beware-of-qt-module-wide-includes/ for more details.
