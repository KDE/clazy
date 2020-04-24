# jni-signatures

Tries to find invalid JNI signatures in QAndroidJniObject usage. Those will lead to runtime errors.

#### Example
```
    QAndroidJniObject::callStaticMethod("toString", "()Ljava/lang/String"); // Should be '()Ljava/lang/String;'
```

#### Pitfalls
For this check to work you need to set the ANDROID_NDK environment variable to your Android NDK installation.
