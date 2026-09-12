template <class T>
vector<T> rotate90(const vector<T> &mat) {
    int m = mat.size(), n = mat[0].size();
    vector<T> ans(n);
    for (int i = 0; i < m; i++)
        for (int j = 0; j < n; j++) ans[j].push_back(mat[m - 1 - i][j]);
    return ans;
}
