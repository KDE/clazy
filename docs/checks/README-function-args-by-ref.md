# function-args-by-ref

Warns when you should be passing by const-ref.
Types with sizeof > 16 bytes [1] or types which are not trivially-copyable [2] or not trivially-destructible [3] should be passed by ref. A rule of thumb is that if passing by value would trigger copy-ctor and/or dtor then pass by ref instead.

This check will ignore shared pointers, you're on your own. Most of the times passing shared pointers by const-ref is the best thing to do, but occasionally that will lead to crashes if you're in a method that calls something else that makes the shared pointer ref count go down to zero.


#### Fixits

This check supports adding missing `&` or `const-&`. See the README.md on how to enable it.


- [1] <http://www.macieira.org/blog/2012/02/the-value-of-passing-by-value/>
- [2] <https://en.cppreference.com/w/cpp/named_req/TriviallyCopyable>
- [3] <http://www.cplusplus.com/reference/type_traits/is_trivially_destructible/>
