#pragma once

#include <algorithm>
#include <cassert>
#include <iostream>
#include <vector>

/**
 * @brief 座標圧縮 (Coordinate Compression) 構造体
 * @tparam T 扱う要素の型 (デフォルト: int)
 * @details
 * (T x): 値 x が何番目か (0-indexed) を取得
 * [int i]: i 番目に大きい元の値を取得
 */
template <typename T = int>
struct CC {
  bool initialized;
  std::vector<T> xs;

  CC() : initialized(false) {}

  // ベクターを受け取って即座に初期化するコンストラクタ
  explicit CC(const std::vector<T>& v) : initialized(false) {
    add(v);
    init();
  }

  // 要素を 1 つ追加
  void add(T x) {
    xs.push_back(x);
    initialized = false;
  }

  // ベクターの要素をまとめて追加
  void add(const std::vector<T>& v) {
    xs.insert(xs.end(), v.begin(), v.end());
    initialized = false;
  }

  // ソート & 重複排除を実行して圧縮テーブルを作成
  void init() {
    std::sort(xs.begin(), xs.end());
    xs.erase(std::unique(xs.begin(), xs.end()), xs.end());
    initialized = true;
  }

  // 値 x の圧縮後インデックスを取得 (0-indexed)
  int operator()(T x) const {
    assert(initialized && "CC::init() が呼び出されていません");
    auto it = std::lower_bound(xs.begin(), xs.end(), x);
    assert(it != xs.end() && *it == x && "指定された値が CC に存在しません");
    return std::distance(xs.begin(), it);
  }

  // i 番目に小さい元の値を取得
  T operator[](int i) const {
    assert(initialized && "CC::init() が呼び出されていません");
    assert(0 <= i && i < (int)xs.size() && "インデックスが範囲外です");
    return xs[i];
  }

  // 圧縮後の要素数 (ユニークな要素の数) を取得
  int size() const {
    assert(initialized && "CC::init() が呼び出されていません");
    return xs.size();
  }

  // デバッグ用出力
  friend std::ostream& operator<<(std::ostream& os, const CC& cc) {
    for (size_t i = 0; i < cc.xs.size(); i++) {
      os << cc.xs[i] << (i + 1 == cc.xs.size() ? "" : " ");
    }
    return os;
  }
};