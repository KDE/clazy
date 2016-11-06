# copyable-polymorphic

Finds polymorphic classes that are copyable.
These classes are usually vulnerable to slicing [1].

To fix these warnings use Q_DISABLE_COPY or delete the copy-ctor yourself.

[1] <https://en.wikipedia.org/wiki/Object_slicing>
