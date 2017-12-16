# thread-with-slots

slots in a `QThread` derived class are usually a code smell, because
they'll run in the thread where the `QThread` `QObject` lives and not in
the thread itself.

Disabled by default since it's very hard to avoid for false-positives. You'll
have to explicitly enable it and check case by case for races.
