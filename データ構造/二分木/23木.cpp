/** structure: 2-3 tree
 * available for both set and multiple set (see constructor)
 * every method works in O(logn)
 * tests: { https://judge.yosupo.jp/submission/105334,
 *          https://atcoder.jp/contests/abc241/submissions/35040063
 *        }
 *
 * コンストラクタの引数にtrueを入れることでmultisetに
 * insert.erase,lower_bound,upper_bound(配列番号),size,count,[]
 */

template <class T>
class twothreetree {
   private:
    struct node {
        node *parent = nullptr;
        unsigned char position;
        unsigned char n = 0;
        unsigned size = 0;
        T keys[2];
        node *children[3] = {nullptr, nullptr, nullptr};
        node() {
        }

        unsigned char LB(T key) {
            for (unsigned char i = 0; i < n; i++)
                if (key <= keys[i]) return i;
            return n;
        }

        unsigned char UB(T key) {
            for (unsigned char i = 0; i < n; i++)
                if (key < keys[i]) return i;
            return n;
        }

        void insert(unsigned char i, T key, node *child = nullptr) {
            if (n < 2) {
                if (i == 0) {
                    keys[1] = keys[0];
                    children[2] = children[1];
                }
                keys[i] = key;
                children[i + 1] = child;
                n++;
                coordinate();
                if (parent == nullptr) return;
                parent->sizeIncrement();
                return;
            }
            node *newNode = new node();
            T newKey;
            if (i == 0) {
                newNode->keys[0] = keys[1];
                newNode->children[0] = children[1];
                newNode->children[1] = children[2];
                newKey = keys[0];
                keys[0] = key;
                children[1] = child;
            } else if (i == 1) {
                newNode->keys[0] = keys[1];
                newNode->children[0] = child;
                newNode->children[1] = children[2];
                newKey = key;
            } else {
                newNode->keys[0] = key;
                newNode->children[0] = children[2];
                newNode->children[1] = child;
                newKey = keys[1];
            }
            children[2] = nullptr;
            newNode->n = 1;
            n = 1;
            newNode->coordinate();
            coordinate();
            if (parent == nullptr) {
                node *newRoot = new node();
                newRoot->keys[0] = newKey;
                newRoot->children[0] = this;
                newRoot->children[1] = newNode;
                newRoot->n = 1;
                newRoot->coordinate();
                return;
            }
            parent->insert(position, newKey, newNode);
        }

        void erase(unsigned char i) {
            delete children[i + 1];
            children[i + 1] = nullptr;
            if (n == 2) {
                if (i == 0) {
                    keys[0] = keys[1];
                    children[1] = children[2];
                    children[2] = nullptr;
                }
                n--;
                this->coordinate();
                if (parent == nullptr) return;
                parent->sizeDecrement();
                return;
            }
            if (parent == nullptr) {
                n--;
                size--;
                return;
            }
            if (position > 0) {
                node *leftNode = parent->children[position - 1];
                if (leftNode->n == 2) {
                    keys[0] = parent->keys[position - 1];
                    parent->keys[position - 1] = leftNode->keys[1];
                    children[1] = children[0];
                    children[0] = leftNode->children[2];
                    leftNode->children[2] = nullptr;
                    leftNode->n = 1;
                    n = 1;
                    leftNode->coordinate();
                    coordinate();
                    if (parent == nullptr) return;
                    parent->sizeDecrement();
                    return;
                }

                leftNode->keys[1] = parent->keys[position - 1];
                leftNode->children[2] = children[0];
                leftNode->n = 2;
                leftNode->coordinate();
                parent->erase(position - 1);
                return;
            }
            node *rightNode = parent->children[1];
            if (rightNode->n == 2) {
                keys[0] = parent->keys[0];
                parent->keys[0] = rightNode->keys[0];
                children[1] = rightNode->children[0];
                rightNode->keys[0] = rightNode->keys[1];
                rightNode->children[0] = rightNode->children[1];
                rightNode->children[1] = rightNode->children[2];
                rightNode->children[2] = nullptr;
                rightNode->n = 1;
                n = 1;
                rightNode->coordinate();
                coordinate();
                if (parent == nullptr) return;
                parent->sizeDecrement();
                return;
            }
            rightNode->keys[1] = rightNode->keys[0];
            rightNode->keys[0] = parent->keys[position];
            rightNode->children[2] = rightNode->children[1];
            rightNode->children[1] = rightNode->children[0];
            rightNode->children[0] = children[0];
            rightNode->n = 2;
            rightNode->coordinate();
            parent->children[0] = rightNode;
            parent->children[1] = this;
            rightNode->position = 0;
            parent->erase(0);
        }

        void sizeIncrement() {
            size++;
            if (parent != nullptr) parent->sizeIncrement();
        }

        void sizeDecrement() {
            size--;
            if (parent != nullptr) parent->sizeDecrement();
        }

        void coordinate() {
            size = n;
            if (children[0] == nullptr) return;
            for (unsigned char i = 0; i <= n; i++) {
                size += children[i]->size;
                children[i]->parent = this;
                children[i]->position = i;
            }
        }
    };
    node *root;
    bool isForMulti;

   public:
    /** if argument is true, works as multiple set.default is set.*/
    twothreetree(bool forMultiSet = false) {
        root = new node();
        isForMulti = forMultiSet;
    }
    /** returns index.*/
    unsigned lower_bound(T key) {
        node *presentNode = root;
        unsigned res = 0;
        while (presentNode->children[0] != nullptr) {
            unsigned char i = presentNode->LB(key);
            for (unsigned char j = 0; j < i; j++)
                res += presentNode->children[j]->size;
            res += i;
            presentNode = presentNode->children[i];
        }
        return res + presentNode->LB(key);
    }
    /** returns index.*/
    unsigned upper_bound(T key) {
        node *presentNode = root;
        unsigned res = 0;
        while (presentNode->children[0] != nullptr) {
            unsigned char i = presentNode->UB(key);
            for (unsigned char j = 0; j < i; j++)
                res += presentNode->children[j]->size;
            res += i;
            presentNode = presentNode->children[i];
        }
        return res + presentNode->UB(key);
    }

    bool insert(T key) {
        node *presentNode = root;
        unsigned char i;
        while (presentNode->children[0] != nullptr) {
            i = presentNode->LB(key);
            if (!isForMulti && i < presentNode->n &&
                presentNode->keys[i] == key)
                return false;
            presentNode = presentNode->children[i];
        }
        i = presentNode->LB(key);
        if (!isForMulti && i < presentNode->n && presentNode->keys[i] == key)
            return false;
        presentNode->insert(i, key);
        if (root->parent != nullptr) {
            root = root->parent;
        }
        return true;
    }

    bool erase(T key) {
        node *presentNode = root;
        unsigned char i;
        while (presentNode->children[0] != nullptr) {
            i = presentNode->LB(key);
            if (i < presentNode->n && presentNode->keys[i] == key) break;
            presentNode = presentNode->children[i];
        }
        if (presentNode->children[0] == nullptr) {
            i = presentNode->LB(key);
            if (i < presentNode->n && presentNode->keys[i] == key) {
                presentNode->erase(i);
                if (root->n == 0 && root->children[0] != nullptr) {
                    root = root->children[0];
                    delete root->parent;
                    root->parent = nullptr;
                }
                return true;
            }
            return false;
        }
        node *eraseNode = presentNode->children[i + 1];
        while (eraseNode->children[0] != nullptr)
            eraseNode = eraseNode->children[0];
        presentNode->keys[i] = eraseNode->keys[0];
        eraseNode->erase(0);
        if (root->n == 0) {
            root = root->children[0];
            delete root->parent;
            root->parent = nullptr;
        }
        return true;
    }

    T get(unsigned i) {
        if (i < 0) i += root->size;
        node *presentNode = root;
        while (presentNode->children[0] != nullptr) {
            unsigned char j = 0;
            while (i >= presentNode->children[j]->size + 1) {
                i -= presentNode->children[j]->size + 1;
                j++;
            }
            if (i == presentNode->children[j]->size) {
                return presentNode->keys[j];
            }
            presentNode = presentNode->children[j];
        }
        return presentNode->keys[i];
    }

    unsigned size() {
        return root->size;
    }

    unsigned count(T key) {
        return upper_bound(key) - lower_bound(key);
    }

    T operator[](unsigned i) {
        return get(i);
    }
};