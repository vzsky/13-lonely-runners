#include <algorithm>
#include <cassert>
#include <format>
#include <iostream>
#include <tuple>
#include <utility>

#include "src/find_cover.h"
#include "src/lift.h"
#include "src/speedset.h"
#include "src/utils.h"

// ============================================================================
// State
// ============================================================================

template <int K> struct State
{
  int current_n;
  SetOfSpeedSets<K> S;
};

// ============================================================================
// Strategies
// ============================================================================

template <int Arg> struct Force
{
  template <int P, int K> State<K> operator()(State<K> st) const
  {
    auto C = lift::make_context(P, K, st.current_n * Arg, true);
    auto T = lift::find_lifted_covers_parallel(C, st.S, Arg);

    std::cout << std::format("  trying {}: T size = {}", Arg, T.size()) << std::endl;

    st.S = std::move(T);
    st.current_n *= Arg;
    return st;
  }
};

template <int Arg> struct Maybe
{
  template <int P, int K> State<K> operator()(State<K> st) const
  {
    auto C = lift::make_context(P, K, st.current_n * Arg, true);
    auto T = lift::find_lifted_covers_parallel(C, st.S, Arg);

    std::cout << std::format("  trying {}: T size = {}", Arg, T.size()) << std::endl;

    if (T.size() <= st.S.size())
    {
      st.S = std::move(T);
      st.current_n *= Arg;
    }
    return st;
  }
};

struct Project
{
  template <int P, int K> State<K> operator()(State<K> st) const
  {
    SetOfSpeedSets<K> T;
    for (auto s : st.S) T.insert(s.project(P).get_sorted_set());

    st.S         = std::move(T);
    st.current_n = 1;
    return st;
  }
};

template <int Limit = 0> struct Print
{
  template <int P, int K> State<K> operator()(State<K> st) const
  {
    if (Limit == 0 || st.S.size() <= Limit) std::cout << "seeds: " << st.S << std::endl;
    return st;
  }
};

struct LargeResolve
{
  template <int P, int K> State<K> operator()(State<K> st) const
  {
    if (P < K * (K - 1)) return st;
    if (st.S.size() != 1) return st;

    const SpeedSet<K>& s = *st.S.begin();
    for (int i = 0; i < K; i++)
      if (s.mSet[i] != i + 1) return st;

    st.S.clear();
    return st;
  }
};

template <int Arg> struct Squeeze
{
  template <int P, int K> State<K> operator()(State<K> st) const
  {
    SetOfSpeedSets<K> last = st.S;

    while (true)
    {
      timeit("\tsqueeze ", [&]
      {
        auto C = lift::make_context(P, K, st.current_n * Arg, true);
        st.S   = lift::find_lifted_covers_parallel(C, st.S, Arg);
        st.current_n *= Arg;
      });

      SetOfSpeedSets<K> U;
      for (auto s : st.S) U.insert(s.project(P).get_sorted_set());

      if (U.size() == last.size()) break;

      last = std::move(U);
      std::cout << std::format("  => squeezing (n={}): S size = {}", st.current_n, st.S.size()) << std::endl;
    }

    st.S         = std::move(last);
    st.current_n = 1;
    return st;
  }
};

// ============================================================================
// Tuple visitor
// ============================================================================

template <int P, int K, typename Tuple, std::size_t I = 0> State<K> apply_config(State<K> st, const Tuple& t)
{
  if constexpr (I < std::tuple_size_v<Tuple>)
  {
    if (!st.S.empty()) st = std::get<I>(t).template operator()<P, K>(st);
    return apply_config<P, K, Tuple, I + 1>(st, t);
  }
  else
    return st;
}

// ============================================================================
// Driver
// ============================================================================

template <int K, int P, typename Config> bool check_prime(const Config& config)
{
  std::cout << std::format("now={}\n", print_time());
  std::cout << std::format("Parameters: p = {}, k = {}", P, K) << std::endl;

  State<K> st{1, {}};

  timeit([&]
  {
    auto cov = find_cover::make_stationary_runner_coverage_mask<K, P>();
    st.S     = find_cover::find_all_covers_parallel<K, P>(cov);
    std::cout << std::format("Step 1 (n=1): S size = {}", st.S.size()) << std::endl;
  });

  timeit([&]
  {
    SetOfSpeedSets<K> T;
    for (auto s : st.S) T.insert(s.get_canonical_representation(P));
    st.S = std::move(T);
    std::cout << std::format("Step 1.5 (n=1): S size = {}", st.S.size()) << std::endl;
  });

  st = apply_config<P, K>(st, config);
  std::cout << std::endl;

  if (!st.S.empty())
  {
    std::cout << std::format("  Counter Example Mod {}\n", P) << std::endl;
    return 1;
  }

  return 0;
}

template <int K, std::array primes, typename Config, std::size_t... Is>
void roll_works(const Config& config, std::index_sequence<Is...>)
{
  timeit("All work done -- ", [&] { (check_prime<K, primes[Is]>(config), ...); });
}

// ============================================================================
// Main
// ============================================================================

int main()
{
  constexpr int K = 9;

  if constexpr (K == 8)
  {
    constexpr std::array primes = {47,  53,  59,  61,  67,  71,  73,  79,  83,  89,  97,  101, 103,
                                   107, 109, 113, 127, 131, 137, 139, 149, 151, 157, 163, 167, 173,
                                   179, 181, 191, 193, 197, 199, 211, 223, 227, 229, 233, 239, 241};

    constexpr auto config = std::make_tuple(Squeeze<2>{}, Force<3>{}, Force<3>{});

    roll_works<K, primes>(config, std::make_index_sequence<primes.size()>{});
  }

  if constexpr (K == 9)
  {
    constexpr std::array primes = {19,  53,  59,  67,  71,  73,  79,  83,  89,  97,  101, 103, 107,
                                   109, 113, 127, 131, 137, 139, 149, 151, 157, 163, 167, 173, 179,
                                   181, 191, 193, 197, 199, 211, 223, 227, 229, 233, 239, 241, 251,
                                   257, 263, 269, 271, 277, 281, 283, 293, 307, 311, 313, 317};

    constexpr auto config = std::make_tuple(Squeeze<2>{}, Squeeze<3>{}, Force<2>{}, Force<5>{});

    roll_works<K, primes>(config, std::make_index_sequence<primes.size()>{});
  }

  if constexpr (K == 10)
  {
    constexpr std::array primes = {103, 107, 109, 127, 131, 137, 139, 149, 151, 157, 163, 167, 173,
                                   179, 181, 191, 193, 197, 199, 211, 223, 227, 229, 233, 239, 241,
                                   251, 257, 263, 269, 271, 277, 281, 283, 293, 307, 311, 313, 317,
                                   331, 337, 347, 349, 353, 359, 367, 373, 379, 383, 389, 397, 401,
                                   409, 419, 421, 431, 433, 439, 443, 449, 457};

    constexpr auto config = std::make_tuple(Squeeze<2>{}, Squeeze<3>{}, LargeResolve{}, Print{});

    roll_works<K, primes>(config, std::make_index_sequence<primes.size()>{});
  }

  if constexpr (K == 11)
  {
    constexpr std::array primes = {
        23,  131, 137, 139, 149, 151, 157, 163, 167, 173, 179, 181, 191, 193, 197, 199, 211, 223, 227,
        229, 233, 239, 241, 251, 257, 263, 269, 271, 277, 281, 283, 293, 307, 311, 313, 317, 331, 337,
        347, 349, 353, 359, 367, 373, 379, 383, 389, 397, 401, 409, 419, 421, 431, 433, 439, 443, 449,
        457, 461, 463, 467, 479, 487, 491, 499, 503, 509, 521, 523, 541, 547, 557, 563, 569, 571, 577};

    constexpr auto config =
        std::make_tuple(Force<2>{}, Force<2>{}, Squeeze<2>{}, Force<2>{}, Force<2>{}, Force<3>{}, Maybe<3>{});

    roll_works<K, primes>(config, std::make_index_sequence<primes.size()>{});
  }

  if constexpr (K == 12)
  {
    constexpr std::array primes = {139, 149, 151, 167, 179, 181, 191, 193, 197, 199, 211, 223, 227, 229, 233,
                                   239, 251, 257, 263, 269, 271, 277, 281, 283, 307, 311, 313, 347, 379, 433,
                                   241, 293, 317, 331, 337, 349, 353, 359, 367, 373, 383, 389, 397, 401, 409,
                                   419, 421, 431, 433, 439, 443, 503, 509, 521, 523, 571, 577, 613, 617, 619,
                                   541, 547, 557, 599, 601, 607, 631, 641, 659, 661, 677, 683};

    constexpr auto config = std::make_tuple(Squeeze<2>{}, Squeeze<3>{}, LargeResolve{}, Print{});

    roll_works<K, primes>(config, std::make_index_sequence<primes.size()>{});
  }

  if constexpr (K == 13)
  {
    constexpr std::array primes = {199, 211, 223, 227, 229, 233, 239, 251, 257, 263, 269, 271, 277, 281, 283,
                                   307, 311, 313, 347, 379, 433, 439, 443, 449, 457, 461, 241, 293, 293, 317,
                                   331, 337, 349, 353, 359, 367, 373, 383, 389, 397, 401, 409, 419, 421, 431};

    constexpr auto config = std::make_tuple(Squeeze<2>{}, Squeeze<3>{}, Print{});

    roll_works<K, primes>(config, std::make_index_sequence<primes.size()>{});
  }

  if constexpr (K == 14)
  {
    constexpr std::array primes = {239, 251, 257, 263, 269, 271, 277, 281, 283, 307, 311, 313, 347,
                                   379, 433, 439, 443, 449, 457, 461, 241, 293, 293, 317, 331, 337,
                                   349, 353, 359, 367, 373, 383, 389, 397, 401, 409, 419, 421, 431};

    constexpr auto config = std::make_tuple(Squeeze<2>{}, Print<10>{}, Squeeze<3>{}, Print{});

    roll_works<K, primes>(config, std::make_index_sequence<primes.size()>{});
  }
}
