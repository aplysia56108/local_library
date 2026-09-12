/*
https://ei1333.github.io/library/string/aho-corasick.hpp.html
を参考に

add(string)で文字列追加。自動で0-indexでidは振られるが、第二引数にidを指定することもできる
build()で構築
match(string): 事前にaddされた各idの文字列が、stringの中に何個ずつ含まれているのかをunordered_mapで返す
move(string): firstに、事前にaddされた各idの文字列が合計何個含まれるかを持つ

verify: https://atcoder.jp/contests/abc362/submissions/55618247
*/
template <int char_size>
struct TrieNode {
    int nxt[char_size];

    int exist;
    vector<int> accept;

    TrieNode() : exist(0) {
        memset(nxt, -1, sizeof(nxt));
    }
};

template <int char_size, int margin>
struct Trie {
    using Node = TrieNode<char_size>;

    vector<Node> nodes;
    int root;

    Trie() : root(0) {
        nodes.push_back(Node());
    }

    void update_direct(int node, int id) {
        nodes[node].accept.push_back(id);
    }

    void update_child(int node, int child, int id) {
        ++nodes[node].exist;
    }

    void add(const string &str, int str_index, int node_index, int id) {
        if(str_index == str.size()) {
            update_direct(node_index, id);
        } else {
            const int c = str[str_index] - margin;
            if(nodes[node_index].nxt[c] == -1) {
                nodes[node_index].nxt[c] = (int)nodes.size();
                nodes.push_back(Node());
            }
            add(str, str_index + 1, nodes[node_index].nxt[c], id);
            update_child(node_index, nodes[node_index].nxt[c], id);
        }
    }

    void add(const string &str, int id) {
        add(str, 0, 0, id);
    }

    void add(const string &str) {
        add(str, nodes[0].exist);
    }

    void query(const string &str, const function<void(int)> &f, int str_index,
               int node_index) {
        for(auto &idx : nodes[node_index].accept) f(idx);
        if(str_index == str.size()) {
            return;
        } else {
            const int c = str[str_index] - margin;
            if(nodes[node_index].nxt[c] == -1) return;
            query(str, f, str_index + 1, nodes[node_index].nxt[c]);
        }
    }

    void query(const string &str, const function<void(int)> &f) {
        query(str, f, 0, 0);
    }

    int count() const {
        return (nodes[0].exist);
    }

    int size() const {
        return ((int)nodes.size());
    }
};

/**
 * @brief Aho-Corasick(エイホ–コラシック法)
 *
 */
template <int char_size, int margin>
struct AhoCorasick : Trie<char_size + 1, margin> {
    using Trie<char_size + 1, margin>::Trie;

    const int FAIL = char_size;
    vector<int> correct;

    void build(bool heavy = true) {
        correct.resize(this->size());
        for(int i = 0; i < this->size(); i++) {
            correct[i] = (int)this->nodes[i].accept.size();
        }
        queue<int> que;
        for(int i = 0; i <= char_size; i++) {
            if(~this->nodes[0].nxt[i]) {
                this->nodes[this->nodes[0].nxt[i]].nxt[FAIL] = 0;
                que.emplace(this->nodes[0].nxt[i]);
            } else {
                this->nodes[0].nxt[i] = 0;
            }
        }
        while(!que.empty()) {
            auto &now = this->nodes[que.front()];
            int fail = now.nxt[FAIL];
            correct[que.front()] += correct[fail];
            que.pop();
            for(int i = 0; i < char_size; i++) {
                if(~now.nxt[i]) {
                    this->nodes[now.nxt[i]].nxt[FAIL] =
                        this->nodes[fail].nxt[i];
                    if(heavy) {
                        auto &u = this->nodes[now.nxt[i]].accept;
                        auto &v = this->nodes[this->nodes[fail].nxt[i]].accept;
                        vector<int> accept;
                        set_union(begin(u), end(u), begin(v), end(v),
                                  back_inserter(accept));
                        u = accept;
                    }
                    que.emplace(now.nxt[i]);
                } else {
                    now.nxt[i] = this->nodes[fail].nxt[i];
                }
            }
        }
    }

    // もとのライブラリではO(cntの総和)かかっていたが、各nodeを訪れた回数をメモしておいてあとでまとめて足すことでO(sum(|T|))を達成
    // さらに、元のライブラリではmapを返していたが、unordered_mapにすることで高速化
    unordered_map<int, int> match(const string &str, int now = 0) {
        unordered_map<int, int> result;
        unordered_map<int, int> visit_cnt;
        for(auto &c : str) {
            now = this->nodes[now].nxt[c - margin];
            visit_cnt[now]++;
        }

        for(auto &[now, cnt] : visit_cnt) {
            for(auto &v : this->nodes[now].accept) result[v] += cnt;
        }
        return result;
    }

    pair<int64_t, int> move(const char &c, int now = 0) {
        now = this->nodes[now].nxt[c - margin];
        return {correct[now], now};
    }

    pair<int64_t, int> move(const string &str, int now = 0) {
        int64_t sum = 0;
        for(auto &c : str) {
            auto nxt = move(c, now);
            sum += nxt.first;
            now = nxt.second;
        }
        return {sum, now};
    }
};

/*
https://atcoder.jp/contests/abc268/submissions/34765119
を参考に

Aho_Corasick():コンストラクタ
add(string):文字列を追加
init():初期化
opertor(int v,char c):頂点番号vの状態に次の文字cを入れた次の頂点番号
operator[int v]:cnt[v]
*/
// struct Aho_Corasick {
//     using MP = unordered_map<char, int>;
//     vector<MP> to;
//     vector<int> cnt, fail;
//     Aho_Corasick() : to(1), cnt(1) {
//     }
//     int add(const string &s) {
//         int v = 0;
//         for (char c : s) {
//             if (!to[v].count(c)) {
//                 to[v][c] = to.size();
//                 to.push_back(MP());
//                 cnt.push_back(0);
//             }
//             v = to[v][c];
//         }
//         cnt[v]++;
//         return v;
//     }
//     void init() {
//         fail = vector<int>(to.size(), -1);
//         queue<int> q;
//         q.push(0);
//         while (!q.empty()) {
//             int v = q.front();
//             q.pop();
//             for (auto [c, u] : to[v]) {
//                 fail[u] = (*this)(fail[v], c);
//                 cnt[u] += cnt[fail[u]];
//                 q.push(u);
//             }
//         }
//     }
//     int operator()(int v, char c) const {
//         while (v != -1) {
//             auto it = to[v].find(c);
//             if (it != to[v].end()) return it->second;
//             v = fail[v];
//         }
//         return 0;
//     }
//     int operator[](int v) const {
//         return cnt[v];
//     }
// };
