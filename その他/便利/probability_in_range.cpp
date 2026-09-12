// probability_in_range(l, r, mean, sigma): 平均mean, 標準偏差sigmaの正規分布上の[l, r]の面積割合を返す。(全体を1で規格化した値)

// std::erfと離籍密度関数の関係式　https://cpprefjp.github.io/reference/cmath/erf.html
// 参考にしたwataさんの提出（ライブラリは最後）https://atcoder.jp/contests/ahc030/submissions/50443474

constexpr double normal_cdf(double x, double mean = 0.0, double sigma = 1.0) {
    return 0.5 * (1.0 + std::erf((x - mean) / (sigma * 1.41421356237)));
}
constexpr double probability_in_range(double l, double r, double mean = 0.0,
                                      double sigma = 1.0) {
    assert(l <= r);
    if(mean < l)
        return probability_in_range(2.0 * mean - r, 2.0 * mean - l, mean,
                                    sigma);
    double p_l = normal_cdf(l, mean, sigma);
    double p_r = normal_cdf(r, mean, sigma);
    return p_r - p_l;
}
