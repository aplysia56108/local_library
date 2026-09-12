/*
https://ei1333.github.io/luzhiled/snippets/graph/grid-bfs.html
を参考に
grid_bfs(s,x,y,wall):(x,y)から全てのマスの最短距離を求める。wallは壁の文字
O(HW)
*/

vector<vector<int>> grid_bfs(vector<vector<char>> &s, int x, int y,
                             const char wall = '#') {
    const int vx[] = {0, 1, 0, -1}, vy[] = {1, 0, -1, 0};
    vector<vector<int>> min_cost(s.size(), vector<int>(s[0].size(), -1));
    queue<pair<int, int>> que;

    que.emplace(x, y);
    min_cost[x][y] = 0;

    while (!que.empty()) {
        auto p = que.front();
        que.pop();
        for (int i = 0; i < 4; i++) {
            int ny = p.first + vy[i], nx = p.second + vx[i];
            if (nx < 0 || ny < 0 || nx >= s[0].size() || ny >= s.size())
                continue;
            if (min_cost[ny][nx] != -1) continue;
            if (s[ny][nx] == wall) continue;
            min_cost[ny][nx] = min_cost[p.first][p.second] + 1;
            que.emplace(ny, nx);
        }
    }
    return min_cost;
}