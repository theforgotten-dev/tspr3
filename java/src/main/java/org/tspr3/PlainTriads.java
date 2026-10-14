package org.tspr3;
import static org.tspr3.TriadSupport.local;
final class PlainTriads implements OrderingStrategy {
    @Override public int[] sort(int[] input) {
        int n=input.length;int[] result=input.clone(),scratch=new int[n],src=result,dst=scratch;
        for(int start=0;start<n;) { int count=Math.min(3,n-start);local(result,start,count);start+=count; }
        for(int width=3;width<n;) {
            for(int start=0;start<n;) {
                int mid=start+Math.min(width,n-start),end=mid+Math.min(width,n-mid);
                if(mid==end || src[mid-1]<=src[mid]) System.arraycopy(src,start,dst,start,end-start);
                else if(src[end-1]<src[start]) {
                    System.arraycopy(src,mid,dst,start,end-mid);System.arraycopy(src,start,dst,start+end-mid,mid-start);
                } else {
                    int l=start,r=mid,k=start;
                    while(l<mid && r<end) dst[k++]=src[l]<=src[r]?src[l++]:src[r++];
                    System.arraycopy(src,l,dst,k,mid-l);k+=mid-l;System.arraycopy(src,r,dst,k,end-r);
                }
                start=end;
            }
            int[] t=src;src=dst;dst=t;if(width>n/2) break;width*=2;
        }
        if(src!=result) System.arraycopy(src,0,result,0,n);return result;

    }
}
