/*
座標圧縮
https://youtu.be/fR3W5IcBGLQ?t=8550
を参考に

(T x):xが何番目か
[T i]:i番目の値
*/
template <typename T = int>
struct CC {
    bool initialized;
    vector<T> xs;
    CC() : initialized(false) {
    }
    CC(vector<T> v) : initialized(false) {
        for (auto x : v) xs.push_back(x);
    }
    void add(T x) {
        xs.push_back(x);
    }
    void add(vector<T> v) {
        for (auto x : v) xs.push_back(x);
    }
    void init() {
        sort(xs.begin(), xs.end());
        xs.erase(unique(xs.begin(), xs.end()), xs.end());
        initialized = true;
    }
    int operator()(T x) {
        if (!initialized) init();
        return upper_bound(xs.begin(), xs.end(), x) - xs.begin() - 1;
    }
    T operator[](int i) {
        if (!initialized) init();
        return xs[i];
    }
    int size() {
        if (!initialized) init();
        return xs.size();
    }
    friend ostream &operator<<(ostream &os, const CC &cc) {
        for (int i = 0; i < (int)cc.xs.size(); i++) {
            os << cc.xs[i] << " ";
        }
        os << endl;
        return (os);
    }
};
