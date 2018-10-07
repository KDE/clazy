# raw-environment-function

Warns when `putenv()` or `qputenv()` are being used and suggests the Qt *thread-safe* equivalents instead.

This check is disabled by default and should be enabled manually if *thread-safety* is an issue for you.
