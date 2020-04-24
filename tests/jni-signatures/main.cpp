
#define jclass int
#define jint int
#define jstring int

// Crude approximation of #include <QAndroidJniObject> so we don't need QtAndroidExtras installed
class QAndroidJniObject
{
    public:
    QAndroidJniObject(const char *className, const char *signature);
    QAndroidJniObject(jclass theClass, const char *signature);

    template <typename T>
    void callObjectMethod(const char *methodName, const char *signature);

    template <typename T>
    void callMethod(const char *methodName, const char *signature);

    template <typename T>
    static void callStaticMethod(const char *className, const char *methodName, const char *signature);

    template <typename T>
    static void callStaticObjectMethod(const char *className, const char *methodName, const char *signature);

};

namespace QtAndroid {
    void androidActivity();
};

int main() {


    QAndroidJniObject obj("/java/lang/String", "(");

    jclass foo = 1;
    QAndroidJniObject obj2(foo, "()V");

    obj.callObjectMethod<int>("someMethod", "()V");
    obj.callObjectMethod<int>("someMethod", "(III)V");
    obj.callObjectMethod<int>("someMethod", "(III");

    obj.callMethod<jint>("someMethod", "()V");
    obj.callMethod<jint>("someMethod", "(III)V");
    obj.callMethod<jint>("someMethod", "()Ljava/lang/String");

    QAndroidJniObject::callStaticMethod<jint>("someClass;", "someMethod", "()Ljava/lang/String");

    QAndroidJniObject::callStaticObjectMethod<int>("someClass", "someOtherMethod", "(II)");

    obj.callObjectMethod<jstring>("className;", "toString");

    QtAndroid::androidActivity();
}
