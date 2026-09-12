template <__int128_t mod>
struct ModLL {
    __int128_t val;

    ModLL() : val(0) {
    }

    ModLL(int64_t y) : val(y >= 0 ? y % mod : (mod - (-y) % mod) % mod) {
    }

    ModLL &operator+=(const ModLL &p) {
        if ((val += p.val) >= mod) val -= mod;
        return *this;
    }

    ModLL &operator-=(const ModLL &p) {
        if ((val += mod - p.val) >= mod) val -= mod;
        return *this;
    }

    ModLL &operator*=(const ModLL &p) {
        val = (int)(1LL * val * p.val % mod);
        return *this;
    }

    ModLL &operator/=(const ModLL &p) {
        *this *= p.inverse();
        return *this;
    }

    ModLL operator-() const {
        return ModLL(-val);
    }

    ModLL operator+(const ModLL &p) const {
        return ModLL(*this) += p;
    }

    ModLL operator-(const ModLL &p) const {
        return ModLL(*this) -= p;
    }

    ModLL operator*(const ModLL &p) const {
        return ModLL(*this) *= p;
    }

    ModLL operator/(const ModLL &p) const {
        return ModLL(*this) /= p;
    }

    bool operator==(const ModLL &p) const {
        return val == p.val;
    }

    bool operator!=(const ModLL &p) const {
        return val != p.val;
    }
    bool operator<(const ModLL &p) const {
        return val < p.val;
    }
    bool operator>(const ModLL &p) const {
        return val > p.val;
    }

    ModLL inverse() const {
        __int128_t a = val, b = mod, u = 1, v = 0, t;
        while (b > 0) {
            t = a / b;
            swap(a -= t * b, b);
            swap(u -= t * v, v);
        }
        return ModLL(u);
    }

    ModLL pow(__int128_t n) const {
        ModLL ret(1), mul(val);
        while (n > 0) {
            if (n & 1) ret *= mul;
            mul *= mul;
            n >>= 1;
        }
        return ret;
    }

    friend ostream &operator<<(ostream &os, const ModLL &p) {
        return os << p.val;
    }

    friend istream &operator>>(istream &is, ModLL &a) {
        __int128_t t;
        is >> t;
        a = ModLL<mod>(t);
        return (is);
    }

    static int get_mod() {
        return mod;
    }
};
const __int128_t mod = 5683713593266136297;
using mint = ModLL<mod>;
