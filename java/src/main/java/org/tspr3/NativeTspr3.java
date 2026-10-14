package org.tspr3;
import java.util.Objects;
/** Optional JNI bridge to the existing C library. Never silently substitutes a path. */
public final class NativeTspr3 {
    private NativeTspr3() { }
    private static final class Loader { static { System.loadLibrary("tspr3_jni"); } static void ensure() { } }
    /** Sorts a copy through the optional native library.
     * @param input original integers, preserved
     * @param algorithm native AUTO or explicit A/B/C
     * @return sorted copy
     */
    public static int[] sort(int[] input,Tspr3.Algorithm algorithm) {
        Objects.requireNonNull(input,"input");Objects.requireNonNull(algorithm,"algorithm");
        Loader.ensure();return sortNative(input,algorithm.ordinal(),256L*1024*1024);
    }
    private static native int[] sortNative(int[] input,int algorithm,long heapLimit);
}
