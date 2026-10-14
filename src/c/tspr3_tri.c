#include "tspr3_tri.h"
#include <stdlib.h>
#include <string.h>

#define NONE SIZE_MAX
typedef struct { size_t offset, length, next; } block;
typedef struct {
    uint32_t prefix;
    unsigned int bits;
    size_t child[2], head, tail, start, end;
} prefix_node;
typedef struct {
    tspr3_tri_stats stats;
    size_t live;
    tspr3_frequency *a, *b;
    block *blocks;
    prefix_node *nodes;
    size_t used, capacity;
} work;

size_t tspr3_tri_memory_bound(size_t n,tspr3_tri_mode mode) {
    size_t m=n/3+(n%3!=0),base,capacity=16,nodes;
    if(mode<TSPR3_TRI_BASE || mode>TSPR3_TRI_BIN) return SIZE_MAX;
    if(!n) return 0;
    if(mode==TSPR3_TRI_BASE) return n>SIZE_MAX/sizeof(int32_t) ? SIZE_MAX : n*sizeof(int32_t);
    if(n>SIZE_MAX/(2*sizeof(tspr3_frequency))) return SIZE_MAX;
    base=2*n*sizeof(tspr3_frequency);
    if(m>(SIZE_MAX-base)/sizeof(block)) return SIZE_MAX;
    base+=m*sizeof(block);
    if(mode==TSPR3_TRI_COUNTS) return base;
    if(m>(SIZE_MAX-1)/2) return SIZE_MAX;
    nodes=2*m+1;
    while(capacity<nodes) { if(capacity>SIZE_MAX/2) return SIZE_MAX;capacity*=2; }
    if(capacity>SIZE_MAX/sizeof(prefix_node)) return SIZE_MAX;
    nodes=capacity*sizeof(prefix_node);
    if(nodes>SIZE_MAX-base || nodes/2>SIZE_MAX-base-nodes) return SIZE_MAX;
    return base+nodes+nodes/2;
}

static void *reserve(work *w, size_t count, size_t size) {
    void *p;
    if (count > SIZE_MAX / size || count*size > SIZE_MAX-w->live) return NULL;
    p=malloc(count*size);
    if (p) {
        w->live+=count*size;
        if(w->live>w->stats.peak_aux_bytes) w->stats.peak_aux_bytes=w->live;
    }
    return p;
}
static void release(work *w, void *p, size_t bytes) {
    if(p) { free(p);w->live-=bytes; }
}
static int key_compare(work *w, int32_t a, int32_t b) {
    ++w->stats.comparisons;
    return (a>b)-(a<b);
}
static void local_sort(work *w, int32_t *v, size_t count) {
    if(count>1 && key_compare(w,v[0],v[1])>0) { int32_t t=v[0];v[0]=v[1];v[1]=t; }
    if(count>2) {
        if(key_compare(w,v[1],v[2])>0) { int32_t t=v[1];v[1]=v[2];v[2]=t; }
        if(key_compare(w,v[0],v[1])>0) { int32_t t=v[0];v[0]=v[1];v[1]=t; }
    }
}
static uint32_t numeric_key(int32_t x) { return (uint32_t)x ^ UINT32_C(0x80000000); }
static unsigned int leading_zeroes(uint32_t x) {
    unsigned int n=0;
    if(!x) return 32;
    if(!(x & UINT32_C(0xffff0000))) { n+=16;x<<=16; }
    if(!(x & UINT32_C(0xff000000))) { n+=8;x<<=8; }
    if(!(x & UINT32_C(0xf0000000))) { n+=4;x<<=4; }
    if(!(x & UINT32_C(0xc0000000))) { n+=2;x<<=2; }
    if(!(x & UINT32_C(0x80000000))) ++n;
    return n;
}
static uint32_t prefix_of(uint32_t key, unsigned int bits) {
    return bits ? key & (UINT32_MAX << (32-bits)) : 0;
}
static unsigned int side_of(uint32_t key, unsigned int bits) {
    return (unsigned int)((key >> (31-bits)) & 1);
}
static size_t new_node(work *w, uint32_t prefix, unsigned int bits) {
    size_t id;
    if(w->used==w->capacity) {
        size_t capacity=w->capacity ? w->capacity*2 : 16;
        prefix_node *grown;
        if(w->capacity>SIZE_MAX/2 || capacity>SIZE_MAX/sizeof(*grown)) return NONE;
        grown=reserve(w,capacity,sizeof(*grown));
        if(!grown) return NONE;
        if(w->used) memcpy(grown,w->nodes,w->used*sizeof(*grown));
        release(w,w->nodes,w->capacity*sizeof(*grown));
        w->nodes=grown;w->capacity=capacity;
    }
    id=w->used++;
    w->nodes[id].prefix=prefix_of(prefix,bits);w->nodes[id].bits=bits;
    w->nodes[id].child[0]=w->nodes[id].child[1]=NONE;
    w->nodes[id].head=w->nodes[id].tail=NONE;
    w->nodes[id].start=w->nodes[id].end=0;
    return id;
}
static void attach(work *w, size_t node, size_t id) {
    if(w->nodes[node].head==NONE) w->nodes[node].head=id;
    else w->blocks[w->nodes[node].tail].next=id;
    w->nodes[node].tail=id;
}
static int insert_block(work *w, size_t id, uint32_t prefix, unsigned int bits) {
    size_t at=0;
    for(;;) {
        size_t child;unsigned int side,common;
        ++w->stats.prefix_tests;
        if(bits==w->nodes[at].bits) { attach(w,at,id);return 1; }
        side=side_of(prefix,w->nodes[at].bits);
        child=w->nodes[at].child[side];
        if(child==NONE) {
            size_t leaf=new_node(w,prefix,bits);
            if(leaf==NONE) return 0;
            w->nodes[at].child[side]=leaf;attach(w,leaf,id);return 1;
        }
        common=leading_zeroes(prefix ^ w->nodes[child].prefix);
        if(common>bits) common=bits;
        if(common>w->nodes[child].bits) common=w->nodes[child].bits;
        if(common==w->nodes[child].bits) { at=child;continue; }
        {
            size_t parent=new_node(w,prefix,common);
            if(parent==NONE) return 0;
            w->nodes[at].child[side]=parent;
            w->nodes[parent].child[side_of(w->nodes[child].prefix,common)]=child;
            if(common==bits) { attach(w,parent,id);return 1; }
            at=parent;
        }
    }
}

/* Both modes below use the same extrema checks and compact merge. */
static size_t merge_counts(work *w, const tspr3_frequency *left, size_t nl,
    const tspr3_frequency *right, size_t nr, tspr3_frequency *out) {
    size_t l=0,r=0,k=0;
    if(!nl || !nr) {
        if(nl) memcpy(out,left,nl*sizeof(*out));
        if(nr) memcpy(out,right,nr*sizeof(*out));
        k=nl+nr;
    } else if(key_compare(w,left[nl-1].value,right[0].value)<0) {
        memcpy(out,left,nl*sizeof(*out));memcpy(out+nl,right,nr*sizeof(*out));
        k=nl+nr;++w->stats.range_joins;
    } else if(key_compare(w,right[nr-1].value,left[0].value)<0) {
        memcpy(out,right,nr*sizeof(*out));memcpy(out+nr,left,nl*sizeof(*out));
        k=nl+nr;++w->stats.range_joins;
    } else {
        while(l<nl && r<nr) {
            int cmp=key_compare(w,left[l].value,right[r].value);
            if(cmp<0) out[k++]=left[l++];
            else if(cmp>0) out[k++]=right[r++];
            else {
                out[k]=left[l++];out[k++].count+=right[r++].count;
                ++w->stats.equal_merges;
            }
        }
        while(l<nl) out[k++]=left[l++];
        while(r<nr) out[k++]=right[r++];
    }
    w->stats.data_bytes_written+=(uint64_t)k*sizeof(*out);
    return k;
}
static size_t merge_group(work *w, size_t head) {
    tspr3_frequency *src=w->a,*dst=w->b;
    if(head==NONE) return 0;
    while(w->blocks[head].next!=NONE) {
        size_t left=head,new_head=NONE,tail=NONE;
        while(left!=NONE) {
            size_t right=w->blocks[left].next,next;
            if(right!=NONE) {
                next=w->blocks[right].next;
                w->blocks[left].length=merge_counts(w,src+w->blocks[left].offset,w->blocks[left].length,
                    src+w->blocks[right].offset,w->blocks[right].length,dst+w->blocks[left].offset);
            } else {
                next=NONE;
                memcpy(dst+w->blocks[left].offset,src+w->blocks[left].offset,w->blocks[left].length*sizeof(*dst));
                w->stats.data_bytes_written+=(uint64_t)w->blocks[left].length*sizeof(*dst);
            }
            if(new_head==NONE) new_head=left;
            else w->blocks[tail].next=left;
            tail=left;left=next;
        }
        w->blocks[tail].next=NONE;head=new_head;
        { tspr3_frequency *tmp=src;src=dst;dst=tmp; }
    }
    if(src!=w->a) {
        memcpy(w->a+w->blocks[head].offset,src+w->blocks[head].offset,w->blocks[head].length*sizeof(*src));
        w->stats.data_bytes_written+=(uint64_t)w->blocks[head].length*sizeof(*src);
    }
    return w->blocks[head].length;
}
/* Preorder layout makes each node subtree a disjoint contiguous arena. */
static void layout(work *w, size_t node, size_t *cursor) {
    size_t id;
    w->nodes[node].start=*cursor;
    for(id=w->nodes[node].head;id!=NONE;id=w->blocks[id].next) {
        size_t length=w->blocks[id].length;
        memcpy(w->b+*cursor,w->a+w->blocks[id].offset,length*sizeof(*w->a));
        w->stats.data_bytes_written+=(uint64_t)length*sizeof(*w->a);
        w->blocks[id].offset=*cursor;*cursor+=length;
    }
    if(w->nodes[node].child[0]!=NONE) layout(w,w->nodes[node].child[0],cursor);
    if(w->nodes[node].child[1]!=NONE) layout(w,w->nodes[node].child[1],cursor);
    w->nodes[node].end=*cursor;
}
/* Ancestor-owned crossing blocks are merged, never sent to a single child.
 * Only disjoint child prefixes permit a comparison-free concatenation. */
static size_t resolve(work *w, size_t node) {
    size_t child[2]={w->nodes[node].child[0],w->nodes[node].child[1]};
    size_t lengths[2]={0,0},local,total,start=w->nodes[node].start;
    unsigned int side;
    for(side=0;side<2;++side) if(child[side]!=NONE) lengths[side]=resolve(w,child[side]);
    local=merge_group(w,w->nodes[node].head);total=local;
    for(side=0;side<2;++side) if(lengths[side]) {
        size_t offset=w->nodes[child[side]].start;
        if(!local) {
            if(offset!=start+total) {
                memmove(w->a+start+total,w->a+offset,lengths[side]*sizeof(*w->a));
                w->stats.data_bytes_written+=(uint64_t)lengths[side]*sizeof(*w->a);
            }
            if(total) ++w->stats.prefix_joins;
            total+=lengths[side];
        } else {
            total=merge_counts(w,w->a+start,total,w->a+offset,lengths[side],w->b+start);
            memcpy(w->a+start,w->b+start,total*sizeof(*w->a));
            w->stats.data_bytes_written+=(uint64_t)total*sizeof(*w->a);
        }
    }
    return total;
}

static tspr3_status compact(const int32_t *input, size_t n, tspr3_tri_mode mode,
    tspr3_frequency **result, size_t *distinct, tspr3_tri_stats *stats) {
    work w={0};size_t m=n/3+(n%3!=0),i,cursor=0,k=0;
    tspr3_status status=TSPR3_NO_MEMORY;
    *result=NULL;*distinct=0;
    w.stats.triads=m;
    w.a=reserve(&w,n,sizeof(*w.a));w.b=reserve(&w,n,sizeof(*w.b));
    w.blocks=reserve(&w,m,sizeof(*w.blocks));
    if(!w.a || !w.b || !w.blocks) goto done;
    if(mode==TSPR3_TRI_BIN && new_node(&w,0,0)==NONE) goto done;
    for(i=0;i<m;++i) {
        int32_t v[3];size_t start=i*3,count=n-start,j,length=0;
        if(count>3) count=3;
        for(j=0;j<count;++j) v[j]=input[start+j];
        local_sort(&w,v,count);
        w.blocks[i].offset=cursor;w.blocks[i].next=NONE;
        for(j=0;j<count;++j) {
            if(j && key_compare(&w,v[j-1],v[j])==0) ++w.a[cursor-1].count;
            else { w.a[cursor].value=v[j];w.a[cursor++].count=1;++length; }
        }
        w.stats.data_bytes_written+=(uint64_t)length*sizeof(*w.a)+(uint64_t)(count-length)*sizeof(size_t);
        w.blocks[i].length=length;
        if(mode==TSPR3_TRI_BIN) {
            uint32_t low=numeric_key(v[0]),high=numeric_key(v[count-1]);
            unsigned int bits=leading_zeroes(low^high);
            ++w.stats.prefix_tests;
            if(!bits) ++w.stats.crossing_root_blocks;
            if(!insert_block(&w,i,prefix_of(low,bits),bits)) goto done;
        } else if(i) w.blocks[i-1].next=i;
    }
    if(mode==TSPR3_TRI_BIN) {
        size_t end=0;
        layout(&w,0,&end);
        { tspr3_frequency *tmp=w.a;w.a=w.b;w.b=tmp; }
        k=resolve(&w,0);
        w.stats.prefix_nodes=w.used;
    } else k=merge_group(&w,0);
    w.stats.distinct_values=k;
    *result=w.a;*distinct=k;w.a=NULL;status=TSPR3_OK;
done:
    if(stats) *stats=w.stats;
    release(&w,w.a,n*sizeof(*w.a));release(&w,w.b,n*sizeof(*w.b));
    release(&w,w.blocks,m*sizeof(*w.blocks));release(&w,w.nodes,w.capacity*sizeof(*w.nodes));
    return status;
}

static tspr3_status base(const int32_t *input,size_t n,int32_t *output,tspr3_tri_stats *stats) {
    work w={0};size_t start,width;int32_t *scratch,*src,*dst;
    scratch=reserve(&w,n,sizeof(*scratch));
    if(!scratch) return TSPR3_NO_MEMORY;
    memcpy(output,input,n*sizeof(*output));w.stats.data_bytes_written=(uint64_t)n*sizeof(*output);
    w.stats.triads=n/3+(n%3!=0);
    for(start=0;start<n;) {
        size_t count=n-start,j;int32_t v[3];
        if(count>3) count=3;
        for(j=0;j<count;++j) v[j]=output[start+j];
        local_sort(&w,v,count);
        for(j=0;j<count;++j) output[start+j]=v[j];
        w.stats.data_bytes_written+=(uint64_t)count*sizeof(*output);start+=count;
    }
    src=output;dst=scratch;
    for(width=3;width<n;) {
        for(start=0;start<n;) {
            size_t mid=start+(n-start<width ? n-start : width),end=mid+(n-mid<width ? n-mid : width);
            size_t l=start,r=mid,pos=start;
            if(mid==end || key_compare(&w,src[mid-1],src[mid])<=0) {
                memcpy(dst+start,src+start,(end-start)*sizeof(*dst));
                if(mid!=end) ++w.stats.range_joins;
            } else if(key_compare(&w,src[end-1],src[start])<0) {
                memcpy(dst+start,src+mid,(end-mid)*sizeof(*dst));
                memcpy(dst+start+end-mid,src+start,(mid-start)*sizeof(*dst));++w.stats.range_joins;
            } else {
                while(l<mid && r<end) dst[pos++]=key_compare(&w,src[l],src[r])<=0 ? src[l++] : src[r++];
                while(l<mid) dst[pos++]=src[l++];
                while(r<end) dst[pos++]=src[r++];
            }
            w.stats.data_bytes_written+=(uint64_t)(end-start)*sizeof(*dst);start=end;
        }
        { int32_t *tmp=src;src=dst;dst=tmp; }
        if(width>n/2) break;
        width*=2;
    }
    if(src!=output) { memcpy(output,src,n*sizeof(*output));w.stats.data_bytes_written+=(uint64_t)n*sizeof(*output); }
    if(stats) *stats=w.stats;
    release(&w,scratch,n*sizeof(*scratch));return TSPR3_OK;
}
/* Strategy registry: validation and selection stay outside implementations. */
static tspr3_status sort_compact(const int32_t *input,size_t n,tspr3_tri_mode mode,
    int32_t *output,tspr3_tri_stats *stats) {
    tspr3_frequency *index;size_t k,i,j,pos=0;tspr3_status status;
    tspr3_tri_stats measured={0};
    status=compact(input,n,mode,&index,&k,&measured);
    if(status==TSPR3_OK) {
        for(i=0;i<k;++i) for(j=0;j<index[i].count;++j) output[pos++]=index[i].value;
        measured.data_bytes_written+=(uint64_t)n*sizeof(*output);free(index);
    }
    if(stats) *stats=measured;return status;
}
static tspr3_status sort_counts(const int32_t *in,size_t n,int32_t *out,tspr3_tri_stats *stats) {
    return sort_compact(in,n,TSPR3_TRI_COUNTS,out,stats);
}
static tspr3_status sort_binary(const int32_t *in,size_t n,int32_t *out,tspr3_tri_stats *stats) {
    return sort_compact(in,n,TSPR3_TRI_BIN,out,stats);
}
static tspr3_status index_counts(const int32_t *in,size_t n,tspr3_frequency **out,size_t *k,tspr3_tri_stats *stats) {
    return compact(in,n,TSPR3_TRI_COUNTS,out,k,stats);
}
static tspr3_status index_binary(const int32_t *in,size_t n,tspr3_frequency **out,size_t *k,tspr3_tri_stats *stats) {
    return compact(in,n,TSPR3_TRI_BIN,out,k,stats);
}
typedef struct {
    tspr3_status (*sort)(const int32_t *,size_t,int32_t *,tspr3_tri_stats *);
    tspr3_status (*index)(const int32_t *,size_t,tspr3_frequency **,size_t *,tspr3_tri_stats *);
} tri_strategy;
static const tri_strategy STRATEGIES[3]={{base,NULL},{sort_counts,index_counts},{sort_binary,index_binary}};

tspr3_status tspr3_tri_index(const int32_t *input,size_t n,tspr3_tri_mode mode,
    tspr3_frequency **index,size_t *distinct_count,tspr3_tri_stats *stats) {
    if(stats) memset(stats,0,sizeof(*stats));
    if(!index || !distinct_count) return TSPR3_INVALID;
    *index=NULL;*distinct_count=0;
    if(mode<TSPR3_TRI_BASE || mode>TSPR3_TRI_BIN || !STRATEGIES[mode].index) return TSPR3_INVALID;
    if(!n) return TSPR3_OK;
    if(!input || n>SIZE_MAX/sizeof(tspr3_frequency)) return TSPR3_INVALID;
    return STRATEGIES[mode].index(input,n,index,distinct_count,stats);
}
tspr3_status tspr3_tri_sort(const int32_t *input,size_t n,tspr3_tri_mode mode,
    int32_t *output,tspr3_tri_stats *stats) {
    if(stats) memset(stats,0,sizeof(*stats));
    if(mode<TSPR3_TRI_BASE || mode>TSPR3_TRI_BIN) return TSPR3_INVALID;
    if(!n) return TSPR3_OK;
    if(!input || !output || n>SIZE_MAX/sizeof(tspr3_frequency)) return TSPR3_INVALID;
    return STRATEGIES[mode].sort(input,n,output,stats);
}
