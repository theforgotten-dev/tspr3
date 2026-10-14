package org.tspr3;
import java.util.Arrays;
import static org.tspr3.TriadSupport.local;
final class CompactTriads implements FrequencyStrategy {
    private final boolean binary;
    CompactTriads(boolean binary) { this.binary=binary; }
    @Override public int[] sort(int[] input) {
        Tspr3.Frequencies f=frequencies(input);int[] result=new int[input.length];int at=0;
        for(int i=0;i<f.values.length;i++) { Arrays.fill(result,at,at+f.counts[i],f.values[i]);at+=f.counts[i]; }
        return result;
    }
    @Override public Tspr3.Frequencies frequencies(int[] input) {
        if(input.length==0) return new Tspr3.Frequencies(new int[0],new int[0]);
        Work w=new Work(input,binary);int length;
        if(binary) { w.layout(0);w.swap();length=w.resolve(0); } else length=w.group(0);
        return new Tspr3.Frequencies(Arrays.copyOf(w.av,length),Arrays.copyOf(w.ac,length));
    }
    private static final class Work {
        int[] av,ac,bv,bc,offset,length,next;
        int[] prefix,bits,left,right,head,tail,start;
        int nodes,capacity,cursor;
        Work(int[] input,boolean binary) {
            int n=input.length,m=n/3+(n%3==0?0:1);
            av=new int[n];ac=new int[n];bv=new int[n];bc=new int[n];
            offset=new int[m];length=new int[m];next=new int[m];Arrays.fill(next,-1);
            if(binary) node(0,0);
            int[] v=new int[3];int at=0;
            for(int i=0;i<m;i++) {
                int position=i*3,count=Math.min(3,n-position);System.arraycopy(input,position,v,0,count);local(v,0,count);
                offset[i]=at;
                for(int j=0;j<count;j++) {
                    if(j>0 && v[j]==v[j-1]) ac[at-1]++;
                    else { av[at]=v[j];ac[at++]=1;length[i]++; }
                }
                if(binary) {
                    int low=v[0]^Integer.MIN_VALUE,high=v[count-1]^Integer.MIN_VALUE;
                    int common=Integer.numberOfLeadingZeros(low^high);insert(i,masked(low,common),common);
                } else if(i>0) next[i-1]=i;
            }
        }
        static int masked(int key,int length) { return length==0?0:key&(-1 << (32-length)); }
        static int side(int key,int length) { return (key >>> (31-length))&1; }
        int node(int key,int lengthBits) {
            if(nodes==capacity) {
                int c=capacity==0?16:Math.multiplyExact(capacity,2);
                prefix=copy(prefix,c);bits=copy(bits,c);left=copy(left,c);right=copy(right,c);
                head=copy(head,c);tail=copy(tail,c);start=copy(start,c);capacity=c;
            }
            int id=nodes++;prefix[id]=masked(key,lengthBits);bits[id]=lengthBits;
            left[id]=right[id]=head[id]=tail[id]=-1;return id;
        }
        static int[] copy(int[] source,int n) { return source==null?new int[n]:Arrays.copyOf(source,n); }
        void attach(int node,int block) { if(head[node]<0) head[node]=block;else next[tail[node]]=block;tail[node]=block; }
        void insert(int block,int key,int lengthBits) {
            int at=0;
            while(true) {
                if(lengthBits==bits[at]) { attach(at,block);return; }
                int s=side(key,bits[at]),child=s==0?left[at]:right[at];
                if(child<0) { int leaf=node(key,lengthBits);if(s==0) left[at]=leaf;else right[at]=leaf;attach(leaf,block);return; }
                int common=Math.min(Integer.numberOfLeadingZeros(key^prefix[child]),Math.min(lengthBits,bits[child]));
                if(common==bits[child]) { at=child;continue; }
                int parent=node(key,common);if(s==0) left[at]=parent;else right[at]=parent;
                if(side(prefix[child],common)==0) left[parent]=child;else right[parent]=child;
                if(common==lengthBits) { attach(parent,block);return; }at=parent;
            }
        }
        int merge(int[] sv,int[] sc,int lo,int nl,int ro,int nr,int[] dv,int[] dc,int out) {
            int l=lo,r=ro,k=out,le=lo+nl,re=ro+nr;
            if(nl==0 || nr==0 || sv[le-1]<sv[ro]) {
                System.arraycopy(sv,lo,dv,out,nl);System.arraycopy(sc,lo,dc,out,nl);
                System.arraycopy(sv,ro,dv,out+nl,nr);System.arraycopy(sc,ro,dc,out+nl,nr);return nl+nr;
            }
            if(sv[re-1]<sv[lo]) {
                System.arraycopy(sv,ro,dv,out,nr);System.arraycopy(sc,ro,dc,out,nr);
                System.arraycopy(sv,lo,dv,out+nr,nl);System.arraycopy(sc,lo,dc,out+nr,nl);return nl+nr;
            }
            while(l<le && r<re) {
                if(sv[l]<sv[r]) { dv[k]=sv[l];dc[k++]=sc[l++]; }
                else if(sv[l]>sv[r]) { dv[k]=sv[r];dc[k++]=sc[r++]; }
                else { dv[k]=sv[l];dc[k++]=sc[l++]+sc[r++]; }
            }
            System.arraycopy(sv,l,dv,k,le-l);System.arraycopy(sc,l,dc,k,le-l);k+=le-l;
            System.arraycopy(sv,r,dv,k,re-r);System.arraycopy(sc,r,dc,k,re-r);return k+re-r-out;
        }
        int group(int first) {
            if(first<0) return 0;
            int[] sv=av,sc=ac,dv=bv,dc=bc;
            while(next[first]>=0) {
                int block=first,newFirst=-1,last=-1;
                while(block>=0) {
                    int other=next[block],following;
                    if(other>=0) { following=next[other];length[block]=merge(sv,sc,offset[block],length[block],offset[other],length[other],dv,dc,offset[block]); }
                    else { following=-1;System.arraycopy(sv,offset[block],dv,offset[block],length[block]);System.arraycopy(sc,offset[block],dc,offset[block],length[block]); }
                    if(newFirst<0) newFirst=block;else next[last]=block;last=block;block=following;
                }
                next[last]=-1;first=newFirst;int[] t=sv;sv=dv;dv=t;t=sc;sc=dc;dc=t;
            }
            if(sv!=av) { System.arraycopy(sv,offset[first],av,offset[first],length[first]);System.arraycopy(sc,offset[first],ac,offset[first],length[first]); }
            return length[first];
        }
        void swap() { int[] t=av;av=bv;bv=t;t=ac;ac=bc;bc=t; }
        void layout(int node) {
            start[node]=cursor;
            for(int block=head[node];block>=0;block=next[block]) {
                System.arraycopy(av,offset[block],bv,cursor,length[block]);System.arraycopy(ac,offset[block],bc,cursor,length[block]);
                offset[block]=cursor;cursor+=length[block];
            }
            if(left[node]>=0) layout(left[node]);if(right[node]>=0) layout(right[node]);
        }
        int resolve(int node) {
            int l=left[node],r=right[node],nl=l<0?0:resolve(l),nr=r<0?0:resolve(r);
            int own=group(head[node]),total=own;
            for(int s=0;s<2;s++) {
                int child=s==0?l:r,lengthChild=s==0?nl:nr;if(lengthChild==0) continue;
                if(own==0) {
                    System.arraycopy(av,start[child],av,start[node]+total,lengthChild);
                    System.arraycopy(ac,start[child],ac,start[node]+total,lengthChild);total+=lengthChild;
                } else {
                    total=merge(av,ac,start[node],total,start[child],lengthChild,bv,bc,start[node]);
                    System.arraycopy(bv,start[node],av,start[node],total);System.arraycopy(bc,start[node],ac,start[node],total);
                }
            }
            return total;
        }
    }
}
