#include "tspr3_generic_tri.h"
#include <stdlib.h>
#include <string.h>

/* Generic frequency: count (size_t) followed by elem_size bytes of value */
#define GFREQ_REC_SIZE(elem_size) (sizeof(size_t) + (elem_size))
#define GFREQ_COUNT(arr, i, rec_size) (*(size_t*)((arr) + (i) * (rec_size)))
#define GFREQ_VALUE(arr, i, rec_size) ((arr) + (i) * (rec_size) + sizeof(size_t))

#define NONE SIZE_MAX

typedef struct { size_t offset, length, next; } gblock;
typedef struct {
    uint64_t prefix;
    unsigned int bits;
    size_t child[2], head, tail, start, end;
} gprefix_node;
typedef struct {
    tspr3_tri_stats stats;
    size_t live;
    unsigned char *a, *b;
    gblock *blocks;
    gprefix_node *nodes;
    size_t used, capacity;
    size_t elem_size;
    size_t rec_size;
    tspr3_generic_compare_fn compare;
    tspr3_generic_key_fn key_fn;
    void *ctx;
} gwork;

static void *greserve(gwork *w, size_t count, size_t size) {
    void *p;
    if(count > SIZE_MAX / size || count*size > SIZE_MAX-w->live) return NULL;
    p=malloc(count*size);
    if(p) {
        w->live+=count*size;
        if(w->live>w->stats.peak_aux_bytes) w->stats.peak_aux_bytes=w->live;
    }
    return p;
}
static void grelease(gwork *w, void *p, size_t bytes) {
    if(p) { free(p);w->live-=bytes; }
}
static int gkey_compare(gwork *w, const void *a, const void *b) {
    ++w->stats.comparisons;
    return w->compare(a,b,w->ctx);
}
static void glocal_sort(gwork *w, unsigned char *v, size_t count) {
    size_t es=w->elem_size;
    unsigned char tmp[sizeof(const void *)];
    unsigned char *tbuf=tmp;
    if(count>1 && gkey_compare(w,v,v+es)>0) {
        memcpy(tbuf,v,es);memcpy(v,v+es,es);memcpy(v+es,tbuf,es);
    }
    if(count>2) {
        if(gkey_compare(w,v+es,v+2*es)>0) {
            memcpy(tbuf,v+es,es);memcpy(v+es,v+2*es,es);memcpy(v+2*es,tbuf,es);
        }
        if(gkey_compare(w,v,v+es)>0) {
            memcpy(tbuf,v,es);memcpy(v,v+es,es);memcpy(v+es,tbuf,es);
        }
    }
}
static uint64_t gnumeric_key(gwork *w, const void *element) {
    return w->key_fn(element,w->ctx);
}
static unsigned int gleading_zeroes(uint64_t x) {
    unsigned int n=0;
    if(!x) return 64;
    if(!(x & UINT64_C(0xffffffff00000000))) { n+=32;x<<=32; }
    if(!(x & UINT64_C(0xffff000000000000))) { n+=16;x<<=16; }
    if(!(x & UINT64_C(0xff00000000000000))) { n+=8;x<<=8; }
    if(!(x & UINT64_C(0xf000000000000000))) { n+=4;x<<=4; }
    if(!(x & UINT64_C(0xc000000000000000))) { n+=2;x<<=2; }
    if(!(x & UINT64_C(0x8000000000000000))) ++n;
    return n;
}
static uint64_t gprefix_of(uint64_t key, unsigned int bits) {
    return bits ? key & (UINT64_MAX << (64-bits)) : 0;
}
static unsigned int gside_of(uint64_t key, unsigned int bits) {
    return (unsigned int)((key >> (63-bits)) & 1);
}
static size_t gnew_node(gwork *w, uint64_t prefix, unsigned int bits) {
    size_t id;
    if(w->used==w->capacity) {
        size_t cap=w->capacity ? w->capacity*2 : 16;
        gprefix_node *grown;
        if(w->capacity>SIZE_MAX/2 || cap>SIZE_MAX/sizeof(*grown)) return NONE;
        grown=greserve(w,cap,sizeof(*grown));
        if(!grown) return NONE;
        if(w->used) memcpy(grown,w->nodes,w->used*sizeof(*grown));
        grelease(w,w->nodes,w->capacity*sizeof(*grown));
        w->nodes=grown;w->capacity=cap;
    }
    id=w->used++;
    w->nodes[id].prefix=gprefix_of(prefix,bits);w->nodes[id].bits=bits;
    w->nodes[id].child[0]=w->nodes[id].child[1]=NONE;
    w->nodes[id].head=w->nodes[id].tail=NONE;
    w->nodes[id].start=w->nodes[id].end=0;
    return id;
}
static void gattach(gwork *w, size_t node, size_t id) {
    if(w->nodes[node].head==NONE) w->nodes[node].head=id;
    else w->blocks[w->nodes[node].tail].next=id;
    w->nodes[node].tail=id;
}
static int ginsert_block(gwork *w, size_t id, uint64_t prefix, unsigned int bits) {
    size_t at=0;
    for(;;) {
        size_t child;unsigned int side,common;
        ++w->stats.prefix_tests;
        if(bits==w->nodes[at].bits) { gattach(w,at,id);return 1; }
        side=gside_of(prefix,w->nodes[at].bits);
        child=w->nodes[at].child[side];
        if(child==NONE) {
            size_t leaf=gnew_node(w,prefix,bits);
            if(leaf==NONE) return 0;
            w->nodes[at].child[side]=leaf;gattach(w,leaf,id);return 1;
        }
        common=gleading_zeroes(prefix ^ w->nodes[child].prefix);
        if(common>bits) common=bits;
        if(common>w->nodes[child].bits) common=w->nodes[child].bits;
        if(common==w->nodes[child].bits) { at=child;continue; }
        {
            size_t parent=gnew_node(w,prefix,common);
            if(parent==NONE) return 0;
            w->nodes[at].child[side]=parent;
            w->nodes[parent].child[gside_of(w->nodes[child].prefix,common)]=child;
            if(common==bits) { gattach(w,parent,id);return 1; }
            at=parent;
        }
    }
}

/* Merge generic frequency records. Operates on byte arrays with rec_size stride. */
static size_t gmerge_counts(gwork *w, const unsigned char *left, size_t nl,
    const unsigned char *right, size_t nr, unsigned char *out) {
    size_t l=0,r=0,k=0,rs=w->rec_size;
    if(!nl || !nr) {
        if(nl) memcpy(out,left,nl*rs);
        if(nr) memcpy(out,right,nr*rs);
        k=nl+nr;
    } else if(gkey_compare(w,GFREQ_VALUE(left,nl-1,rs),GFREQ_VALUE(right,0,rs))<0) {
        memcpy(out,left,nl*rs);memcpy(out+nl*rs,right,nr*rs);
        k=nl+nr;++w->stats.range_joins;
    } else if(gkey_compare(w,GFREQ_VALUE(right,nr-1,rs),GFREQ_VALUE(left,0,rs))<0) {
        memcpy(out,right,nr*rs);memcpy(out+nr*rs,left,nl*rs);
        k=nl+nr;++w->stats.range_joins;
    } else {
        while(l<nl && r<nr) {
            int cmp=gkey_compare(w,GFREQ_VALUE(left,l,rs),GFREQ_VALUE(right,r,rs));
            if(cmp<0) { memcpy(out+k*rs,left+l*rs,rs);l++;k++; }
            else if(cmp>0) { memcpy(out+k*rs,right+r*rs,rs);r++;k++; }
            else {
                memcpy(out+k*rs,left+l*rs,rs);
                GFREQ_COUNT(out,k,rs)+=GFREQ_COUNT(right,r,rs);
                l++;r++;k++;++w->stats.equal_merges;
            }
        }
        while(l<nl) { memcpy(out+k*rs,left+l*rs,rs);l++;k++; }
        while(r<nr) { memcpy(out+k*rs,right+r*rs,rs);r++;k++; }
    }
    w->stats.data_bytes_written+=(uint64_t)k*rs;
    return k;
}
static size_t gmerge_group(gwork *w, size_t head) {
    unsigned char *src=w->a,*dst=w->b;
    size_t rs=w->rec_size;
    if(head==NONE) return 0;
    while(w->blocks[head].next!=NONE) {
        size_t left=head,new_head=NONE,tail=NONE;
        while(left!=NONE) {
            size_t right=w->blocks[left].next,next;
            if(right!=NONE) {
                next=w->blocks[right].next;
                w->blocks[left].length=gmerge_counts(w,
                    src+w->blocks[left].offset*rs,w->blocks[left].length,
                    src+w->blocks[right].offset*rs,w->blocks[right].length,
                    dst+w->blocks[left].offset*rs);
            } else {
                next=NONE;
                memcpy(dst+w->blocks[left].offset*rs,src+w->blocks[left].offset*rs,
                    w->blocks[left].length*rs);
                w->stats.data_bytes_written+=(uint64_t)w->blocks[left].length*rs;
            }
            if(new_head==NONE) new_head=left;
            else w->blocks[tail].next=left;
            tail=left;left=next;
        }
        w->blocks[tail].next=NONE;head=new_head;
        { unsigned char *tmp=src;src=dst;dst=tmp; }
    }
    if(src!=w->a) {
        memcpy(w->a+w->blocks[head].offset*rs,src+w->blocks[head].offset*rs,
            w->blocks[head].length*rs);
        w->stats.data_bytes_written+=(uint64_t)w->blocks[head].length*rs;
    }
    return w->blocks[head].length;
}
static void glayout(gwork *w, size_t node, size_t *cursor) {
    size_t id,rs=w->rec_size;
    w->nodes[node].start=*cursor;
    for(id=w->nodes[node].head;id!=NONE;id=w->blocks[id].next) {
        size_t length=w->blocks[id].length;
        memcpy(w->b+(*cursor)*rs,w->a+w->blocks[id].offset*rs,length*rs);
        w->stats.data_bytes_written+=(uint64_t)length*rs;
        w->blocks[id].offset=*cursor;*cursor+=length;
    }
    if(w->nodes[node].child[0]!=NONE) glayout(w,w->nodes[node].child[0],cursor);
    if(w->nodes[node].child[1]!=NONE) glayout(w,w->nodes[node].child[1],cursor);
    w->nodes[node].end=*cursor;
}
static size_t gresolve(gwork *w, size_t node) {
    size_t child[2]={w->nodes[node].child[0],w->nodes[node].child[1]};
    size_t lengths[2]={0,0},local,total,start=w->nodes[node].start;
    size_t rs=w->rec_size;
    unsigned int side;
    for(side=0;side<2;++side) if(child[side]!=NONE) lengths[side]=gresolve(w,child[side]);
    local=gmerge_group(w,w->nodes[node].head);total=local;
    for(side=0;side<2;++side) if(lengths[side]) {
        size_t offset=w->nodes[child[side]].start;
        if(!local) {
            if(offset!=start+total) {
                memmove(w->a+(start+total)*rs,w->a+offset*rs,lengths[side]*rs);
                w->stats.data_bytes_written+=(uint64_t)lengths[side]*rs;
            }
            if(total) ++w->stats.prefix_joins;
            total+=lengths[side];
        } else {
            total=gmerge_counts(w,w->a+start*rs,total,w->a+offset*rs,lengths[side],w->b+start*rs);
            memcpy(w->a+start*rs,w->b+start*rs,total*rs);
            w->stats.data_bytes_written+=(uint64_t)total*rs;
        }
    }
    return total;
}

static tspr3_status gcompact(const void *input, size_t n, size_t elem_size,
    tspr3_tri_mode mode, gwork *w, size_t *distinct) {
    size_t m=n/3+(n%3!=0),i,cursor=0,rs=w->rec_size;
    const unsigned char *src=(const unsigned char *)input;
    unsigned char *scratch;
    tspr3_status status=TSPR3_NO_MEMORY;
    *distinct=0;
    w->stats.triads=m;
    w->a=(unsigned char*)greserve(w,n,rs);
    w->b=(unsigned char*)greserve(w,n,rs);
    w->blocks=(gblock*)greserve(w,m,sizeof(gblock));
    if(!w->a || !w->b || !w->blocks) return TSPR3_NO_MEMORY;
    if(mode==TSPR3_TRI_BIN && gnew_node(w,0,0)==NONE) return TSPR3_NO_MEMORY;
    scratch=(unsigned char*)greserve(w,3,elem_size);
    if(!scratch) return TSPR3_NO_MEMORY;
    for(i=0;i<m;++i) {
        size_t start=i*3,count=n-start,j,length=0;
        if(count>3) count=3;
        memcpy(scratch,src+start*elem_size,count*elem_size);
        glocal_sort(w,scratch,count);
        w->blocks[i].offset=cursor;w->blocks[i].next=NONE;
        for(j=0;j<count;++j) {
            unsigned char *val=scratch+j*elem_size;
            if(j && gkey_compare(w,scratch+(j-1)*elem_size,val)==0)
                GFREQ_COUNT(w->a,cursor-1,rs)++;
            else {
                GFREQ_COUNT(w->a,cursor,rs)=1;
                memcpy(GFREQ_VALUE(w->a,cursor,rs),val,elem_size);
                cursor++;length++;
            }
        }
        w->stats.data_bytes_written+=(uint64_t)length*rs+(uint64_t)(count-length)*sizeof(size_t);
        w->blocks[i].length=length;
        if(mode==TSPR3_TRI_BIN) {
            uint64_t low=gnumeric_key(w,scratch);
            uint64_t high=gnumeric_key(w,scratch+(count-1)*elem_size);
            unsigned int bits=gleading_zeroes(low^high);
            ++w->stats.prefix_tests;
            if(!bits) ++w->stats.crossing_root_blocks;
            if(!ginsert_block(w,i,gprefix_of(low,bits),bits)) { grelease(w,scratch,3*elem_size);return TSPR3_NO_MEMORY; }
        } else if(i) w->blocks[i-1].next=i;
    }
    grelease(w,scratch,3*elem_size);
    if(mode==TSPR3_TRI_BIN) {
        size_t end=0;
        glayout(w,0,&end);
        { unsigned char *tmp=w->a;w->a=w->b;w->b=tmp; }
        *distinct=gresolve(w,0);
        w->stats.prefix_nodes=w->used;
    } else *distinct=gmerge_group(w,0);
    w->stats.distinct_values=*distinct;
    status=TSPR3_OK;
    return status;
}

static tspr3_status gbase(const void *input, size_t n, size_t elem_size,
    gwork *w, void *output) {
    size_t start,width;
    unsigned char *scratch,*src,*dst;
    unsigned char *out=(unsigned char*)output;
    const unsigned char *in=(const unsigned char*)input;
    scratch=(unsigned char*)greserve(w,n,elem_size);
    if(!scratch) return TSPR3_NO_MEMORY;
    memcpy(out,in,n*elem_size);w->stats.data_bytes_written=(uint64_t)n*elem_size;
    w->stats.triads=n/3+(n%3!=0);
    for(start=0;start<n;) {
        size_t count=n-start,j;
        unsigned char v[3*sizeof(const void *)];
        unsigned char *vbuf=v;
        if(count>3) count=3;
        memcpy(vbuf,out+start*elem_size,count*elem_size);
        glocal_sort(w,vbuf,count);
        memcpy(out+start*elem_size,vbuf,count*elem_size);
        w->stats.data_bytes_written+=(uint64_t)count*elem_size;
        start+=count;
        (void)j;
    }
    src=out;dst=scratch;
    for(width=3;width<n;) {
        for(start=0;start<n;) {
            size_t mid=start+(n-start<width ? n-start : width);
            size_t end=mid+(n-mid<width ? n-mid : width);
            size_t l=start,r=mid,pos=start;
            if(mid==end || gkey_compare(w,src+(mid-1)*elem_size,src+mid*elem_size)<=0) {
                memcpy(dst+start*elem_size,src+start*elem_size,(end-start)*elem_size);
                if(mid!=end) ++w->stats.range_joins;
            } else if(gkey_compare(w,src+(end-1)*elem_size,src+start*elem_size)<0) {
                memcpy(dst+start*elem_size,src+mid*elem_size,(end-mid)*elem_size);
                memcpy(dst+(start+end-mid)*elem_size,src+start*elem_size,(mid-start)*elem_size);
                ++w->stats.range_joins;
            } else {
                while(l<mid && r<end) {
                    if(gkey_compare(w,src+l*elem_size,src+r*elem_size)<=0)
                        { memcpy(dst+pos*elem_size,src+l*elem_size,elem_size);l++; }
                    else
                        { memcpy(dst+pos*elem_size,src+r*elem_size,elem_size);r++; }
                    pos++;
                }
                while(l<mid) { memcpy(dst+pos*elem_size,src+l*elem_size,elem_size);l++;pos++; }
                while(r<end) { memcpy(dst+pos*elem_size,src+r*elem_size,elem_size);r++;pos++; }
            }
            w->stats.data_bytes_written+=(uint64_t)(end-start)*elem_size;start=end;
        }
        { unsigned char *tmp=src;src=dst;dst=tmp; }
        if(width>n/2) break;
        width*=2;
    }
    if(src!=out) { memcpy(out,src,n*elem_size);w->stats.data_bytes_written+=(uint64_t)n*elem_size; }
    grelease(w,scratch,n*elem_size);return TSPR3_OK;
}

static tspr3_status gsort_compact(const void *input, size_t n, size_t elem_size,
    tspr3_tri_mode mode, void *output, gwork *w) {
    size_t k,i,pos=0,rs=w->rec_size;
    unsigned char *out=(unsigned char*)output;
    tspr3_status status=gcompact(input,n,elem_size,mode,w,&k);
    if(status==TSPR3_OK) {
        for(i=0;i<k;++i) {
            size_t cnt=GFREQ_COUNT(w->a,i,rs),j;
            unsigned char *val=GFREQ_VALUE(w->a,i,rs);
            for(j=0;j<cnt;++j) { memcpy(out+pos*elem_size,val,elem_size);pos++; }
        }
        w->stats.data_bytes_written+=(uint64_t)n*elem_size;
    }
    return status;
}
static tspr3_status gsort_counts(const void *in, size_t n, size_t elem_size,
    void *out, gwork *w) {
    return gsort_compact(in,n,elem_size,TSPR3_TRI_COUNTS,out,w);
}
static tspr3_status gsort_binary(const void *in, size_t n, size_t elem_size,
    void *out, gwork *w) {
    return gsort_compact(in,n,elem_size,TSPR3_TRI_BIN,out,w);
}

static tspr3_status sort_references(const void *input, size_t n, size_t elem_size,
    tspr3_tri_mode mode, void *output,
    tspr3_generic_compare_fn compare, tspr3_generic_key_fn key_fn, void *ctx,
    tspr3_tri_stats *stats) {
    gwork w={0};
    tspr3_status status;
    size_t m;
    if(stats) memset(stats,0,sizeof(*stats));
    if(mode<TSPR3_TRI_BASE || mode>TSPR3_TRI_BIN) return TSPR3_INVALID;
    if(!n) return TSPR3_OK;
    if(!input || !output || !compare || !elem_size) return TSPR3_INVALID;
    if(mode==TSPR3_TRI_BIN && !key_fn) return TSPR3_INVALID;
    w.elem_size=elem_size;
    w.rec_size=GFREQ_REC_SIZE(elem_size);
    w.compare=compare;
    w.key_fn=key_fn;
    w.ctx=ctx;
    m=n/3+(n%3!=0);
    if(mode==TSPR3_TRI_BASE) {
        status=gbase(input,n,elem_size,&w,output);
    } else if(mode==TSPR3_TRI_COUNTS) {
        status=gsort_counts(input,n,elem_size,output,&w);
    } else {
        status=gsort_binary(input,n,elem_size,output,&w);
    }
    if(stats) *stats=w.stats;
    /* release resources */
    grelease(&w,w.a,n*w.rec_size);
    grelease(&w,w.b,n*w.rec_size);
    grelease(&w,w.blocks,m*sizeof(gblock));
    grelease(&w,w.nodes,w.capacity*sizeof(gprefix_node));
    return status;
}

static size_t references_memory_bound(size_t n, size_t elem_size, tspr3_tri_mode mode) {
    size_t m,base,rec_size,capacity=16,nodes;
    if(mode<TSPR3_TRI_BASE || mode>TSPR3_TRI_BIN) return SIZE_MAX;
    if(!n || !elem_size) return 0;
    m=n/3+(n%3!=0);
    if(mode==TSPR3_TRI_BASE) {
        if(n>SIZE_MAX/elem_size) return SIZE_MAX;
        /* scratch buffer + temp for local_sort (3 elements) */
        base=n*elem_size;
        if(3*elem_size>SIZE_MAX-base) return SIZE_MAX;
        return base+3*elem_size;
    }
    rec_size=GFREQ_REC_SIZE(elem_size);
    if(n>SIZE_MAX/(2*rec_size)) return SIZE_MAX;
    base=2*n*rec_size;
    if(m>(SIZE_MAX-base)/sizeof(gblock)) return SIZE_MAX;
    base+=m*sizeof(gblock);
    /* scratch for local_sort (3 elements) */
    if(3*elem_size>SIZE_MAX-base) return SIZE_MAX;
    base+=3*elem_size;
    if(mode==TSPR3_TRI_COUNTS) return base;
    if(m>(SIZE_MAX-1)/2) return SIZE_MAX;
    nodes=2*m+1;
    while(capacity<nodes) { if(capacity>SIZE_MAX/2) return SIZE_MAX;capacity*=2; }
    if(capacity>SIZE_MAX/sizeof(gprefix_node)) return SIZE_MAX;
    nodes=capacity*sizeof(gprefix_node);
    if(nodes>SIZE_MAX-base || nodes/2>SIZE_MAX-base-nodes) return SIZE_MAX;
    return base+nodes+nodes/2;
}

/* The internal engine only sorts pointer representations. Callbacks always see
 * the caller's original, correctly aligned objects. A position tie-break keeps
 * equal keys stable and prevents frequency compaction from discarding payload. */
typedef struct {
    tspr3_generic_compare_fn compare;
    tspr3_generic_key_fn key;
    void *context;
} reference_context;
static const void *load_reference(const void *slot) {
    const void *value;
    memcpy(&value,slot,sizeof(value));
    return value;
}
static int compare_references(const void *a,const void *b,void *opaque) {
    reference_context *context=(reference_context *)opaque;
    const void *left=load_reference(a),*right=load_reference(b);
    int order=context->compare(left,right,context->context);
    if(order) return order;
    /* Both point into the same input array; unsigned byte pointers are ordered. */
    return ((const unsigned char *)left>(const unsigned char *)right)-
           ((const unsigned char *)left<(const unsigned char *)right);
}
static uint64_t key_reference(const void *slot,void *opaque) {
    reference_context *context=(reference_context *)opaque;
    return context->key(load_reference(slot),context->context);
}
size_t tspr3_generic_tri_memory_bound(size_t n,size_t elem_size,tspr3_tri_mode mode) {
    size_t references,bound;
    if(mode<TSPR3_TRI_BASE || mode>TSPR3_TRI_BIN || !elem_size) return SIZE_MAX;
    if(!n) return 0;
    if(n>PTRDIFF_MAX/elem_size || n>SIZE_MAX/(2*sizeof(const void *))) return SIZE_MAX;
    references=2*n*sizeof(const void *);
    bound=references_memory_bound(n,sizeof(const void *),mode);
    if(bound==SIZE_MAX || bound>SIZE_MAX-references) return SIZE_MAX;
    return bound+references;
}
tspr3_status tspr3_generic_tri_sort(const void *input,size_t n,size_t elem_size,
    tspr3_tri_mode mode,void *output,tspr3_generic_compare_fn compare,
    tspr3_generic_key_fn key_fn,void *ctx,tspr3_tri_stats *stats) {
    const void **references,**ordered;
    size_t i,bytes,bound;
    tspr3_status status;
    tspr3_tri_stats measured={0};
    reference_context context={compare,key_fn,ctx};
    if(stats) memset(stats,0,sizeof(*stats));
    bound=tspr3_generic_tri_memory_bound(n,elem_size,mode);
    if(bound==SIZE_MAX) return TSPR3_INVALID;
    if(!n) return TSPR3_OK;
    if(!input || !output || !compare || (mode==TSPR3_TRI_BIN && !key_fn)) return TSPR3_INVALID;
    bytes=n*sizeof(*references);
    references=malloc(bytes);ordered=malloc(bytes);
    if(!references || !ordered) { free(references);free(ordered);return TSPR3_NO_MEMORY; }
    for(i=0;i<n;++i) references[i]=(const unsigned char *)input+i*elem_size;
    status=sort_references(references,n,sizeof(*references),mode,ordered,
        compare_references,key_fn?key_reference:NULL,&context,&measured);
    if(status==TSPR3_OK) {
        for(i=0;i<n;++i) memcpy((unsigned char *)output+i*elem_size,ordered[i],elem_size);
        measured.data_bytes_written+=(uint64_t)n*elem_size;
    }
    measured.data_bytes_written+=(uint64_t)n*sizeof(*references);
    measured.peak_aux_bytes+=2*bytes;
    measured.distinct_values=0; /* Generic records are not collapsed by key. */
    if(stats) *stats=measured;
    free(references);free(ordered);return status;
}
