/*
https://37zigen.com/persistent-array/
を参考に

以下のような問題を処理するためのデータ構造
問題（部分永続配列）：𝑎0 を初期状態の配列とする。以下の 𝑇
個のクエリを処理するデータ構造を構築せよ。𝑖
個目のクエリでは次のいずれかを処理する。 𝑎𝑖−1[𝑝𝑖] を 𝑣𝑖 で更新した配列を 𝑎𝑖
とする。 𝑎𝑡𝑖[𝑝𝑖] を出力せよ。ただし 0≤𝑡𝑖<𝑖 とする。

配列を更新しても、更新時刻を覚えておくことで、その時刻におけるi番目の値を返すことができる。
PartialPersistentArray(vector<T> a):コンストラクタ
void set(int i,int val):a[i]<-valと更新。time+=1。O(1)
T get(int i,int time):時刻timeにおけるa[i]の値を返す。O(log Q)

*/

template <typename T>
struct PartialPersistentArray {
    vector<vector<pair<int, int>>> b;
    int last_time = 0;

    // 時刻 -1 における a を設定する。
    PartialPersistentArray(vector<T> a) {
        b.resize(a.size());
        for (int i = 0; i < (int)a.size(); ++i) {
            b[i].push_back(make_pair(-1, a[i]));
        }
    }

    // 時刻 last_time において、a[i] <- val と更新する。
    void set(int i, int val) {
        b[i].push_back(make_pair(last_time++, val));
    }

    // 時刻 time における a[i] の値を返す。
    T get(int i, int time) {
        int ok = 0, ng = b[i].size();
        while (ng - ok > 1) {
            int middle = (ok + ng) / 2;
            if (b[i][middle].first <= time) {
                ok = middle;
            } else {
                ng = middle;
            }
        }
        return b[i][ok].second;
    }
};