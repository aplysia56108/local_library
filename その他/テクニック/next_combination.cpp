/*
https://qiita.com/drken/items/7c6ff2aa4d8fce1c9361 を参考に
k=0の時は動作しない
*/

int next_combination(int sub) {
    int x = sub & -sub, y = sub + x;
    return (((sub & ~y) / x) >> 1) | y;
}

int main() {
    int n = 5;  // {0, 1, 2, 3, 4} の部分集合を考える
    int k = 3;

    int bit = (1 << k) - 1;  // bit = {0, 1, 2}
    for (; bit < (1 << n); bit = next_combination(bit)) {
        /*
        こんな感じの処理
        vector<int> S;
        for (int i = 0; i < n; ++i) {
            if (bit & (1<<i)) { // i が bit に入るかどうか
                S.push_back(i);
            }
        }
        */
    }
}