#pragma once
#include "json.hpp"

inline J lcs(const J& input) {
    string a = input["a"].text(), b = input["b"].text();
    int n = (int)a.size(), m = (int)b.size();
    vector<vector<int>> dp(n + 1, vector<int>(m + 1));
    for (int i = 1; i <= n; ++i) {
        for (int j = 1; j <= m; ++j) {
            if (a[i - 1] == b[j - 1]) dp[i][j] = dp[i - 1][j - 1] + 1;
            else dp[i][j] = max(dp[i - 1][j], dp[i][j - 1]);
        }
    }
    string sequence;
    int i = n, j = m;
    while (i > 0 && j > 0) {
        if (a[i - 1] == b[j - 1]) {
            sequence += a[i - 1];
            --i;
            --j;
        } else if (dp[i - 1][j] >= dp[i][j - 1]) --i;
        else --j;
    }
    reverse(sequence.begin(), sequence.end());
    return J::O{{"length", dp[n][m]}, {"sequence", sequence}};
}

inline J merge(const J& input) {
    const auto& values = input["sizes"].array();
    int n = (int)values.size();
    if (n == 0) return J::O{{"cost", 0}, {"plan", J::A{}}};
    vector<long long> prefix(n + 1);
    for (int i = 0; i < n; ++i) prefix[i + 1] = prefix[i] + values[i].number();
    vector<vector<long long>> dp(n, vector<long long>(n));
    vector<vector<int>> split(n, vector<int>(n, -1));
    for (int length = 2; length <= n; ++length) {
        for (int left = 0; left + length <= n; ++left) {
            int right = left + length - 1;
            dp[left][right] = LLONG_MAX;
            long long sum = prefix[right + 1] - prefix[left];
            for (int k = left; k < right; ++k) {
                long long candidate = dp[left][k] + dp[k + 1][right] + sum;
                if (candidate < dp[left][right]) {
                    dp[left][right] = candidate;
                    split[left][right] = k;
                }
            }
        }
    }
    J::A plan;
    function<void(int, int)> build = [&](int left, int right) {
        if (left == right) return;
        int k = split[left][right];
        build(left, k);
        build(k + 1, right);
        plan.push_back(J::A{left, k, right});
    };
    build(0, n - 1);
    return J::O{{"cost", dp[0][n - 1]}, {"plan", plan}};
}

inline J tree(const J& input) {
    int n = (int)input["n"].number();
    const auto& weights = input["weights"].array();
    const auto& edges = input["edges"].array();
    vector<vector<int>> graph(n);
    for (const auto& edge : edges) {
        int u = (int)edge.array()[0].number(), v = (int)edge.array()[1].number();
        graph[u].push_back(v);
        graph[v].push_back(u);
    }
    if (n == 0) return J::O{{"weight", 0}, {"selected", J::A{}}};
    vector<int> parent(n, -1), order{0};
    for (size_t i = 0; i < order.size(); ++i) {
        int u = order[i];
        for (int v : graph[u]) if (v != parent[u]) {
            parent[v] = u;
            order.push_back(v);
        }
    }
    vector<long long> take(n), skip(n);
    for (int idx = n - 1; idx >= 0; --idx) {
        int u = order[idx];
        take[u] = weights[u].number();
        for (int v : graph[u]) if (parent[v] == u) {
            take[u] += skip[v];
            skip[u] += max(take[v], skip[v]);
        }
    }
    J::A selected;
    vector<pair<int, bool>> stack{{0, false}};
    while (!stack.empty()) {
        auto [u, parentTaken] = stack.back();
        stack.pop_back();
        bool chosen = !parentTaken && take[u] > skip[u];
        if (chosen) selected.push_back(u);
        for (int v : graph[u]) if (parent[v] == u) stack.push_back({v, chosen});
    }
    return J::O{{"weight", max(take[0], skip[0])}, {"selected", selected}};
}

inline J naive(const J& input) {
    string text = input["text"].text(), pattern = input["pattern"].text();
    int n = (int)text.size(), m = (int)pattern.size();
    J::A positions;
    if (m == 0) for (int i = 0; i <= n; ++i) positions.push_back(i);
    else if (m <= n) for (int start = 0; start + m <= n; ++start) {
        int j = 0;
        while (j < m && text[start + j] == pattern[j]) ++j;
        if (j == m) positions.push_back(start);
    }
    return J::O{{"positions", positions}};
}

inline J kmp(const J& input) {
    string text = input["text"].text(), pattern = input["pattern"].text();
    int n = (int)text.size(), m = (int)pattern.size();
    J::A positions;
    if (m == 0) {
        for (int i = 0; i <= n; ++i) positions.push_back(i);
        return J::O{{"positions", positions}};
    }
    vector<int> pi(m);
    for (int i = 1, j = 0; i < m; ++i) {
        while (j > 0 && pattern[i] != pattern[j]) j = pi[j - 1];
        if (pattern[i] == pattern[j]) ++j;
        pi[i] = j;
    }
    for (int i = 0, j = 0; i < n; ++i) {
        while (j > 0 && text[i] != pattern[j]) j = pi[j - 1];
        if (text[i] == pattern[j]) ++j;
        if (j == m) {
            positions.push_back(i - m + 1);
            j = pi[j - 1];
        }
    }
    return J::O{{"positions", positions}};
}

inline J rk(const J& input) {
    string text = input["text"].text(), pattern = input["pattern"].text();
    int n = (int)text.size(), m = (int)pattern.size();
    J::A positions;
    if (m == 0) {
        for (int i = 0; i <= n; ++i) positions.push_back(i);
        return J::O{{"positions", positions}};
    }
    if (m > n) return J::O{{"positions", positions}};
    const long long base = 31, mod = 101;
    long long highest = 1, patternHash = 0, windowHash = 0;
    for (int i = 1; i < m; ++i) highest = highest * base % mod;
    for (int i = 0; i < m; ++i) {
        patternHash = (patternHash * base + (unsigned char)pattern[i]) % mod;
        windowHash = (windowHash * base + (unsigned char)text[i]) % mod;
    }
    for (int start = 0; start + m <= n; ++start) {
        if (windowHash == patternHash) {
            int matched = 0;
            while (matched < m && text[start + matched] == pattern[matched]) ++matched;
            if (matched == m) positions.push_back(start);
        }
        if (start + m < n) {
            long long removed = ((unsigned char)text[start] * highest) % mod;
            windowHash = (windowHash - removed + mod) % mod;
            windowHash = (windowHash * base + (unsigned char)text[start + m]) % mod;
        }
    }
    return J::O{{"positions", positions}};
}

inline J suffix(const J& input) {
    string text = input["text"].text();
    int n = (int)text.size();
    vector<int> sa(n), rank(n), nextRank(n);
    iota(sa.begin(), sa.end(), 0);
    for (int i = 0; i < n; ++i) rank[i] = (unsigned char)text[i];
    for (int width = 1; width < n; width *= 2) {
        sort(sa.begin(), sa.end(), [&](int a, int b) {
            if (rank[a] != rank[b]) return rank[a] < rank[b];
            int ra = a + width < n ? rank[a + width] : -1;
            int rb = b + width < n ? rank[b + width] : -1;
            return ra < rb;
        });
        nextRank[sa[0]] = 0;
        for (int i = 1; i < n; ++i) {
            int a = sa[i - 1], b = sa[i];
            bool different = rank[a] != rank[b] ||
                (a + width < n ? rank[a + width] : -1) != (b + width < n ? rank[b + width] : -1);
            nextRank[b] = nextRank[a] + different;
        }
        rank.swap(nextRank);
        if (rank[sa.back()] == n - 1) break;
        if (width > n / 2) break;
    }
    vector<int> position(n), lcp(n);
    for (int i = 0; i < n; ++i) position[sa[i]] = i;
    int common = 0;
    for (int i = 0; i < n; ++i) {
        int order = position[i];
        if (order == 0) {
            common = 0;
            continue;
        }
        int j = sa[order - 1];
        while (i + common < n && j + common < n && text[i + common] == text[j + common]) ++common;
        lcp[order] = common;
        if (common > 0) --common;
    }
    int best = 0, bestOrder = 0;
    for (int i = 1; i < n; ++i) if (lcp[i] > best) {
        best = lcp[i];
        bestOrder = i;
    }
    string repeated = best == 0 ? "" : text.substr(sa[bestOrder], best);
    J::A saJson, lcpJson;
    for (int x : sa) saJson.push_back(x);
    for (int x : lcp) lcpJson.push_back(x);
    return J::O{{"sa", saJson}, {"lcp", lcpJson}, {"repeated", repeated}};
}

inline J aho(const J& input) {
    struct Node {
        array<int, 95> next;
        int fail = 0;
        Node() { next.fill(-1); }
    };
    vector<Node> nodes(1);
    const auto& patterns = input["patterns"].array();
    const auto& texts = input["texts"].array();
    vector<int> terminal;
    for (const auto& patternValue : patterns) {
        string pattern = patternValue.text();
        int state = 0;
        for (unsigned char c : pattern) {
            int symbol = c - 32;
            if (nodes[state].next[symbol] == -1) {
                nodes[state].next[symbol] = (int)nodes.size();
                nodes.emplace_back();
            }
            state = nodes[state].next[symbol];
        }
        terminal.push_back(state);
    }
    queue<int> pending;
    for (int c = 0; c < 95; ++c) {
        int child = nodes[0].next[c];
        if (child == -1) nodes[0].next[c] = 0;
        else {
            nodes[child].fail = 0;
            pending.push(child);
        }
    }
    vector<int> order{0};
    while (!pending.empty()) {
        int state = pending.front();
        pending.pop();
        order.push_back(state);
        for (int c = 0; c < 95; ++c) {
            int child = nodes[state].next[c];
            if (child == -1) nodes[state].next[c] = nodes[nodes[state].fail].next[c];
            else {
                nodes[child].fail = nodes[nodes[state].fail].next[c];
                pending.push(child);
            }
        }
    }
    J::A rows;
    for (const auto& textValue : texts) {
        string text = textValue.text();
        vector<long long> visits(nodes.size());
        int state = 0;
        for (unsigned char c : text) {
            state = nodes[state].next[c - 32];
            ++visits[state];
        }
        for (int i = (int)order.size() - 1; i > 0; --i) {
            int node = order[i];
            visits[nodes[node].fail] += visits[node];
        }
        J::A row;
        for (int node : terminal) row.push_back(visits[node]);
        rows.push_back(row);
    }
    return J::O{{"counts", rows}};
}
