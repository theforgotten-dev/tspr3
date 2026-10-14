from random import Random
from time import perf_counter_ns


def ternas(a):
    return [a[i:i + 3] for i in range(0, len(a), 3)]


def tspr3_sum(a):
    return sorted(
        ((sum(t), i) for i, t in enumerate(ternas(a))),
        key=lambda x: (x[0], x[1]),
    )


def merge(left, right):
    if not left:
        return right
    if not right:
        return left
    if left[-1] <= right[0]:
        return left + right
    if right[-1] <= left[0]:
        return right + left
    out = []
    i = j = 0
    while i < len(left) and j < len(right):
        if left[i] <= right[j]:
            out.append(left[i])
            i += 1
        else:
            out.append(right[j])
            j += 1
    out.extend(left[i:])
    out.extend(right[j:])
    return out


def tspr3_full(a):
    blocks = [sorted(t) for t in ternas(a)]
    while len(blocks) > 1:
        nxt = []
        for i in range(0, len(blocks), 2):
            if i + 1 < len(blocks):
                nxt.append(merge(blocks[i], blocks[i + 1]))
            else:
                nxt.append(blocks[i])
        blocks = nxt
    return blocks[0] if blocks else []


def verify():
    rng = Random(20261013)
    for n in range(0, 501):
        for _ in range(10):
            a = [rng.randint(-1000, 1000) for _ in range(n)]
            expected_sum = sorted(
                ((sum(a[i:i + 3]), i // 3)
                 for i in range(0, n, 3)),
                key=lambda x: (x[0], x[1]),
            )
            assert tspr3_sum(a) == expected_sum, ("SUM", a)
            assert tspr3_full(a) == sorted(a), ("FULL", a)
    print("OK: 5010 casos aleatorios para ambas variantes")


def bench():
    rng = Random(42)
    for n in (1000, 10000, 100000):
        a = [rng.randint(-10**6, 10**6) for _ in range(n)]
        for label, fn in (
            ("SUM", tspr3_sum),
            ("FULL", tspr3_full),
            ("sorted(A)", sorted),
        ):
            samples = []
            for _ in range(5):
                t0 = perf_counter_ns()
                fn(a)
                samples.append((perf_counter_ns() - t0) / 1e6)
            samples.sort()
            print(f"n={n:>6} {label:>9}: mediana={samples[2]:.3f} ms")
        print("Nota: SUM y FULL no producen el mismo tipo de salida")


if __name__ == "__main__":
    verify()
    bench()
