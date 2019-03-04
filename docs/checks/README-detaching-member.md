# detaching-member

Finds places where member containers are potentially detached.

#### Example

    QString MyClass::myMethod()
    {
        return m_container.first(); // Should be constFirst()
    }

#### Pitfalls
This check is disabled by default as it reports too many false positives.
