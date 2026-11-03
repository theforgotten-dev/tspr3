"""Run the historical experiment; SUM and FULL measure different operations."""
from pathlib import Path
import runpy

if __name__ == "__main__":
    runpy.run_path(
        str(Path(__file__).resolve().parent / "experiments/legacy/python/tspr3_verify.py"),
        run_name="__main__",
    )
