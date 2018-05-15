# qfileinfo-exists

Finds places using `QFileInfo("filename").exists()` instead of the faster version `QFileInfo::exists("filename")`.

According to Qt's docs:
"Using this function is faster than using QFileInfo(file).exists() for file system access."
