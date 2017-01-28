# foreach

- Finds places where you're detaching the `foreach` container.
- Finds places where big or non-trivial types are passed by value instead of const-ref.
- Finds places where you're using `foreach` on STL containers. It causes deep-copy. Use C++11 range-loop instead.

**Note**: range-loop is prefered over `foreach` since the compiler generates less and more optimized code.
Use range-loop if your container is const, otherwise a detach will happen.

This check is disabled for Qt >= 5.9
