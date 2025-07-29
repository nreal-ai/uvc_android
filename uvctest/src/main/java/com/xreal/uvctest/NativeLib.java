package com.xreal.uvctest;

public class NativeLib {

    // Used to load the 'uvctest' library on application startup.
    static {
        System.loadLibrary("uvctest");
    }

    /**
     * A native method that is implemented by the 'uvctest' native library,
     * which is packaged with this application.
     */
    public native String stringFromJNI();
}