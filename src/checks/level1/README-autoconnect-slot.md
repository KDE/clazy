# autoconnect-slot

Warns when use of the autoconnected slot behavior from uic generated setupUi() is detected.
E.g. slots following the naming convention "on_btnRefresh_clicked()". 

The connections are very fragile, since any changes to either the widget's name or type
will cause the connection to silently fail without any compile time or run time warnings.

Instead, explicit connections should be created for these slots to ensure compilation
failures if changes to the widget would break the slot connections.
