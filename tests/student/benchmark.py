import json
import pathlib
import random
import statistics
import subprocess
import time

ROOT = pathlib.Path(__file__).resolve().parents[2]
BINARY = ROOT / "Cpp" / "build" / "main.exe"
SEED = 20261108
REPEATS = 10
RUNS = 3


def measure(task, text, pattern):
    request = {"task": task, "text": text, "pattern": pattern}
    data = (json.dumps(request, separators=(",", ":")) + "\n") * REPEATS
    durations = []
    expected = None
    for _ in range(RUNS):
        start = time.perf_counter()
        result = subprocess.run([str(BINARY)], input=data, text=True, capture_output=True, check=True)
        elapsed = time.perf_counter() - start
        answers = [json.loads(line)["positions"] for line in result.stdout.splitlines()]
        if len(answers) != REPEATS or any(value != answers[0] for value in answers):
            raise AssertionError(f"unstable output for {task}")
        if expected is None:
            expected = answers[0]
        elif answers[0] != expected:
            raise AssertionError(f"different answers across runs for {task}")
        durations.append(elapsed * 1000 / REPEATS)
    return durations, expected


randomizer = random.Random(SEED)
print("case,n,m,algorithm,run1_ms,run2_ms,run3_ms,median_ms,verified_positions")
for n in (300, 1000, 3000):
    m = n // 3
    random_text = "".join(randomizer.choice("abcdefghijklmnopqrstuvwxyz") for _ in range(n))
    random_pattern = "".join(randomizer.choice("abcdefghijklmnopqrstuvwxyz") for _ in range(m))
    for label, text, pattern in (
        ("random", random_text, random_pattern),
        ("repetitive", "a" * n, "a" * m),
    ):
        reference = None
        for task in ("naive", "kmp", "rk"):
            durations, positions = measure(task, text, pattern)
            if reference is None:
                reference = positions
            elif positions != reference:
                raise AssertionError(f"algorithms disagree for {label} n={n}")
            median = statistics.median(durations)
            print(f"{label},{n},{m},{task},{durations[0]:.4f},{durations[1]:.4f},{durations[2]:.4f},{median:.4f},{len(positions)}")
