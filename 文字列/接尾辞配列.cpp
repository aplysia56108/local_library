/*
蟻本p336

SuffixArray(string s):文字列sの接尾辞配列のコンストラクタ構成。O(n log^2 n)
bool contain(string t):文字列tがsに含まれるか。O(|T| log |S|)
void construct_lcp():高さ配列lcpを構築
*/

struct SuffixArray {
    string s;
    int n, k;
    vector<int> rank;
    vector<int> tmp;
    vector<int> sa;
    vector<int> lcp;

    SuffixArray(string S)
        : s(S),
          n(S.size()),
          rank(S.size() + 1),
          tmp(S.size() + 1),
          sa(S.size() + 1),
          lcp(S.size() + 1) {
        for (int i = 0; i <= n; i++) {
            sa[i] = i;
            rank[i] = i < n ? s[i] : -1;
        }

        for (k = 1; k <= n; k *= 2) {
            sort(sa.begin(), sa.end(), [&](int i, int j) {
                if (rank[i] != rank[j])
                    return rank[i] < rank[j];
                else {
                    int ri = i + k < n ? rank[i + k] : -1;
                    int rj = j + k < n ? rank[j + k] : -1;
                    return ri < rj;
                }
            });

            tmp[sa[0]] = 0;
            for (int i = 1; i <= n; i++) {
                tmp[sa[i]] =
                    tmp[sa[i - 1]] + (compare_sa(sa[i - 1], sa[i]) ? 1 : 0);
            }
            for (int i = 0; i <= n; i++) {
                rank[i] = tmp[i];
            }
        }
    };

    bool compare_sa(int i, int j) {
        if (rank[i] != rank[j])
            return rank[i] < rank[j];
        else {
            int ri = i + k < n ? rank[i + k] : -1;
            int rj = j + k < n ? rank[j + k] : -1;
            return ri < rj;
        }
    }

    bool contain(string t) {
        int a = 0, b = n;
        while (b - a > 1) {
            int c = (a + b) / 2;
            if (s.compare(sa[c], t.length(), t) < 0)
                a = c;
            else
                b = c;
        }
        return s.compare(sa[b], t.length(), t) == 0;
    }

    void construct_lcp() {
        for (int i = 0; i <= n; i++) rank[sa[i]] = i;

        int h = 0;
        lcp[0] = 0;
        for (int i = 0; i < n; i++) {
            int j = sa[rank[i] - 1];
            if (h > 0) h--;
            for (; j + h < n && i + h < n; h++) {
                if (s[j + h] != s[i + h]) break;
            }
            lcp[rank[i] - 1] = h;
        }
    }
};