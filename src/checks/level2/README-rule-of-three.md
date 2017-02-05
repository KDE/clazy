# rule-of-three

Implements the rule of three:
<https://en.wikipedia.org/wiki/Rule_of_three_%28C%2B%2B_programming%29>

#### Exceptions
To reduce the amount of warnings, these cases won't emit warnings:
- class has a QSharedDataPointer member
- class inherits from QSharedData
- if only the dtor is implemented and it's protected
- class name ends with "Private" and is defined in a .cpp, .cxx or _p.h file

In some cases you're missing methods, in others you have too many methods. You'll have to judge what's the correct fix and beware of binary compatibility.
