/*
https://ei1333.github.io/luzhiled/snippets/other/random-number-generator.html
を参考に

RandomNumberGenerator rt;//コンストラクタ生成
rt(a,b):[a,b)で発生した乱数を返す
rt(a):[0,a)で発生した乱数を返す。
*/

struct RandomNumberGenerator {
    mt19937 mt;

    RandomNumberGenerator()
        : mt(chrono::steady_clock::now().time_since_epoch().count()) {
    }

    int operator()(int a, int b) {  // [a, b)
        uniform_int_distribution<int> dist(a, b - 1);
        return dist(mt);
    }

    int operator()(int b) {  // [0, b)
        return (*this)(0, b);
    }
};
