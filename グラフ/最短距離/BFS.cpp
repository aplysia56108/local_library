//全ての距離が1のグラフの最短距離。(g:グラフ、s:スタート地点)
vector<int> BFS(vector<vector<int>> g, int s) {
    int n = g.size();
    long long INF = 1e18;
    vector<int> dp(n, INF);
    dp[s] = 0;
    queue<int> q;
    q.push(s);
    while (!q.empty()) {
        int p = q.front();
        q.pop();
        for (int c : g[p]) {
            if (dp[c] == INF) {
                dp[c] = dp[p] + 1;
                q.push(c);
            }
        }
    }
    return dp;
}