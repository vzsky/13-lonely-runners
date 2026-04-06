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
  std::array<std::bitset<Q>, Q> vec;

  Context()
  {
    for (int i = 0; i < Q; ++i)
    {
      auto& B = vec[i];
      for (int t = 1; t <= Q; ++t)
      {
        int pos   = Q - t;
        int rem   = (int)((1LL * t * i) % Q);
        bool cond = (1LL * rem * (K + 1) < Q) || (1LL * (Q - rem) * (K + 1) < Q);
        if (cond) B.set(pos);
      }
    }
  }
};

template <int P, int K, int N> inline const Context<P, K, N> context{};

template <int P, int K, int N> struct Dfs
{
  SpeedSet<K> elem;
  std::array<int, K> order;
  std::array<std::vector<int>, K> cand;
  SetOfSpeedSets<K> result;
  Dfs(const std::array<int, K>& ord, const std::array<std::vector<int>, K>& cnd) : order{ord}, cand{cnd} {}
  void run(int depth)
  {
    if (depth == K)
    {
      std::bitset<context<P, K, N>.Q> acc;
      for (auto v : elem) acc |= context<P, K, N>.vec[v];
      if ((int)acc.count() != context<P, K, N>.Q) return;
      if (elem.subset_gcd_implies_proper(N)) return;
      result.insert(elem.get_sorted_set());
      return;
    }
    int pos = order[depth];
    for (int candidate : cand[pos])
    {
      elem.insert(candidate);
      run(depth + 1);
      elem.remove(candidate);
    }
  }
};

template <int P, int K, int L, int C> SetOfSpeedSets<K> lift(const SpeedSet<K>& seed)
{
  const auto& Ctx = context<P, K, L * C>;
  auto cand       = [&]
  {
    std::array<std::vector<int>, K> cand{};
    int j = 0;
    for (const auto& s : seed)
    {
      for (int a = 0; a < C; a++)
      {
        long long val = (long long)s + (long long)a * (Ctx.Q / C);
        if (val < Ctx.Q)
          cand[j].push_back((int)val);
        else
          break;
      }
      ++j;
    }
    return cand;
  }();

  std::array<int, K> order;
  std::iota(order.begin(), order.end(), 0);
  std::sort(order.begin(), order.end(), [&](int A, int B) { return cand[A].size() < cand[B].size(); });

  Dfs<P, K, L * C> runner{order, cand};
  runner.run(0);

  return runner.result;
}

template <int P, int K, int L, int C>
SetOfSpeedSets<K> find_lifted_covers_parallel(const SetOfSpeedSets<K>& seeds)
{
  size_t N_seeds = seeds.size();
  if (N_seeds == 0) return {};

  unsigned int nthreads = parallelize_core();
  if (nthreads > N_seeds) nthreads = (unsigned int)N_seeds;

  std::vector<SetOfSpeedSets<K>> thread_results(nthreads);
  std::vector<std::thread> threads;

  auto worker = [&](auto begin, auto end, unsigned tid)
  {
    auto& local_results = thread_results[tid];
    for (auto it = begin; it != end; ++it) local_results.merge(lift<P, K, L, C>(*it));
  };

  // TODO: make this a co_await instead?
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
