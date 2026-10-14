package org.tspr3;
final class TriadSupport {
    private TriadSupport() { }
    static void local(int[] values,int start,int count) {
        if(count>1 && values[start]>values[start+1]) exchange(values,start,start+1);
        if(count>2) {
            if(values[start+1]>values[start+2]) exchange(values,start+1,start+2);
            if(values[start]>values[start+1]) exchange(values,start,start+1);
        }

    }
    private static void exchange(int[] a,int i,int j) { int t=a[i];a[i]=a[j];a[j]=t; }
}
