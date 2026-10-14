package org.tspr3;
import java.util.Arrays;
import java.util.Random;
public final class Tspr3Test {
    private static int cases;
    private static void verify(int[] input,boolean nativePath) {
        int[] saved=input.clone(),expected=input.clone();Arrays.sort(expected);
        for(Tspr3.Algorithm mode:Tspr3.Algorithm.values()) {
            if(!Arrays.equals(Tspr3.sort(input,mode),expected)) throw new AssertionError("Java "+mode);
            if(nativePath && !Arrays.equals(NativeTspr3.sort(input,mode),expected)) throw new AssertionError("JNI "+mode);
            if(mode!=Tspr3.Algorithm.A) {
                Tspr3.Frequencies f=Tspr3.frequencies(input,mode);int at=0;
                for(int i=0;i<f.values.length;i++) {
                    int begin=at;while(at<expected.length && expected[at]==f.values[i]) at++;
                    if(at==begin || f.counts[i]!=at-begin) throw new AssertionError("counts");
                }
                if(at!=expected.length) throw new AssertionError("missing keys");
            }
        }
        if(!Arrays.equals(input,saved)) throw new AssertionError("input changed");cases++;
    }
    public static void main(String[] args) {
        boolean nativePath=Arrays.asList(args).contains("--native");
        int[] values={-2,0,1,3};
        for(int n=0,total=1;n<=7;n++,total*=4) for(int code=0;code<total;code++) {
            int[] a=new int[n];int v=code;for(int i=0;i<n;i++) { a[i]=values[v%4];v/=4; }verify(a,nativePath);
        }
        Random random=new Random(20261013L);
        for(int n=0;n<=300;n++) for(int repeat=0;repeat<5;repeat++) {
            int[] a=new int[n];for(int i=0;i<n;i++) a[i]=random.nextInt();verify(a,nativePath);
        }
        verify(new int[]{Integer.MIN_VALUE,Integer.MAX_VALUE,0,-1,Integer.MIN_VALUE,Integer.MAX_VALUE},nativePath);
        int[] a=new int[10000];for(int i=0;i<a.length;i++) a[i]=(i%3==0)?i:-i;verify(a,nativePath);
        boolean rejected=false;try { Tspr3.sort(null); } catch(NullPointerException expected) { rejected=true; }
        if(!rejected) throw new AssertionError("null input");
        rejected=false;try { Tspr3.frequencies(a,Tspr3.Algorithm.A); } catch(IllegalArgumentException expected) { rejected=true; }
        if(!rejected) throw new AssertionError("A counts");
        Arrays.fill(a,0);for(int i=0;i<a.length;i+=100) a[i]=i;
        if(Tspr3.choose(a)!=Tspr3.Algorithm.B) throw new AssertionError("dominant repeats should not be separated ranges");
        for(int i=0;i<a.length;i++) a[i]=i;
        if(Tspr3.choose(a)!=Tspr3.Algorithm.A) throw new AssertionError("ordered ranges");
        if(Arrays.asList(args).contains("--large")) {
            for(int scenario=0;scenario<3;scenario++) {
                int[] large=new int[2000000];int center=0;
                for(int i=0;i<large.length;i++) {
                    if(scenario==0) large[i]=random.nextInt();
                    else if(scenario==1) large[i]=random.nextInt(8);
                    else { if(i%3==0) center=random.nextInt(2000001)-1000000;large[i]=center+random.nextInt(5)-2; }
                }
                verify(large,nativePath);
            }
        }
        System.out.println("OK: "+cases+" inputs; Java A/B/C/AUTO, counts, immutable input"+(nativePath?" and JNI":""));
    }
}
