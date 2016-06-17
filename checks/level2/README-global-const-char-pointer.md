# global-const-char-pointer

Finds where you're using `const char *foo` instead of `const char *const foo` or `const char []foo`.
The former case adds a pointer in .data, pointing to .rodata. The later cases only use .rodata.
