//https://qiita.com/mdstoy/items/39104b686540c5f1dc6cを参考に
//初期配置をソートすることに注意
//実行すると、先頭k個が辞書順で次のcombinationに更新される
//計算量はならしO(comb(n, k))になっている(はず？)
//verify: https://atcoder.jp/contests/abc328/submissions/47529384

template <typename T> bool next_combination(const T first, const T last, int k) {
    const T subset = first + k;
    // empty container | k = 0 | k == n 
    if (first == last || first == subset || last == subset) {
        return false;
    }
    T src = subset;
    while (first != src) {
        src--;
        if (*src < *(last - 1)) {
            T dest = subset;
            while (*src >= *dest) {
                dest++;
            }
            iter_swap(src, dest);
            rotate(src + 1, dest + 1, last);
            rotate(subset, subset + (last - dest) - 1, last);
            return true;
        }
    }
    // restore
    rotate(first, subset, last);
    return false;
}
