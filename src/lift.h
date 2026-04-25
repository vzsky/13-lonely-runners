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
  using CoverBitset      = std::bitset<Q>;

  const CoverBitset& cover(int i)
  {
    if (mCover[i].count() != 0) [[likely]]
      return mCover[i];

    for (int t = 1; t <= Q; ++t)
    {
      int pos   = Q - t;
      int rem   = (t * i) % Q;
      bool cond = (rem * (K + 1) < Q) || ((Q - rem) * (K + 1) < Q);
      if (cond) mCover[i].set(pos);
    }
    return mCover[i];
  }

private:
  // NB. ideally this is const after initialization but compile time heavy enough
  // TODO: see how bad/good is it to move to runtime context
  std::array<CoverBitset, Q> mCover;
};

template <int P, int K, int N> static Context<P, K, N> context{};

template <int P, int K, int N> struct Dfs
{
  SpeedSet<K> elem;
  Context<P, K, N>::CoverBitset unionCover;
  std::array<std::vector<int>, K> candidates;
  SetOfSpeedSets<K> result;
  Dfs(std::array<std::vector<int>, K>&& cnd) : candidates{std::move(cnd)} {}

  void run()
  {
    if (elem.size() == K)
    {
      if (unionCover.count() != context<P, K, N>.Q) return;
      if (elem.subset_gcd_implies_proper(N)) return;
      result.insert(elem.get_sorted_set());
      return;
    }

    for (int candidate : candidates[elem.size()])
    {
      auto savedCover = unionCover;
      elem.insert(candidate);
      unionCover |= context<P, K, N>.cover(candidate);
      run();
      elem.remove(candidate);
      unionCover = savedCover;
    }
  }
};

template <int P, int K, int L, int C> SetOfSpeedSets<K> lift(const SpeedSet<K>& seed)
{
  static constexpr auto Q  = context<P, K, L * C>.Q;
  const auto makeCandidate = [&]
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

    std::sort(cand.begin(), cand.end(), [&](const auto& A, const auto& B) { return A.size() < B.size(); });
    return cand;
  };

  Dfs<P, K, L * C> runner{makeCandidate()};
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
