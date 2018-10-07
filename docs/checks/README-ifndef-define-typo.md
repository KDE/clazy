# ifndef-define-typo

Tries to find cases where a `#define` following an `#ifndef` defines a different but similar name.

Example:
```
#ifndef GL_FRAMEBUFFER_SRG // Oops, typo.
# define GL_FRAMEBUFFER_SRGB 0x8DB9
#endif
```
This check uses a Levenshtein Distance algorithm so it will only warn if the names are similar.
This check is disabled by default as it will report many false-positives.
