# wrong-qglobalstatic

Finds `Q_GLOBAL_STATIC`s being used with trivial types.
This is unnecessary and creates code bloat.

#### Example:

    struct Trivial
    {
        int v;
    };

    Q_GLOBAL_STATIC(Trivial, t); // Wrong
    static Trivial t; // Correct
