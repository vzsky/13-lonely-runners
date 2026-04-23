#pragma once

#include <algorithm>
#include <bitset>
#include <cassert>
#include <climits>
#include <cstdint>
#include <functional>
#include <iostream>
#include <numeric>
#include <thread>
#include <utility>
#include <vector>

#include "speedset.h"
#include "utils.h"

namespace lift
{

template <int P, int K, int N> struct Context
{
  static constexpr int Q = N * P;

  const std::bitset<Q>& cover(int i) const { return mCover[i]; }

  Context()
  {
    for (int i = 0; i < Q; ++i)
    {
      auto& B = mCover[i];
      for (int t = 1; t <= Q; ++t)
      {
        int pos   = Q - t;
        int rem   = (t * i) % Q;
        bool cond = (rem * (K + 1) < Q) || ((Q - rem) * (K + 1) < Q);
        if (cond) B.set(pos);
      }
    }
  }

private:
  // TODO: make const
  std::array<std::bitset<Q>, Q> mCover;
};

template <int P, int K, int N> static const Context<P, K, N> context{};

template <int P, int K, int N> struct Dfs
{
  SpeedSet<K> elem;
  std::array<int, K> order;
  std::array<std::vector<int>, K> cand;
  SetOfSpeedSets<K> result;
  Dfs(const std::array<int, K>& ord, const std::array<std::vector<int>, K>& cnd) : order{ord}, cand{cnd} {}

  void run()
  {
    if (elem.size() == K)
    {
      std::bitset<context<P, K, N>.Q> acc;
      for (auto v : elem) acc |= context<P, K, N>.cover(v);
      if (acc.count() != context<P, K, N>.Q) return;
      if (elem.subset_gcd_implies_proper(N)) return;
      result.insert(elem.get_sorted_set());
      return;
    }
    const int pos = order[elem.size()];
    for (int candidate : cand[pos])
    {
      elem.insert(candidate);
      run();
      elem.remove(candidate);
    }
  }
};

template <int P, int K, int L, int C> SetOfSpeedSets<K> lift(const SpeedSet<K>& seed)
{
  static constexpr auto Q = context<P, K, L * C>.Q;
  auto cand               = [&]
  {
    std::array<std::vector<int>, K> cand{};
    int j = 0;
    for (const auto& s : seed)
    {
      for (int a = 0; a < C; a++)
      {
        long long val = (long long)s + (long long)a * (Q / C);
        if (val >= Q) break;
        cand[j].push_back((int)val);
      }
      ++j;
    }
    return cand;
  }();

  std::array<int, K> order;
  std::iota(order.begin(), order.end(), 0);
  std::sort(order.begin(), order.end(), [&](int A, int B) { return cand[A].size() < cand[B].size(); });

  // TODO: get rid of order
  Dfs<P, K, L * C> runner{std::move(order), std::move(cand)};

  runner.run();
  return runner.result;
}

template <int P, int K, int L, int C>
SetOfSpeedSets<K> find_lifted_covers_parallel(const SetOfSpeedSets<K>& seeds)
{
  const size_t N_seeds = seeds.size();
  if (N_seeds == 0) return {};

  const unsigned int nthreads = std::min(parallelize_core(), N_seeds);

  std::vector<SetOfSpeedSets<K>> thread_results(nthreads);
  std::vector<std::thread> threads;

  const auto worker = [&](auto begin, auto end, unsigned tid)
  {
    auto& local_results = thread_results[tid];
    for (auto it = begin; it != end; ++it) local_results.merge(lift<P, K, L, C>(*it));
  };

  size_t chunk = (N_seeds + nthreads - 1) / nthreads;
  auto it      = seeds.begin();

  for (unsigned int t = 0; t < nthreads && it != seeds.end(); ++t)
  {
    auto start  = it;
    size_t step = std::min(chunk, (size_t)std::distance(it, seeds.end()));
    std::advance(it, step);
    threads.emplace_back(worker, start, it, t);
  }

  for (auto& th : threads) th.join();

  SetOfSpeedSets<K> results;
  for (unsigned int t = 0; t < nthreads; ++t) results.merge(thread_results[t]);
  return results;
}

} // namespace lift
