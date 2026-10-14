"""Report paired median speedups and AUTO regret from million_raw.csv.
No fitting or new calibration; Python standard library only.
"""
import csv
import statistics
from collections import defaultdict
from pathlib import Path

raw = list(csv.DictReader(Path("benchmarks/results/million_raw.csv").open(encoding="utf-8")))
groups = defaultdict(dict)
for row in raw:
    groups[(row["scenario"], row["seed"])][row["algorithm"]] = float(row["ms"])
speedups = defaultdict(list)
regrets = defaultdict(list)
methods = defaultdict(set)
for row in raw:
    if row["algorithm"] == "AUTO":
        methods[row["scenario"]].add(row["method"])
for (scenario, seed), timing in groups.items():
    assert set(timing) == {"AUTO", "FULL", "FREQ", "RADIX", "qsort"}
    speedups[scenario].append(timing["FULL"] / timing["AUTO"])
    regrets[scenario].append(timing["AUTO"] / min(timing.values()))
print("scenario,median_paired_FULL_speedup,median_AUTO_over_best,methods")
for scenario in speedups:
    print(f"{scenario},{statistics.median(speedups[scenario]):.3f},"
          f"{statistics.median(regrets[scenario]):.3f},{'/'.join(sorted(methods[scenario]))}")
all_ratios = [x for ratios in speedups.values() for x in ratios]
all_regrets = [x for ratios in regrets.values() for x in ratios]
print(f"paired_inputs={len(groups)}, timed_outputs={len(raw)}")
print(f"AUTO wins FULL on {sum(v > 1 for v in all_ratios)}/{len(all_ratios)} paired inputs")
print(f"Median paired FULL speedup={statistics.median(all_ratios):.3f}")
print(f"Worst AUTO/best single measured run={max(all_regrets):.3f}")
