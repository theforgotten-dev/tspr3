"""Paired comparisons of the three ternas variants, without external baselines."""
import csv
import statistics
from collections import defaultdict
from pathlib import Path

raw=list(csv.DictReader(Path('benchmarks/results/tri_raw.csv').open(encoding='utf-8')))
groups=defaultdict(dict)
for row in raw:
    groups[(row['scenario'],row['seed'])][row['variant']]=row
ratios=defaultdict(list)
all_freq=[]
all_bin=[]
for (scenario,seed),variants in groups.items():
    assert set(variants)=={'TERNAS','TERNAS_FREQ','TERNAS_BIN'}
    base=float(variants['TERNAS']['ms'])
    freq=float(variants['TERNAS_FREQ']['ms'])
    binary=float(variants['TERNAS_BIN']['ms'])
    ratios[scenario].append((base/freq,freq/binary))
    all_freq.append(base/freq)
    all_bin.append(freq/binary)
print('scenario,median_BASE_over_FREQ,median_FREQ_over_BIN')
for scenario,values in ratios.items():
    print(f'{scenario},{statistics.median(v[0] for v in values):.3f},'
          f'{statistics.median(v[1] for v in values):.3f}')
print(f'paired_inputs={len(groups)}, sort_checks={len(raw)}')
print(f'FREQ faster than BASE: {sum(x>1 for x in all_freq)}/{len(all_freq)} paired inputs')
print(f'BIN faster than FREQ: {sum(x>1 for x in all_bin)}/{len(all_bin)} paired inputs')
print(f'median_BASE/FREQ={statistics.median(all_freq):.3f}; median_FREQ/BIN={statistics.median(all_bin):.3f}')
print(f'median_BIN/FREQ={statistics.median(1/x for x in all_bin):.3f}')
