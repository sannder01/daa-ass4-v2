# Assignment 4 — analysis

## Environment and commands

- Language: C++17 (`Cpp` in `submission.json`).
- Compiler: MinGW-w64 g++ 16.2.0, `-O2`.
- OS: Windows 11 Home, version 10.0.26200.
- CPU: AMD Ryzen 5 8400F, 6 cores / 12 logical processors. Installed RAM reported by Windows: about 15.6 GiB.
- Python launcher: Python 3.11.9.
- Build: `g++ -std=c++17 -O2 -o Cpp/build/main.exe Cpp/source/main.cpp`.
- Run JSON Lines input: `Cpp/build/main.exe < requests.jsonl`.
- Official public checks: `python tools/run.py --check`.
- Student edge, randomized and deep-chain checks: `python tests/student/check.py`.
- Search timings: `python tests/student/benchmark.py`.

The launcher compiles before checking. Compilation is excluded from the search timings. The benchmark sends ten identical requests to one newly started process for each measurement and divides elapsed wall time by ten. There is no JIT; no separate warm-up is performed. The same process startup, JSON parsing and output serialization are included for all three algorithms. Outputs are checked against one another before times are reported.

## Algorithm explanations

### LCS

`dp[i][j]` is the length of an LCS of prefixes `a[0..i)` and `b[0..j)`. Empty prefixes have value zero. Equal final characters extend `dp[i-1][j-1]`; otherwise the best solution drops one of the two final characters, so the cell is the larger of `dp[i-1][j]` and `dp[i][j-1]`. Rows and columns are filled in increasing order, after the smaller prefixes they depend on.

Reconstruction starts at `(n,m)`. Equal characters are appended and both indices decrease. On a mismatch, it moves to a neighbor with the same optimal value. Reversing the collected characters restores their original order. Thus each emitted character belongs to both strings in order, and the walk emits exactly `dp[n][m]` characters. Time and auxiliary memory are `O((n+1)(m+1))`.

Boundary check: two empty strings must produce length zero and an empty sequence. The `lcs-not-substring` public case catches solutions that mistakenly search for a contiguous substring.

### Ordered adjacent-block merge

`dp[l][r]` is the minimum cost to combine the original consecutive blocks from `l` through `r`. A single block costs zero. If the final operation splits at `k`, its cost is `dp[l][k] + dp[k+1][r] + sum(l..r)`. Prefix sums provide the interval sum. Increasing interval length ensures both smaller intervals are already solved. The minimizing split is saved.

Reconstruction recursively emits the left and right subplans, then `[l,k,r]`; therefore each merge happens after the two adjacent groups it combines already exist. The number of operations is `n-1`. Time is `O(n^3)` and auxiliary memory is `O(n^2)`; the empty input uses constant space. Costs and sums use signed 64-bit integers.

Boundary check: zero sizes exercise equal-cost split ties and ensure zero-cost plans are still valid. The public 64-bit case catches implementations that store accumulated costs in 32-bit integers. A one-block input must have an empty plan.

### Maximum-weight independent set on a tree

After building the adjacency list, an iterative traversal roots the tree at vertex zero and records parent-before-child order. In reverse order, `take[u]` is the best value when `u` is selected, and `skip[u]` is the best value when it is not. Selecting `u` forces each child to be skipped; skipping it allows the better of each child's two states. A vertex with no children has `take=weight` and `skip=0`. The empty set makes zero available for every skipped subtree, so negative weights are safely omitted.

Reconstruction walks from the root while carrying whether its parent was selected. A selected parent forces the child to be skipped; otherwise the child is selected only when `take > skip`. Ties are resolved by skipping. This produces a valid optimum. The traversal, DP and reconstruction take `O(n)` time and `O(n)` memory, without recursion on the input tree.

Boundary check: the all-negative fixture must return the empty set with value zero. The 100,000-vertex chain in `check.py` catches recursive stack overflow and verifies the optimum value and witness size.

### Naive search

For each valid start from zero through `n-m`, the algorithm compares pattern characters from left to right and reports the start only if all `m` match. It advances by one after every attempt, so overlapping matches are retained. A longer pattern has no valid start. An empty pattern is handled before indexing and returns all `n+1` boundaries.

The worst-case time is `O(nm+n+1)`, including empty-pattern output; auxiliary memory is `O(1)` apart from the `O(z+1)` result. The last valid nonempty start is `n-m`.

Boundary check: an empty pattern against `abc` must report `[0,1,2,3]`; this catches skipped terminal boundaries and invalid pattern indexing.

### KMP search

`pi[i]` is the length of the longest proper prefix of the pattern that is also a suffix ending at `i`. It is built left-to-right: on mismatch, the already computed prefix table selects the next shorter candidate; a match extends the current prefix. During the text scan, the same fallback reuses matched-prefix information rather than rechecking the text. After a complete match, fallback to `pi[m-1]` retains a possible overlap.

The empty pattern is handled before indexing. The scan is `O(n+m+z)` time, the prefix table uses `O(m)` auxiliary memory and reported starts use `O(z)` output memory.

Boundary check: `abababa` with pattern `aba` must return starts `[0,2,4]`; it catches resetting to zero after a match and losing overlaps. Seeded differential checks compare KMP with a simple Python oracle.

### Exact Rabin–Karp search

The polynomial hash uses ASCII codes, base 31 and modulus 101. The first character has the highest power. For each next window, the outgoing character's weighted value is subtracted, the remainder is normalized, and the result is multiplied by the base before adding the incoming character. A hash match is only a candidate: every character is checked before a position is emitted. This verification prevents collisions from becoming false matches.

Empty and too-long patterns return before a window is formed. For `1 <= m <= n`, time is `O(n+m+c*m)`, where `c` is the number of equal-hash candidates; the candidate verification can make the algorithm quadratic. A bound for every allowed input, including empty-pattern output, is `O(nm+n+m+1)`. Auxiliary rolling-hash memory is `O(1)` and output memory is `O(z)`.

Boundary check: `ab` and `dj` collide with the required hash parameters, but only `ab` is reported in `abdj`. This catches treating equal hashes as proof of equality. The collision fixture also verifies a nonmatching colliding window.

### Suffix array and Kasai LCP

Suffix-array construction begins with each suffix start and ranks characters. At doubling width `w`, suffixes are sorted by `(rank[i], rank[i+w])`, using `-1` when the second half is past the end. New ranks identify equal and ordered pairs. Width doubles until every suffix has a unique rank. The comparator sort costs `O(n log^2(n+1))` time overall and the rank arrays use `O(n)` memory. No suffix strings or sentinel are created.

Kasai first maps each start to its suffix-array position. For each text start, it compares that suffix with the previous suffix in array order, stores the common-prefix length, then reduces the carried length by one for the next start. This reuse makes the pass linear. Rank zero has no previous suffix and resets the carried length. The maximum LCP is the longest repeated substring: any repeated substring appears as a prefix of at least two suffixes, and some adjacent pair in lexicographic order has at least that common-prefix length. Overlaps are allowed. Total auxiliary and output memory are `O(n)`.

Boundary check: empty input returns empty arrays and string. `aaaa` catches a wrong suffix order, incorrect LCP reuse, and failure to permit overlapping repeats. Public tests and fixture outputs also check that no empty suffix is included.

### Aho–Corasick

Each trie node has one transition for each printable ASCII character and a failure link. Breadth-first construction sets each failure link to the longest suffix that is also a trie prefix; missing transitions inherit the transition of the failure state. This makes every text character a single automaton transition.

For each independent text, the scan increments a visit count for the reached state. Processing nodes in reverse breadth-first order adds each count to its failure state. As a result, a terminal node receives all visits at states for which its pattern is a suffix. Mapping the resulting terminal counts back through the original pattern list preserves duplicates and dictionary order. Each text starts with a fresh state and fresh counts. Empty dictionary rows and empty texts naturally produce the required empty or zero counts.

For total pattern length `L`, `S` trie nodes, `p` dictionary entries, and alphabet size 95, construction takes `O(L+95S+p)` time and `O(95S+p)` memory. Each text takes `O(|text|+S+p)` time for scan, propagation and output; output memory is `O(qp)` for `q` texts.

Boundary check: duplicate `he` patterns and suffix pattern `he` within `she` must each count correctly in repeated texts. Empty text must yield one zero per pattern, and empty dictionary must yield an empty row per text. These catch missing failure suffix counts, accidental duplicate collapse and stale per-text state.

## Own tests

|Command|Input or fixture|Expected / actual|What it checks|
|---|---|---|---|
|LCS|`cases.json`: repeated symbols and empty strings|Both pass|Empty prefixes and repeated-character reconstruction|
|Merge|`cases.json`: zero sizes and one block|Both pass|Tied split choices, chronological valid plan and base case|
|Tree|`cases.json`: negative weights and one vertex|Both pass|Empty-set optimum and leaf base case|
|Tree|`check.py`: 100,000-vertex chain|Pass; value 50,000 and 50,000 selected vertices|Iterative traversal and reconstruction at the input limit|
|Naive/KMP/RK|`check.py`: 200 seeded text/pattern pairs, checked against a Python oracle|All three agree with the oracle|Empty patterns, overlaps, spaces, punctuation and mismatches|
|RK|`cases.json`: `ab` / `dj` collision|Pass; only true match reported|Exact character verification after a hash match|
|Suffix|`cases.json`: empty and repeated-character strings; `tests/public.json`: order and overlap examples|Pass|Empty suffix array, ordering, Kasai values and overlapping repeat|
|Aho|`cases.json`: duplicates, suffix patterns, empty text and empty dictionary|Pass|Failure propagation, duplicate positions and independent rows|
|Mixed process|`python tools/run.py --check` repeated mixed batch|Pass|Per-request state reset|

Commands run from the repository root. `python tools/run.py --check` passed all 44 public cases and the repeated mixed batch. `python tests/student/check.py` passed 14 fixtures, 200 seeded search triples and the 100,000-vertex chain.

## Search comparison

Generator: Python `random.Random(20261108)`, lowercase ASCII for random text and independently generated random pattern. Repetitive inputs use only `a`. Sizes are `n = 300, 1000, 3000` and `m = floor(n/3)`, all in the algorithms' common domain. Each printed time is the wall-clock time for ten identical JSON Lines requests divided by ten. Three runs were made per algorithm and case; the median is in milliseconds per request. `verified_positions` is the number of reported starts; outputs were compared across all three algorithms before being printed. The benchmark command reproduces these measurements and uses no compilation in its timed section.

|Case|n|m|Algorithm|Run 1 ms|Run 2 ms|Run 3 ms|Median ms|Verified positions|
|---|---:|---:|---|---:|---:|---:|---:|---:|
|Random|300|100|Naive|1.7470|1.4115|1.4187|1.4187|0|
|Random|300|100|KMP|1.4038|1.3452|1.4114|1.4038|0|
|Random|300|100|RK|1.4051|1.4124|1.5827|1.4124|0|
|Repetitive|300|100|Naive|1.4190|1.3976|1.3978|1.3978|201|
|Repetitive|300|100|KMP|1.3792|1.4037|1.3452|1.3792|201|
|Repetitive|300|100|RK|1.4468|1.4780|1.3839|1.4468|201|
|Random|1000|333|Naive|1.3706|1.3780|1.4297|1.3780|0|
|Random|1000|333|KMP|1.3868|1.3722|1.4331|1.3868|0|
|Random|1000|333|RK|1.4303|1.4758|1.3935|1.4303|0|
|Repetitive|1000|333|Naive|1.5214|1.5206|1.5374|1.5214|668|
|Repetitive|1000|333|KMP|1.5757|1.4785|1.5973|1.5757|668|
|Repetitive|1000|333|RK|1.7195|1.6415|1.7086|1.7086|668|
|Random|3000|1000|Naive|1.5056|1.5163|1.4241|1.5056|0|
|Random|3000|1000|KMP|1.5213|1.5298|1.4227|1.5213|0|
|Random|3000|1000|RK|1.5092|1.5012|1.5900|1.5092|0|
|Repetitive|3000|1000|Naive|2.2793|2.2308|2.2133|2.2308|2001|
|Repetitive|3000|1000|KMP|1.7480|1.7691|1.7612|1.7612|2001|
|Repetitive|3000|1000|RK|2.3911|2.2063|2.3063|2.3063|2001|

At these sizes, process and JSON overhead is visible, especially for random cases where few characters match. On repetitive text, naive performs more character comparisons and is slower at the largest tested size. KMP remains linear in the scan size. The chosen repetitive pattern also makes most rolling hashes candidates, so RK must verify many windows. These small timings illustrate this workload only; they do not prove asymptotic bounds. With modulus 101, collisions are intentionally easy to create, so exact character verification is essential and the measurements should not be generalized to larger production hashes.

## Sources and AI assistance

- Assignment contracts, rubric, testing and submission requirements: the official `RZhaniya/daa-assignment-4-archive` repository included with this template.
- AI assistance: OpenAI Codex helped implement the C++ algorithms, prepare this analysis and create student tests/benchmark code.
- Verification performed: the C++ implementation compiled with the template's C++17 command; all 44 public cases and the repeated mixed batch passed; the student fixtures, seeded differential search checks and 100,000-vertex chain passed; benchmark outputs were checked for agreement before recording. I should review and understand the code and rerun the commands before submitting.
