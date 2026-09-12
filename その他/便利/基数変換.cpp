// 十進数からb進数へ
template <class T>
string to_baseB(T x, int b = 10) {
    string ans;
    do {
        int num = x % b;
        ans = (char)((num <= 9) ? ('0' + num) : ('A' + num - 10)) + ans;
        x /= b;
    } while (x != 0);
    return ans;
}
// b進数から十進数へ
long long to_base10(const string &x, int b = 10) {
    long long ans = 0, base = 1;
    for (int i = x.length() - 1; i >= 0; --i) {
        int num =
            ('0' <= x[i] && x[i] <= '9') ? (x[i] - '0') : (x[i] - 'A' + 10);
        ans += base * num;
        base *= b;
    }
    return ans;
}
