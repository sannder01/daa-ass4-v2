import json
import pathlib
import random
import subprocess
import sys

ROOT = pathlib.Path(__file__).resolve().parents[2]
BINARY = ROOT / "Cpp" / "build" / "main.exe"
sys.path.insert(0, str(ROOT / "tools"))
from public_validate import check


def run(requests):
    data = "".join(json.dumps(item, separators=(",", ":")) + "\n" for item in requests)
    result = subprocess.run([str(BINARY)], input=data, text=True, capture_output=True, check=True)
    return [json.loads(line) for line in result.stdout.splitlines()]


fixtures = json.loads((pathlib.Path(__file__).with_name("cases.json")).read_text(encoding="utf-8"))
answers = run([case["input"] for case in fixtures])
for case, answer in zip(fixtures, answers):
    if not check(case["input"], answer, case["output"]):
        raise AssertionError(f"{case['name']}: {answer} does not satisfy {case['output']}")

generator = random.Random(20261108)
search_requests = []
for _ in range(200):
    text = "".join(generator.choice("ab c!") for _ in range(generator.randrange(0, 40)))
    pattern = "".join(generator.choice("ab c!") for _ in range(generator.randrange(0, 12)))
    for task in ("naive", "kmp", "rk"):
        search_requests.append({"task": task, "text": text, "pattern": pattern})
search_answers = run(search_requests)
for offset in range(0, len(search_requests), 3):
    request = search_requests[offset]
    text, pattern = request["text"], request["pattern"]
    if pattern:
        expected = [i for i in range(len(text) - len(pattern) + 1) if text.startswith(pattern, i)]
    else:
        expected = list(range(len(text) + 1))
    group = [answer["positions"] for answer in search_answers[offset:offset + 3]]
    if group != [expected, expected, expected]:
        raise AssertionError(f"search differential failure for {request}: {group} != {expected}")

n = 100_000
chain = {"task": "tree", "n": n, "weights": [1] * n, "edges": [[i, i + 1] for i in range(n - 1)]}
chain_answer = run([chain])[0]
if chain_answer["weight"] != n // 2 or len(chain_answer["selected"]) != n // 2:
    raise AssertionError("deep-chain reconstruction failed")

print(f"PASS {len(fixtures)} fixtures, 200 seeded search triples, and a {n}-vertex chain")
