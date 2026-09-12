// l以上r以下のmの倍数の数(number of multiples)
int nom(int l, int r, int m) {
    if (l > r) return 0;
    if (l <= 0) {
        int ad = (-l) / m + 1;
        ad *= m;
        l += ad;
        r += ad;
    }
    return r / m - (l - 1) / m;
}
