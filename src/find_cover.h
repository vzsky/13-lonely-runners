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
  using CovArray = std::array<std::bitset<P / 2>, P / 2 + 1>;
  CovArray cov{};

  Context()
  {
    for (int i = 0; i <= P / 2; ++i)
      for (int t = 1; t <= P / 2; ++t)
      {
        int pos     = P / 2 - t;
        int rem     = (int)((1LL * t * i) % P);
        cov[i][pos] = (1LL * rem * (K + 1) < P) || (1LL * (P - rem) * (K + 1) < P);
      }
  }

  static const Context& instance()
  {
    static const Context ctx;
    return ctx;
  }
};

template <int K, int P> struct DfsSeed
{
  int depth;
  std::bitset<P / 2> covered;
  std::vector<char> eliminated;
  SpeedSet<K> elems;
  std::array<int, P / 2> remaining;
  int wasted_bits;
};

template <int K, int P> struct Dfs
{
  DfsSeed<K, P>& seed;
  SetOfSpeedSets<K> solutions{};
  const Context<K, P>::CovArray& cov = Context<K, P>::instance().cov;

  void run(int depth, std::bitset<P / 2> current_covered, int wasted_bits)
  {
    constexpr int bitlen       = P / 2;
    constexpr int bits_per_set = P / (K + 1);
    constexpr int max_waste    = K * bits_per_set - bitlen;

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

    if (wasted_bits > max_waste) return;
    if (early_return_bound(seed.remaining, current_covered, seed.eliminated, depth, nextToCover)) return;

    const std::array saved_remaining         = seed.remaining;
    const std::vector<char> saved_eliminated = seed.eliminated;
    for (int i = 1; i <= P / 2; ++i)
    {
      if (seed.eliminated[i]) continue;
      if (nextToCover == -1 || cov[i][nextToCover])
      {
        seed.elems.insert(i);
        int overlap              = (int)(current_covered & cov[i]).count();
        std::bitset next_covered = current_covered;
        next_covered |= cov[i];
        run(depth + 1, std::move(next_covered), wasted_bits + overlap);
        seed.elems.remove(i);
        seed.eliminated[i] = 1;
        for (int pos = 0; pos < bitlen; ++pos)
          if (cov[i][pos]) seed.remaining[pos]--;
      }
    }
    seed.remaining  = saved_remaining;
    seed.eliminated = saved_eliminated;
  }

private:
  bool early_return_bound(const std::array<int, P / 2>& remaining, const std::bitset<P / 2>& covered,
                          const std::vector<char>& eliminated, int used, int nextToCover)
  {
    if (nextToCover != -1 && remaining[nextToCover] == 0) [[unlikely]]
      return true;

    if (used < K - 5 || nextToCover == -1) return false;

    int slots = K - used - 1;

    std::bitset nextC  = ~covered;
    nextC[nextToCover] = 0;

    int totalToCover = P / 2 - (int)covered.count();

    std::vector<long long> contribs;
    contribs.reserve(P / 2);

    long long bestCovering_next = 0;
    for (int i = 1; i <= P / 2; ++i)
    {
      if (eliminated[i]) continue;
      long long c = (nextC & cov[i]).count();
      contribs.push_back(c);
      if (cov[i][nextToCover]) bestCovering_next = std::max(bestCovering_next, c + 1);
    }

    std::partial_sort(contribs.begin(), contribs.begin() + std::min((int)contribs.size(), slots),
                      contribs.end(), std::greater<>());

    long long topSum = 0;
    for (int i = 0; i < std::min((int)contribs.size(), slots); ++i) topSum += contribs[i];

    return totalToCover > topSum + bestCovering_next;
  }
};

template <int K, int P> static SetOfSpeedSets<K> run_dfs(DfsSeed<K, P>&& seed)
{
  auto d = Dfs<K, P>{seed};
  d.run(seed.depth, seed.covered, seed.wasted_bits);
  return d.solutions;
}

template <int K, int P> static SetOfSpeedSets<K> find_all_covers_parallel()
{
  constexpr int bitlen = P / 2;
  const auto& cov      = Context<K, P>::instance().cov;

  std::array<int, bitlen> remaining0{};
  for (int i = 1; i <= bitlen; ++i)
    for (int pos = 0; pos < bitlen; ++pos)
      if (cov[i][pos]) remaining0[pos]++;

  for (int pos = 0; pos < bitlen; ++pos)
    if (remaining0[pos] == 0) return {};

  std::vector<char> base_eliminated(bitlen + 1, 0);
  std::array<int, bitlen> base_remaining = remaining0;
  base_eliminated[1]                     = 1;
  for (int pos = 0; pos < bitlen; ++pos)
    if (cov[1][pos]) base_remaining[pos]--;

  std::bitset<bitlen> first_covered = cov[1];
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
  for (int i = 2; i <= bitlen; ++i)
    if (nextToCover1 == -1 || cov[i][nextToCover1]) top_candidates.push_back(i);

  // Precompute prefix elim/remaining so each task at index idx can start
  // with the correct state (all prior candidates already skipped past)
  const size_t ncands = top_candidates.size();
  std::vector<std::vector<char>> prefix_elim(ncands + 1);
  std::vector<std::array<int, bitlen>> prefix_rem(ncands + 1);
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

        int wasted_bits             = (int)(first_covered & cov[i]).count();
        std::bitset<bitlen> covered = first_covered | cov[i];
        SpeedSet<K> local_elems     = elems;
        local_elems.insert(i);

        thread_results[t].merge(run_dfs<K, P>(
            {2, std::move(covered), prefix_elim[idx], local_elems, prefix_rem[idx], wasted_bits}));
      }
    });
  }

  for (auto& th : threads) th.join();

  SetOfSpeedSets<K> base_solutions;
  for (size_t t = 0; t < nthreads; ++t) base_solutions.merge(thread_results[t]);

  return base_solutions;
}

} // namespace find_cover
