#include <algorithm>
#include <atomic>
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

namespace find_cover
{

template <int K, int P> struct Context
{
  using CoveredBitset = std::bitset<P / 2>;
  using CovArray      = std::array<CoveredBitset, P / 2>;
  CovArray cov{};

  Context()
  {
    for (int i = 0; i < P / 2; ++i)
      for (int t = 1; t <= P / 2; ++t)
      {
        int pos     = P / 2 - t;
        int rem     = int((1LL * t * (i + 1)) % P);
        cov[i][pos] = (1LL * rem * (K + 1) < P) || (1LL * (P - rem) * (K + 1) < P);
      }
  }
};

template <int K, int P> inline const Context<K, P> context{};

template <int K, int P> struct Dfs
{
  using CoveredBitset = typename Context<K, P>::CoveredBitset;
  using ElimArray     = std::array<char, P / 2>;
  using RemainArray   = std::array<char, P / 2>;

  struct Seed
  {
    int depth;
    CoveredBitset covered;
    ElimArray eliminated{};
    SpeedSet<K> elems;
    RemainArray remaining{};
  } seed;

  SetOfSpeedSets<K> solutions{};
  const typename Context<K, P>::CovArray& cov = context<K, P>.cov;

  static constexpr int bitlen = P / 2;

  void run() { run(seed.depth, seed.covered); }

  void run(int depth, CoveredBitset current_covered)
  {
    if (depth == K)
    {
      if (current_covered.count() != bitlen) return;
      solutions.insert(seed.elems);
      return;
    }

    int nextToCover = -1, best = INT_MAX;
    for (int pos = 0; pos < bitlen; ++pos)
      if (!current_covered[pos] && seed.remaining[pos] < best)
      {
        best        = seed.remaining[pos];
        nextToCover = pos;
      }

    if (early_return_bound(current_covered, depth, nextToCover)) return;

    const RemainArray saved_remaining = seed.remaining;
    const ElimArray saved_eliminated  = seed.eliminated;

    for (int i = 0; i < bitlen; ++i)
    {
      if (seed.eliminated[i]) continue;
      if (nextToCover == -1 || cov[i][nextToCover])
      {
        seed.elems.insert(i + 1);
        CoveredBitset next_covered = current_covered;
        next_covered |= cov[i];

        run(depth + 1, std::move(next_covered));

        seed.elems.remove(i + 1);
        seed.eliminated[i] = 1;
        for (int pos = 0; pos < bitlen; ++pos)
          if (cov[i][pos]) seed.remaining[pos]--;
      }
    }

    seed.remaining  = saved_remaining;
    seed.eliminated = saved_eliminated;
  }

private:
  bool early_return_bound(const CoveredBitset& covered, int used, int nextToCover)
  {
    if (nextToCover != -1 && seed.remaining[nextToCover] == 0) return true;
    if (used < K - 4 || nextToCover == -1) return false;

    int slots = K - used - 1;

    CoveredBitset nextC = ~covered;
    nextC[nextToCover]  = 0;

    int totalToCover = bitlen - covered.count();

    int bestCovering_next = 0;
    int bestCovering      = 0;
    for (int i = 0; i < bitlen; ++i)
    {
      if (seed.eliminated[i]) continue;
      int c        = (nextC & cov[i]).count();
      bestCovering = std::max(bestCovering, c);
      if (cov[i][nextToCover]) bestCovering_next = std::max(bestCovering_next, c + 1);
    }

    return totalToCover > bestCovering_next + bestCovering * slots;
  }
};

template <int K, int P> static SetOfSpeedSets<K> find_all_covers_parallel()
{
  using CoveredBitset = typename Dfs<K, P>::CoveredBitset;
  using ElimArray     = typename Dfs<K, P>::ElimArray;
  using RemainArray   = typename Dfs<K, P>::RemainArray;

  constexpr int bitlen = P / 2;
  const auto& cov      = context<K, P>.cov;

  RemainArray remaining0{};
  for (int i = 0; i < bitlen; ++i)
    for (int pos = 0; pos < bitlen; ++pos)
      if (cov[i][pos]) remaining0[pos]++;

  for (int pos = 0; pos < bitlen; ++pos)
    if (remaining0[pos] == 0) return {};

  ElimArray base_eliminated{};
  RemainArray base_remaining = remaining0;

  base_eliminated[0] = 1;
  for (int pos = 0; pos < bitlen; ++pos)
    if (cov[0][pos]) base_remaining[pos]--;

  CoveredBitset first_covered = cov[0];
  SpeedSet<K> elems{};
  elems.insert(1);

  int nextToCover1 = -1, best1 = INT_MAX;
  for (int pos = 0; pos < bitlen; ++pos)
    if (!first_covered[pos] && base_remaining[pos] < best1)
    {
      best1        = base_remaining[pos];
      nextToCover1 = pos;
    }

  std::vector<int> top_candidates;
  for (int i = 1; i < bitlen; ++i)
    if (nextToCover1 == -1 || cov[i][nextToCover1]) top_candidates.push_back(i);

  const size_t ncands = top_candidates.size();
  std::vector<ElimArray> prefix_elim(ncands + 1);
  std::vector<RemainArray> prefix_rem(ncands + 1);

  prefix_elim[0] = base_eliminated;
  prefix_rem[0]  = base_remaining;

  for (size_t idx = 0; idx < ncands; ++idx)
  {
    prefix_elim[idx + 1]    = prefix_elim[idx];
    prefix_rem[idx + 1]     = prefix_rem[idx];
    int j                   = top_candidates[idx];
    prefix_elim[idx + 1][j] = 1;
    for (int pos = 0; pos < bitlen; ++pos)
      if (cov[j][pos]) prefix_rem[idx + 1][pos]--;
  }

  size_t nthreads = parallelize_core();
  if (nthreads > ncands) nthreads = ncands;

  std::atomic<size_t> next_idx{0};
  std::vector<SetOfSpeedSets<K>> thread_results(nthreads);
  std::vector<std::thread> threads;

  for (size_t t = 0; t < nthreads; ++t)
  {
    threads.emplace_back([&, t]
    {
      while (true)
      {
        size_t idx = next_idx.fetch_add(1, std::memory_order_relaxed);
        if (idx >= ncands) break;

        int i = top_candidates[idx];

        SpeedSet<K> local_elems = elems;
        local_elems.insert(i + 1);

        Dfs<K, P> d(typename Dfs<K, P>::Seed{
            2,
            first_covered | cov[i],
            prefix_elim[idx],
            local_elems,
            prefix_rem[idx],
        });
        d.run();
        thread_results[t].merge(d.solutions);
      }
    });
  }

  for (auto& th : threads) th.join();

  SetOfSpeedSets<K> base_solutions;
  for (size_t t = 0; t < nthreads; ++t) base_solutions.merge(thread_results[t]);

  return base_solutions;
}

} // namespace find_cover
