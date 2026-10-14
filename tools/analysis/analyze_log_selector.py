"""Held-out choice quality and prediction errors, matched by scenario/seed."""
import csv
import math
import statistics
from collections import Counter, defaultdict
from pathlib import Path

raw=list(csv.DictReader(Path('benchmarks/results/tri_2m_raw.csv').open(encoding='utf-8')))
validation=list(csv.DictReader(Path('benchmarks/results/log_validation.csv').open(encoding='utf-8')))
assert len(raw)==540 and len(validation)==72
pairs=defaultdict(dict)
for row in raw:
    if row['fold']=='test': pairs[(row['scenario'],row['seed'])][row['variant']]=row
choices=Counter()
regret=[]
speedups=[]
errors=defaultdict(list)
scenario_rows=defaultdict(list)
correct=0
for row in validation:
    p=pairs[(row['scenario'],row['seed'])]
    assert len(p)==3 and all(r['fold']=='test' for r in p.values())
    times={name:float(r['ms']) for name,r in p.items()}
    selected=row['selected'];choices[selected]+=1
    best=min(times,key=times.get)
    correct+=selected==best
    ratio=times[selected]/times[best]
    regret.append(ratio)
    speedups.append(times['TERNAS']/times[selected])
    scenario_rows[row['scenario']].append(row)
    for name,column in zip(('TERNAS','TERNAS_FREQ','TERNAS_BIN'),('predicted_A_ms','predicted_B_ms','predicted_C_ms')):
        errors[name].append(abs(float(row[column])-times[name])/times[name])
print(f'held_out_inputs={len(validation)}; selections={dict(choices)}')
print(f'Chosen measured fastest={correct}/{len(validation)}; median choice/best={statistics.median(regret):.3f}; worst={max(regret):.3f}')
print(f'Median paired BASE/choice={statistics.median(speedups):.3f}')
for name,values in errors.items(): print(f'{name} held-out median relative prediction error={statistics.median(values)*100:.2f}%')
print('scenario,selected_modes,median_total_ms')
for scenario,rows in scenario_rows.items():
    print(f"{scenario},{'/'.join(sorted({r['selected'] for r in rows}))},"
          f"{statistics.median(float(r['selected_total_ms']) for r in rows):.3f}")
print('Choice/best uses matched timings from tri_2m_raw.csv, not the separate end-to-end validation run.')
