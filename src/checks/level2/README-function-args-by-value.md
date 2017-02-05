# function-args-by-value

Warns when you should be passing by value instead of by-ref.
Types with sizeof <= 16 bytes [1] which are trivially-copyable [2] and trivially-destructible [3] should be passed by value.

Only fix these warnings if you're sure that the value would be passed in a CPU register instead on the stack.

- [1] <http://www.macieira.org/blog/2012/02/the-value-of-passing-by-value/>
- [2] <http://en.cppreference.com/w/cpp/concept/TriviallyCopyable>
- [3] <http://www.cplusplus.com/reference/type_traits/is_trivially_destructible/>
