#include "resolvable.h"
#include <atomic>
#include <iostream>
#include <thread>
#include <vector>

// ═════════════════════════════════════════════════════════════════════════════
//  Threaded resolvability check
// ═════════════════════════════════════════════════════════════════════════════

template <int K, long long P> void run_threads()
{
  constexpr int mod         = K + 1;
  constexpr long long total = []
  {
    long long t = 1;
    for (int i = 0; i < K; ++i) t *= (K + 1);
    return t;
  }();
  constexpr int NUM_THREADS = 10;

  // ── 1. Build all_r once, shared across threads ────────────────────────────
  std::cout << "Building all_r (p=" << P << ")... " << std::flush;
  auto all_r = resolvable::all_r_components<K>(P);
  std::cout << "|all_r| = " << all_r.size() << "\n";

  // ── 2. Shared state ───────────────────────────────────────────────────────
  std::atomic<bool> found{false}; // early-exit flag
  std::vector<std::thread> threads;
  threads.reserve(NUM_THREADS);

  // First non-resolvable vector found (written once, read after join)
  typename resolvable::SpeedSet<K>::value_t found_vec{};

  // ── 3. Partition [0, total) across threads ────────────────────────────────
  long long chunk = (total + NUM_THREADS - 1) / NUM_THREADS;

  for (int t = 0; t < NUM_THREADS; ++t)
  {
    long long lo = t * chunk;
    long long hi = std::min(lo + chunk, total);
    if (lo >= hi) break;

    threads.emplace_back([&, lo, hi]()
    {
      typename resolvable::SpeedSet<K>::value_t components{};

      for (long long idx = lo; idx < hi; ++idx)
      {
        // ── early exit check ─────────────────────────────────────────
        if (found.load(std::memory_order_relaxed)) return;

        // ── decode idx → components (little-endian base-mod) ─────────
        long long tmp = idx;
        for (int i = 0; i < K; ++i)
        {
          components[i] = tmp % mod;
          tmp /= mod;
        }

        resolvable::SpeedSet<K> ss(components);

        if (!ss.is_resolvable(all_r))
        {
          // ── signal all other threads to stop ─────────────────────
          if (!found.exchange(true, std::memory_order_relaxed))
            found_vec = components; // record only the first finder
          return;
        }
      }
    });
  }

  // ── 4. Join ───────────────────────────────────────────────────────────────
  for (auto& th : threads) th.join();

  // ── 5. Report ─────────────────────────────────────────────────────────────
  if (found.load())
  {
    resolvable::SpeedSet<K> bad(found_vec);
    std::cout << "✗  Non-resolvable vector found: " << bad << '\n';
  }
  else
  {
    std::cout << "✓  All " << total << " vectors are resolvable.\n";
  }
}

// ═════════════════════════════════════════════════════════════════════════════
//  Main
// ═════════════════════════════════════════════════════════════════════════════

int main()
{
  constexpr int K           = 10;
  constexpr long long P     = 101;
  constexpr int mod         = K + 1;
  constexpr long long total = []
  {
    long long t = 1;
    for (int i = 0; i < K; ++i) t *= (K + 1);
    return t;
  }();

  std::cout << "=== k = " << K << "  (Z_" << mod << ")^" << K << " ===\n";
  std::cout << "Total vectors  : " << total << '\n';

  run_threads<K, P>();
}
