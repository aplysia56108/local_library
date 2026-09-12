/*
https://qiita.com/Sooh31/items/97e2d9724fe47ed238b3
を参考に

数列aのxor基底を返す。
*/

//掃き出し法　xor基底を返す。
vector<int> xor_base(vector<int> &a) {
    vector<int> res;
    for (int v : a) {
        for (int e : res) {
            v = min(v, v ^ e);
        }

        if (v) {
            for (auto &e : res) {
                e = min(e, e ^ v);
            }
            res.push_back(v);
        }
    }
    return res;
}

// aの中で線形独立な要素を返す。
vector<int> liniary_independent(vector<int> &a) {
    vector<int> res;
    for (int v : a) {
        int v2 = v;
        for (int e : res) {
            v = min(v, v ^ e);
        }
        if (v) res.push_back(v2);
    }
    return res;
}

/*
自作
add(ll x):xを追加
check(ll x):xが基底で表現できるか
*/

// struct Base {
//     vi base;
//     int MX = 60;
//     void add(long long x) {
//         base.pb(x);
//         int rank = 0;
//         rrep(i, MX) {
//             int piv = -1;
//             rep(j, rank, base.size()) {
//                 if (base[j] >> i & 1) {
//                     piv = j;
//                     break;
//                 }
//             }
//             if (piv == -1) continue;
//             swap(base[rank], base[piv]);
//             rep(j, base.size()) {
//                 if (j == rank) continue;
//                 if (base[j] >> i & 1) {
//                     base[j] ^= base[rank];
//                 }
//             }
//             rank++;
//         }
//         while (base.size() && base.back() == 0) base.pop_back();
//     }
//     bool check(long long x) {
//         rrep(i, MX) {
//             if (!(x >> i & 1)) continue;
//             rep(j, base.size()) {
//                 if (base[j] >> i & 1) {
//                     x ^= base[j];
//                     break;
//                 }
//             }
//         }
//         if (x) return false;
//         return true;
//     }
// };
