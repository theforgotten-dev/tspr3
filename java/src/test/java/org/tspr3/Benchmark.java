package org.tspr3;
import java.io.PrintWriter;
import java.util.Arrays;
import java.util.Locale;
import java.util.Random;
public final class Benchmark {
    private static final int N=2000000,REPEATS=5;
    private static final String[] CASES={"wide","bounded","ascending","descending","k8","k2048","skew99","normal","local_normal"};
    private static final String[] METHODS={"JAVA_A","JAVA_B","JAVA_C","JAVA_AUTO","JNI_A","JNI_B","JNI_C"};
    private static int[] generate(int scenario,long seed,int n) {
        Random r=new Random(seed);int[] a=new int[n];double center=0;
        for(int i=0;i<n;i++) {
            switch(scenario) {
            case 0:a[i]=r.nextInt();break;
            case 1:a[i]=r.nextInt(2000001)-1000000;break;
            case 2:a[i]=i;break;
            case 3:a[i]=n-i;break;
            case 4:a[i]=r.nextInt(8);break;
            case 5:a[i]=r.nextInt(2048);break;
            case 6:a[i]=r.nextInt(100)==0?r.nextInt():0;break;
            case 7:a[i]=(int)Math.round(r.nextGaussian()*1000000);break;
            default:if(i%3==0) center=Math.round(r.nextGaussian()*1000000);a[i]=(int)center+r.nextInt(5)-2;break;
            }
        }
        return a;
    }
    private static int[] run(int[] input,int method) {
        if(method<4) return Tspr3.sort(input,Tspr3.Algorithm.values()[method==3?0:method+1]);
        return NativeTspr3.sort(input,Tspr3.Algorithm.values()[method-3]);
    }
    public static void main(String[] args) throws Exception {
        System.out.println(System.getProperty("java.vm.name")+" "+System.getProperty("java.version"));
        if(args.length>0 && args[0].equals("--auto-only")) {
            for(int c:new int[]{0,4,6,8}) for(int repeat=0;repeat<3;repeat++) run(generate(c,20261013L+repeat,100000),3);
            try(PrintWriter raw=new PrintWriter("benchmarks/results/java_auto_final.csv","UTF-8")) {
                raw.println("scenario,n,seed,selected,ms");
                for(int c=0;c<CASES.length;c++) {
                    double[] times=new double[REPEATS];
                    for(int repeat=0;repeat<REPEATS;repeat++) {
                        long seed=20261013L+c*101L+repeat*7919L;int[] a=generate(c,seed,N),expected=a.clone();Arrays.sort(expected);
                        Tspr3.Algorithm selected=Tspr3.choose(a);long t=System.nanoTime();int[] out=Tspr3.sort(a);double ms=(System.nanoTime()-t)/1e6;
                        if(!Arrays.equals(out,expected)) throw new AssertionError(CASES[c]);
                        times[repeat]=ms;raw.printf(Locale.ROOT,"%s,%d,%d,%s,%.6f%n",CASES[c],N,seed,selected,ms);
                    }
                    Arrays.sort(times);System.out.printf(Locale.ROOT,"%-14s final Java AUTO=%.3f ms%n",CASES[c],times[2]);raw.flush();
                }
            }
            System.out.println("OK: 45 verified final AUTO sorts, 2,000,000 elements each.");return;
        }
        // Warm all implementations on several distributions; exploratory, not JMH.
        for(int c:new int[]{0,4,8}) for(int method=0;method<METHODS.length;method++)
            for(int repeat=0;repeat<3;repeat++) run(generate(c,20261013L+repeat,100000),method);
        try(PrintWriter raw=new PrintWriter("benchmarks/results/java_raw.csv","UTF-8");
            PrintWriter summary=new PrintWriter("benchmarks/results/java_summary.csv","UTF-8")) {
            raw.println("scenario,n,seed,method,ms");summary.println("scenario,n,method,median_ms,min_ms,max_ms");
            for(int c=0;c<CASES.length;c++) {
                double[][] times=new double[METHODS.length][REPEATS];
                for(int repeat=0;repeat<REPEATS;repeat++) {
                    long seed=20261013L+c*101L+repeat*7919L;int[] a=generate(c,seed,N),expected=a.clone();Arrays.sort(expected);
                    for(int slot=0;slot<METHODS.length;slot++) {
                        int method=(slot+repeat+c)%METHODS.length;
                        long t=System.nanoTime();int[] out=run(a,method);double ms=(System.nanoTime()-t)/1e6;
                        if(!Arrays.equals(expected,out)) throw new AssertionError(CASES[c]+" "+METHODS[method]);
                        times[method][repeat]=ms;raw.printf(Locale.ROOT,"%s,%d,%d,%s,%.6f%n",CASES[c],N,seed,METHODS[method],ms);
                    }
                }
                for(int method=0;method<METHODS.length;method++) {
                    Arrays.sort(times[method]);summary.printf(Locale.ROOT,"%s,%d,%s,%.6f,%.6f,%.6f%n",CASES[c],N,METHODS[method],times[method][2],times[method][0],times[method][4]);
                }
                System.out.printf(Locale.ROOT,"%-14s Java A/B/C=%.2f/%.2f/%.2f JNI A/B/C=%.2f/%.2f/%.2f ms%n",
                    CASES[c],times[0][2],times[1][2],times[2][2],times[4][2],times[5][2],times[6][2]);
                raw.flush();summary.flush();
            }
        }
        System.out.println("OK: 315 verified sorts, 2,000,000 elements each; JIT warm-up included before timing.");
    }
}
