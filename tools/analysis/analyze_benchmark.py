"""Fit a descriptive cost model from local C timings; Python standard library only.

Usage: python analyze_benchmark.py build/optimization.txt
Fit random/cardinality distributions only; monotonic/range data is excluded.
Timings are medians, not raw independent observations or confidence intervals.
"""
import math
import sys
from pathlib import Path

rows = []
raw = Path(sys.argv[1]).read_bytes()
contents = raw.decode("utf-16" if raw.startswith((b"\xff\xfe", b"\xfe\xff")) else "utf-8-sig")
for line in contents.splitlines():
    fields = line.split(",")
    if len(fields) == 8 and (fields[0] == "random" or fields[0] == "duplicates" or fields[0].startswith("k")):
        name, n, k, method, full, freq, auto, baseline = fields
        rows.append((int(n), int(k), float(full), float(freq)))

# Ordinary least squares through origin: FREQ = a*n + b*k*log2(k).
x = [float(n) for n, k, full, freq in rows]
z = [k * math.log2(max(k, 2)) for n, k, full, freq in rows]
y = [freq for n, k, full, freq in rows]
xx = sum(v*v for v in x)
zz = sum(v*v for v in z)
xz = sum(v*w for v, w in zip(x, z))
xy = sum(v*w for v, w in zip(x, y))
zy = sum(v*w for v, w in zip(z, y))
det = xx*zz - xz*xz
a = (xy*zz - zy*xz)/det
b = (zy*xx - xy*xz)/det
u = [n*math.log2(max(n, 2)) for n, k, full, freq in rows]
c = sum(v*row[2] for v, row in zip(u, rows))/sum(v*v for v in u)

def rmse(actual, predicted):
    return math.sqrt(sum((v-w)**2 for v,w in zip(actual,predicted))/len(actual))

print(f"training_rows={len(rows)}")
print(f"T_FREQ_ms = {a:.12g}*n + {b:.12g}*k*log2(max(k,2))")
print(f"T_FULL_ms = {c:.12g}*n*log2(max(n,2))")
print(f"FREQ training RMSE_ms={rmse(y,[a*v+b*w for v,w in zip(x,z)]):.6f}")
print(f"FULL training RMSE_ms={rmse([r[2] for r in rows],[c*v for v in u]):.6f}")
print("Policy: FREQ predicted cost <= 0.65*FULL predicted cost; cap k at n/4.")
print("Coefficients are local estimates; RMSE describes training error, not held-out accuracy.")
