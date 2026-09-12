/*
https://github.com/drken1215/algorithm/blob/master/String/manacher.cpp
を参考に

文字列Sの各indexに対し、その点を中心にした回文の最長半径を求める。O(|S|)

Manachar mn(S):コンストラクタ。O(|S|)
get_odd(i):i文字目が中心の最長回文の半径。O(1)
get_even(i):i-1とi番目の間を中心とした最長回文の半径。O(1)
isplain(left,right):[left,right]が回文かどうか。O(1)
                    ちょうど半径かを調べていた既存のものを変更
                    開区間から閉区間に変更
max_length():最大の長さを返す。
*/

struct Manacher {
    string S;
    vector<int> len;

    // construct
    Manacher(const string &iS) {
        init(iS);
    }
    void init(const string &iS) {
        S = "";
        for (int i = 0; i < (int)iS.size(); ++i) {
            S += iS[i];
            if (i + 1 < (int)iS.size()) S += "$";
        }
        construct();
    }
    void construct() {
        int N = (int)S.size();
        len.resize(N);
        int i = 0, j = 0;
        while (i < N) {
            while (i - j >= 0 && i + j < N && S[i - j] == S[i + j]) ++j;
            len[i] = j;
            int k = 1;
            while (i - k >= 0 && i + k < N && k + len[i - k] < j)
                len[i + k] = len[i - k], ++k;
            i += k, j -= k;
        }
    }

    // radius, center is i
    inline int get_odd(int i) {
        return (len[i * 2] + 1) / 2;
    }

    // radius, center is between i-1 and i
    inline int get_even(int i) {
        return len[i * 2 - 1] / 2;
    }

    // judge if [left, right] is palindrome
    inline bool ispalin(int left, int right) {
        if (left > right) return 0;
        right++;  //オリジナルから変更
        int mid = (left + right) / 2;
        if ((right - left) & 1)
            return (get_odd(mid) >= (right - left + 1) / 2);
        else
            return (get_even(mid) >= (right - left) / 2);
    }

    inline int max_length() {
        int res = 0;
        int N = (int)S.size();
        for (int i = 0; i < (N + 1) / 2; i++) {
            res = max(res, get_odd(i) * 2 - 1);
            if (i > 0) res = max(res, get_even(i) * 2);
        }
        return res;
    }
};
