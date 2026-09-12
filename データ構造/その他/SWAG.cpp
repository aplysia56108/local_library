/*
https://atcoder.jp/contests/abc228/submissions/27419751
https://tkmst201.github.io/Library/DataStructure/SlidingWindowAggregation.hpp
を参考に

SlidingWindowAggregation
スライド最小値の拡張版みたいなイメージ。
+, xor, min, gcdなどモノイドに使える。
FIFOでpriority値等を返すデータ構造みたいな感じ。
計算量は全てならしO(1)

SWAG:コンストラクタ。ACLのセグメント木のように<型,op,id>がテンプレート引数に必要
    例）int型でminの時
    int op(int x, int y) {
        return min(x, y);
    }
    int id() {
        return inf;
    }
    SWAG<int, op, id> swag;

push(x):挿入
pop():取り出し
get():priority値等を返す。
top():先頭（次に取り出される値）を返す。
*/

template <class T, T (*op)(T, T), T (*e)()>
class SWAG {
   private:
    struct Stack {
        stack<T> a, data;
        T get() {
            return data.size() ? data.top() : e();
        }
        void push(T x) {
            a.push(x);
            data.push(op(x, get()));
        }
        T top() {
            assert(a.size());
            return a.top();
        }
        void pop() {
            a.pop();
            data.pop();
        }
        int size() {
            return a.size();
        }
    };

    Stack l, r;
    void mv() {
        while (r.size()) {
            l.push(r.top());
            r.pop();
        }
    }

   public:
    void push(T x) {
        r.push(x);
    }
    void pop() {
        if (!l.size()) mv();
        l.pop();
    }
    //次に取り出される値
    T top() {
        if (!l.size()) mv();
        return l.top();
    }
    // priority値等
    T get() {
        return op(l.get(), r.get());
    }
};
