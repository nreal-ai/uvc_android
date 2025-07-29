#include <jni.h>
#include <string>

extern "C" JNIEXPORT jstring JNICALL
Java_com_xreal_uvctest_NativeLib_stringFromJNI(
        JNIEnv* env,
        jobject /* this */) {
    std::string hello = "Hello from C++";
    return env->NewStringUTF(hello.c_str());
}


int main(int argc, char** argv) {
    return 1;
}