/*
蟻本p226より
二次元構造体P(double x,double)は,
dot:内積
det:外積
ort:象限

比較演算子はx座標の昇順。偏角ソートは#define Declination_sortをオンに

bool on_seg(P p1,P p2,P q)：線分p1-p2上に点qがあるか判定
P intersection(P p1,P p2,P q1,P q2):直線p1-p2と直線q1-q2の交点
bool parallel(P p1, P p2, P q1, P q2):直線p1-p2と直線q1-q2が並行かどうか
double dist(P p,P q):点p、点qの距離
double angle(P p1, P p2, P p3):∠P1P2P3をラジアンで返す。
vector<P> convext_hull(P*ps,int n):凸包を求める。(nは頂点数)

円：
https://atcoder.jp/contests/abc157/submissions/10474498
を参考に
Circle(o,r):コンストラクタ（中心座標　o,半径　r）
crossPoint(c):cとの交点を返す。中心が同じ時は何も返さない
isInsice(v):点vを内部に含むか否か
*/

// 二次元ベクトル構造体
// #define Declination_sort  // 偏角ソート
struct P {
    double x, y;
    P(double x = 0, double y = 0) : x(x), y(y) {
    }

    double add(double a, double b) const {
        if (abs(a + b) < eps * (abs(a) + abs(b))) return 0;
        return a + b;
    }
    int ort() const {
        if (y > 0)
            return x > 0 ? 1 : 2;
        else
            return x > 0 ? 4 : 3;
    }

    double norm2() const {
        return x * x + y * y;
    }
    double norm() const {
        return sqrt(norm2());
    }
    P rotate90() const {
        return P(y, -x);
    }

    P operator+(P p) const {
        return P(add(x, p.x), add(y, p.y));
    }
    P &operator+=(const P &other) {
        *this = (*this) + other;
        return *this;
    }
    P operator-(P p) const {
        return P(add(x, -p.x), add(y, -p.y));
    }
    P &operator-=(const P &other) {
        *this = (*this) - other;
        return *this;
    }
    P operator*(double d) const {
        return P(x * d, y * d);
    }
    P &operator*=(const double d) {
        *this = (*this) * d;
        return *this;
    }
    P operator/(double d) const {
        return P(x / d, y / d);
    }
    P &operator/=(double d) {
        *this = (*this) / d;
        return *this;
    }
    double dot(P p) const {  // 内積
        return add(x * p.x, y * p.y);
    }
    double det(P p) const {  // 外積
        return add(x * p.y, -y * p.x);
    }
    bool operator==(const P &other) const {
        if (abs(x - other.x) < eps && abs(y - other.y) < eps) return true;
        return false;
    }
    bool operator!=(const P &other) const {
        if ((*this) == other) return false;
        return true;
    }

#ifdef Declination_sort
    // 偏角ソート 同一直線上は中心が近いものが先
    bool operator<(const P &p) const {
        int o = ort(), vo = p.ort();
        if (o != vo) return o < vo;
        return det(p) > 0;
    }
    bool operator>(const P &p) const {
        int o = ort(), vo = p.ort();
        if (o != vo) return o > vo;
        return det(p) < 0;
    }
#else
    bool operator<(const P &other) const {
        if (abs(x - other.x) > eps) return x < other.x;
        return y < other.y;
    }
    bool operator>(const P &other) const {
        if (abs(x - other.x) > eps) return x > other.x;
        return y > other.y;
    }
#endif

    // 線分p1-p2上に点qがあるか判定
    friend bool on_seg(P p1, P p2, P q) {
        return (p1 - q).det(p2 - q) == 0 && (p1 - q).dot(p2 - q) <= 0;
    }

    // 直線p1-p2と直線q1-q2の交点
    friend P intersection(P p1, P p2, P q1, P q2) {
        return p1 +
               (p2 - p1) * ((q2 - q1).det(q1 - p1) / (q2 - q1).det(p2 - p1));
    }

    // 直線p1-p2と直線q1-q2が並行かどうか
    friend bool parallel(P p1, P p2, P q1, P q2) {
        return (p1 - p2).det(q1 - q2) == 0;
    }

    // 距離の二乗を求める
    friend double dist2(P p, P q) {
        return (p - q).dot(p - q);
    }
    friend double dist(P p, P q) {
        return sqrt(dist2(p, q));
    }

    // 反時計回りにx回転
    friend P rotate(P p, double x) {
        P res;
        res.x = p.x * cos(x) - p.y * sin(x);
        res.y = p.x * sin(x) + p.y * cos(x);
        return res;
    }

    // ∠P1 P2 P3をラジアンで返す。（度数変換は180かけてM_PIで割る）
    friend double angle(P p1, P p2, P p3) {
        double d1 = sqrt(dist(p1, p2));
        double d2 = sqrt(dist(p2, p3));
        double c = ((p1 - p2).dot(p3 - p2)) / (d1 * d2);
        return acos(c);
    }

    // 凸包を求める。
    friend vector<P> convex_hull(vector<P> ps) {
        int n = ps.size();
        sort(ps.begin(), ps.end());
        int k = 0;
        vector<P> qs(n * 2);
        for (int i = 0; i < n; i++) {
            while (k > 1 && (qs[k - 1] - qs[k - 2]).det(ps[i] - qs[k - 1]) <= 0)
                k--;
            qs[k++] = ps[i];
        }
        for (int i = n - 2, t = k; i >= 0; i--) {
            while (k > t && (qs[k - 1] - qs[k - 2]).det(ps[i] - qs[k - 1]) <= 0)
                k--;
            qs[k++] = ps[i];
        }
        qs.resize(k - 1);
        return qs;
    }
    friend istream &operator>>(istream &is, P &p) {
        is >> p.x >> p.y;
        return is;
    }

    friend ostream &operator<<(ostream &os, const P &p) {
        os << p.x << " " << p.y << endl;
        return (os);
    }
};

struct Circle {
    P o;
    double r;
    Circle(P o = P(), double r = 0) : o(o), r(r) {
    }
    bool equal(double a, double b) {
        return abs(a - b) < eps;
    }
    vector<P> crossPoint(const Circle &c) {
        P v = c.o - o;
        double l = v.norm();
        if (equal(l, 0)) return {};
        if (equal(l + r + c.r, max({l, r, c.r}) * 2)) {
            if (equal(l + r, c.r)) return {o - v * (r / l)};
            return {o + v * (r / l)};
        }
        if (l + r + c.r < max({l, r, c.r}) * 2) return {};
        double x = (l * l + r * r - c.r * c.r) / (2 * l);
        double y = sqrt(r * r - x * x);
        P mid = o + v * (x / l);
        v = v.rotate90();
        return {mid + v * (y / l), mid - v * (y / l)};
    }

    bool isInside(const P &p) const {
        return (p - o).norm() < r + eps;
    }
};

// https://youtu.be/UWbGRhF3Ozw?t=9564
/*
distLP(P p):点と直線の距離
*/
struct Line {
    P s, t;
    Line(P s = P(), P t = P()) : s(s), t(t) {
    }
    P dir() {
        return t - s;
    }
    double norm() {
        return dir().norm();
    }
    /* +1: s-t,s-p : ccw(反時計回り)
     * -1: s-t,s-p : cw(時計回り)
     * +2: t-s-p : s!=p
     * -2: s-t-p :t!=p
     *  0: s-p-t */
    int ccw(P p) {
        if (dir().det(p - s) > eps) return +1;
        if (dir().det(p - s) < -eps) return -1;
        if (dir().dot(p - s) < -eps) return +2;
        if (dir().norm2() + eps < (p - s).norm2()) return -2;
        return 0;
    }
    double distLP(P p) {
        return abs(dir().det(p - s) / norm());
    }
};
