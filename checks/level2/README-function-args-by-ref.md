# function-args-by-ref

Warns when you should be passing by const-ref.
Types with sizeof > 16 bytes [1] or types which are not trivially-copyable [2] or not trivially-destructible [3] should be passed by ref. A rule of thumb is that if passing by value would trigger copy-ctor and/or dtor then pass by ref instead.

This check will ignore shared pointers, you're on your own. Most of the times passing shared pointers by const-ref is the best thing to do, but occasionally that will lead to crashes if you're in a method that calls something else that makes the shared pointer ref count go down to zero.

Note that in some cases there might be false-positives: There's a pattern where you pass a movable type
by value and then move from it in the ctor initialization list. This check does not honour that and warns.

[1] http://www.macieira.org/blog/2012/02/the-value-of-passing-by-value/
[2] http://en.cppreference.com/w/cpp/concept/TriviallyCopyable
[3] http://www.cplusplus.com/reference/type_traits/is_trivially_destructible/
