"""Ridge regression of log runtime; train fold only. Standard library only."""
import csv
import json
import math
import statistics
from pathlib import Path

rows=list(csv.DictReader(Path('benchmarks/results/tri_2m_raw.csv').open(encoding='utf-8')))
training=[r for r in rows if r['fold']=='train']
assert len(training)==36*3*3, 'The complete 2M experiment is required.'
names=['TERNAS','TERNAS_FREQ','TERNAS_BIN']
feature_names=['local_duplicates','sample_duplicates','separated_ranges','common_prefix',
               'sign_crossings','log_interval_width','log_boundary_crossings','ascending_hint','log_total_range']
features=[[float(r[f'f{i}']) for i in range(9)] for r in training]
center=[statistics.mean(x[i] for x in features) for i in range(9)]
scale=[statistics.pstdev(x[i] for x in features) or 1 for i in range(9)]

def design(row):
    return [1]+[(float(row[f'f{i}'])-center[i])/scale[i] for i in range(9)]

def solve(matrix,vector):
    a=[list(row)+[v] for row,v in zip(matrix,vector)]
    for i in range(len(a)):
        pivot=max(range(i,len(a)),key=lambda j:abs(a[j][i]))
        a[i],a[pivot]=a[pivot],a[i]
        assert abs(a[i][i])>1e-12
        divisor=a[i][i];a[i]=[v/divisor for v in a[i]]
        for j in range(len(a)):
            if j!=i:
                factor=a[j][i];a[j]=[v-factor*w for v,w in zip(a[j],a[i])]
    return [row[-1] for row in a]

models=[]
for name in names:
    data=[r for r in training if r['variant']==name]
    x=[design(r) for r in data]
    y=[math.log(float(r['ms'])/(int(r['n'])/1e6)) for r in data]
    matrix=[[sum(row[i]*row[j] for row in x)+(0.01 if i==j and i>0 else 0)
             for j in range(10)] for i in range(10)]
    vector=[sum(row[i]*v for row,v in zip(x,y)) for i in range(10)]
    weights=solve(matrix,vector)
    predictions=[sum(v*w for v,w in zip(row,weights)) for row in x]
    errors=sorted(max(0,v-p) for v,p in zip(y,predictions))
    margin=errors[math.ceil(0.90*len(errors))-1]
    models.append({'variant':name,'weights':weights,'log_margin':margin,
                   'training_log_rmse':math.sqrt(statistics.mean((v-p)**2 for v,p in zip(y,predictions)))})

# C stays experimental unless paired training shows a >5% win over B.
pairs={}
for r in training: pairs.setdefault((r['scenario'],r['seed']),{})[r['variant']]=float(r['ms'])
enabled=[True,True,any(p['TERNAS_BIN']<0.95*p['TERNAS_FREQ'] for p in pairs.values())]
model={'feature_names':feature_names,'center':center,'scale':scale,'models':models,
       'enabled':enabled,'ridge_lambda':0.01,'advantage_required':0.10,
       'training_inputs':len(pairs),'training_n':2000000}
Path('models/c/log_model.json').write_text(json.dumps(model,indent=2)+'\n',encoding='utf-8')
array=lambda xs:'{'+','.join(format(x,'.17g') for x in xs)+'}'
header='#ifndef TSPR3_LOG_WEIGHTS_H\n#define TSPR3_LOG_WEIGHTS_H\n/* Generated from TRAIN rows only by fit_log_selector.py. */\n'
header+='static const double log_center[9]='+array(center)+';\n'
header+='static const double log_scale[9]='+array(scale)+';\n'
header+='static const double log_weights[3][10]={'+','.join(array(m['weights']) for m in models)+'};\n'
header+='static const double log_margin[3]='+array([m['log_margin'] for m in models])+';\n'
header+='static const int log_enabled[3]={'+','.join(str(int(v)) for v in enabled)+'};\n#endif\n'
Path('src/c/internal/tspr3_log_weights.h').write_text(header,encoding='utf-8')
print(f'Trained on {len(pairs)} inputs, n=2,000,000; test rows unused. C enabled={enabled[2]}')
for m in models: print(m['variant'],f"training log RMSE={m['training_log_rmse']:.4f}")
