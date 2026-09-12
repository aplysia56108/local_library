long long Catalan(int k) {
    return C(2 * k, k) - C(2 * k, k - 1);
}
