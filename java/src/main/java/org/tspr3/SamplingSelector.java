package org.tspr3;
import java.util.Arrays;
import java.util.Objects;
import org.tspr3.Tspr3.Algorithm;
final class SamplingSelector {
    private SamplingSelector() { }
    static Algorithm choose(int[] input) {
        Objects.requireNonNull(input,"input");
        int m=input.length/3+(input.length%3==0?0:1),s=Math.min(512,m);
        if(input.length<256) return Algorithm.A;
        int[] sample=new int[s*3];int used=0,separated=0,previousLow=0,previousHigh=0;
        for(int i=0;i<s;i++) {
            int block=i*(m/s)+(int)((long)i*(m%s)/s),start=block*3;
            int end=start+Math.min(3,input.length-start),low=input[start],high=low;
            for(int j=start;j<end;j++) { int value=input[j];sample[used++]=value;low=Math.min(low,value);high=Math.max(high,value); }
            if(i>0 && (previousHigh<low || high<previousLow)) separated++;
            previousLow=low;previousHigh=high;
        }
        if(separated>0.85*(s-1)) return Algorithm.A;
        Arrays.sort(sample,0,used);int unique=1;
        for(int i=1;i<used;i++) if(sample[i]!=sample[i-1]) unique++;
        return 1-(double)unique/used>=0.90 ? Algorithm.B : Algorithm.A;

    }
}
