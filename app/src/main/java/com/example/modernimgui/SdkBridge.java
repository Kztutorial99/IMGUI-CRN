package com.example.modernimgui;

/**
 * Minimal Java-side load check for hosts that use System.loadLibrary("Sdk").
 * Rendering still has to be driven by the host's OpenGL render loop through
 * the native plugin API.
 */
public final class SdkBridge {
    private SdkBridge() {
    }

    public static void load() {
        System.loadLibrary("Sdk");
    }
}