#include <algorithm>
#include <cassert>
#include <iostream>
#include <utility>

#include "src/find_cover.h"
#include "src/lift.h"
#include "src/speedset.h"
#include "src/utils.h"

struct Config
{
  enum class Type
  {
    Force,
    Maybe,
    Project,
    Squeeze,
    Print,
    LargeResolve
  } type;
  int argument;
};

std::ostream& operator<<(std::ostream& os, const Config& c)
{
  const char* type_str = "";

  switch (c.type)
  {
  case Config::Type::Force:
    type_str = "Force";
    break;
  case Config::Type::Maybe:
    type_str = "Maybe";
    break;
  case Config::Type::Project:
    type_str = "Project";
    break;
  case Config::Type::Squeeze:
    type_str = "Squeeze";
    break;
  case Config::Type::Print:
    type_str = "Print";
    break;
  case Config::Type::LargeResolve:
    type_str = "LargeResolve";
    break;
  }

  os << "Strategy [ type=" << type_str << ", argument=" << c.argument << "]";
  return os;
}

// Main driver: constructs and applies the lifting sieve over levels
template <int K, int P, std::array config> bool check_prime()
{
  std::cout << std::format("now={}\n", print_time());
  std::cout << std::format("Parameters: p = {}, k = {}", P, K) << std::endl;

  SetOfSpeedSets<K> S;

  // Step 1: Compute seed covers S at level n = 1 (half-range mod p)
  timeit([&]
  {
    find_cover::CoverageArray<P> cov = find_cover::make_stationary_runner_coverage_mask<K, P>();
    S                                = find_cover::find_all_covers_parallel<K, P>(cov);
    std::cout << std::format("Step 1 (n=1): S size = {}", S.size()) << std::endl;
  });

  timeit([&]
  {
    SetOfSpeedSets<K> T;
    for (auto seed : S) T.insert(seed.get_canonical_representation(P));
    S = std::move(T);
    std::cout << std::format("Step 1.5 (n=1): S size = {}", S.size()) << std::endl;
  });

  // Step 2 to N: Lift each seed from S using argiplier p, m =
  int current_n = 1;
  for (const auto& strat : config)
  {
    if (S.size() == 0) break;
    std::cout << "Doing " << strat << std::endl;
    timeit([&current_n, &S, &strat]
    {
      const auto [type, arg] = strat;
      if (type == Config::Type::Print)
      {
        if (S.size() > arg) return;
        std::cout << "seeds: " << S << std::endl;
        return;
      }
      if (type == Config::Type::LargeResolve)
      {
        if (P < K * (K - 1)) return;
        if (S.size() != 1) return;
        const SpeedSet<K> s = *S.begin();
        for (int i = 0; i < K; i++)
          if (s.mSet[i] != i + 1) return;
        S.clear();
        return;
      }
      if (type == Config::Type::Project)
      {
        SetOfSpeedSets<K> T;
        for (auto elem : S) T.insert(elem.project(P).get_sorted_set());
        current_n = 1;
        S         = std::move(T);
        return;
      }
      if (type == Config::Type::Squeeze)
      {
        SetOfSpeedSets<K> last = S;
        while (true)
        {
          timeit("\tsqueeze ", [&]
          {
            lift::Context C2 = lift::make_context(P, K, current_n * arg, true);
            S                = lift::find_lifted_covers_parallel(C2, S, arg);
            current_n *= arg;
          });
          SetOfSpeedSets<K> U;
          for (auto elem : S) U.insert(elem.project(P).get_sorted_set());
          if (U.size() == last.size()) break;
          last = U;
          std::cout << std::format("  => squeezing (n={}): S size = {}, U size = {}", current_n, S.size(),
                                   U.size())
                    << std::endl;
        }
        current_n = 1;
        S         = std::move(last);
        return;
      }

      if (type == Config::Type::Force)
      {
        lift::Context C2 = lift::make_context(P, K, current_n * arg, true);
        auto T           = lift::find_lifted_covers_parallel(C2, S, arg);
        std::cout << std::format("  trying {}: T size = {}", arg, T.size()) << std::endl;
        S = std::move(T);
        current_n *= arg;
        return;
      }

      if (type == Config::Type::Maybe)
      {
        lift::Context C2 = lift::make_context(P, K, current_n * arg, true);
        auto T           = lift::find_lifted_covers_parallel(C2, S, arg);
        std::cout << std::format("  trying {}: T size = {}", arg, T.size()) << std::endl;
        if (T.size() <= S.size())
        {
          S = std::move(T);
          current_n *= arg;
        }
        return;
      }
    });
    std::cout << std::format("  => (n={}): S size = {}", current_n, S.size()) << std::endl;
  }

  // Final result: if S is empty then LRC verified for this p
  if (!S.empty())
  {
    std::cout << std::format("  Counter Example Mod {}\n", P) << std::endl;
    return 1;
  }
  std::cout << std::endl;
  return 0;
}

template <int K, std::array primes, std::array configs, std::size_t... Is>
void roll_works(std::index_sequence<Is...>)
{
  timeit("All work done -- ", [] { (check_prime<K, primes[Is], configs>(), ...); });
}

int main()
{
  constexpr auto Force        = [](int p) { return Config{Config::Type::Force, p}; };
  constexpr auto Maybe        = [](int p) { return Config{Config::Type::Maybe, p}; };
  constexpr auto Squeeze      = [](int p) { return Config{Config::Type::Squeeze, p}; };
  constexpr auto Project      = [] { return Config{Config::Type::Project, 0}; };
  constexpr auto Print        = [](int limit = 0) { return Config{Config::Type::Print, limit}; };
  constexpr auto LargeResolve = [] { return Config{Config::Type::LargeResolve, 0}; };

  constexpr int K = 13;

  if constexpr (K == 8)
  {
    constexpr std::array primes = {47,  53,  59,  61,  67,  71,  73,  79,  83,  89,  97,  101, 103,
                                   107, 109, 113, 127, 131, 137, 139, 149, 151, 157, 163, 167, 173,
                                   179, 181, 191, 193, 197, 199, 211, 223, 227, 229, 233, 239, 241};
    constexpr std::array config = {Squeeze(2), Force(3), Force(3)};
    roll_works<K, primes, config>(std::make_index_sequence<primes.size()>{});
  }

  if constexpr (K == 9)
  {
    constexpr std::array primes = {19,  53,  59,  67,  71,  73,  79,  83,  89,  97,  101, 103, 107,
                                   109, 113, 127, 131, 137, 139, 149, 151, 157, 163, 167, 173, 179,
                                   181, 191, 193, 197, 199, 211, 223, 227, 229, 233, 239, 241, 251,
                                   257, 263, 269, 271, 277, 281, 283, 293, 307, 311, 313, 317};
    constexpr std::array config = {Squeeze(2), Squeeze(3), Force(2), Force(5)};
    roll_works<K, primes, config>(std::make_index_sequence<primes.size()>{});
  }

  if constexpr (K == 10)
  {
    constexpr std::array primes = {103, 107, 109, 127, 131, 137, 139, 149, 151, 157, 163, 167, 173,
                                   179, 181, 191, 193, 197, 199, 211, 223, 227, 229, 233, 239, 241,
                                   251, 257, 263, 269, 271, 277, 281, 283, 293, 307, 311, 313, 317,
                                   331, 337, 347, 349, 353, 359, 367, 373, 379, 383, 389, 397, 401,
                                   409, 419, 421, 431, 433, 439, 443, 449, 457};
    constexpr std::array config = {Squeeze(2), Squeeze(3), LargeResolve(), Print()};
    roll_works<K, primes, config>(std::make_index_sequence<primes.size()>{});
  }

  if constexpr (K == 11)
  {
    constexpr std::array primes = {
        23,  131, 137, 139, 149, 151, 157, 163, 167, 173, 179, 181, 191, 193, 197, 199, 211, 223, 227,
        229, 233, 239, 241, 251, 257, 263, 269, 271, 277, 281, 283, 293, 307, 311, 313, 317, 331, 337,
        347, 349, 353, 359, 367, 373, 379, 383, 389, 397, 401, 409, 419, 421, 431, 433, 439, 443, 449,
        457, 461, 463, 467, 479, 487, 491, 499, 503, 509, 521, 523, 541, 547, 557, 563, 569, 571, 577};
    constexpr std::array config = {Force(2), Force(2), Squeeze(2), Force(2), Force(2), Force(3), Maybe(3)};
    roll_works<K, primes, config>(std::make_index_sequence<primes.size()>{});
  }

  if constexpr (K == 12)
  {
    constexpr std::array primes = {139, 149, 151, 167, 179, 181, 191, 193, 197, 199, 211, 223, 227, 229, 233,
                                   239, 251, 257, 263, 269, 271, 277, 281, 283, 307, 311, 313, 347, 379, 433,
                                   241, 293, 317, 331, 337, 349, 353, 359, 367, 373, 383, 389, 397, 401, 409,
                                   419, 421, 431, 433, 439, 443, 503, 509, 521, 523, 571, 577, 613, 617, 619,
                                   541, 547, 557, 599, 601, 607, 631, 641, 659, 661, 677, 683};
    constexpr std::array config = {Squeeze(2), Squeeze(3), LargeResolve(), Print()};
    roll_works<K, primes, config>(std::make_index_sequence<primes.size()>{});
  }

  if constexpr (K == 13)
  { // wip
    constexpr std::array primes = {199, 211, 223, 227, 229, 233, 239, 251, 257, 263, 269, 271, 277, 281, 283,
                                   307, 311, 313, 347, 379, 433, 439, 443, 449, 457, 461, 241, 293, 293, 317,
                                   331, 337, 349, 353, 359, 367, 373, 383, 389, 397, 401, 409, 419, 421, 431};
    constexpr std::array config = {Squeeze(2), Squeeze(3), Print()};
    roll_works<K, primes, config>(std::make_index_sequence<primes.size()>{});
  }

  if constexpr (K == 14)
  { // wip
    constexpr std::array primes = {239, 251, 257, 263, 269, 271, 277, 281, 283, 307, 311, 313, 347,
                                   379, 433, 439, 443, 449, 457, 461, 241, 293, 293, 317, 331, 337,
                                   349, 353, 359, 367, 373, 383, 389, 397, 401, 409, 419, 421, 431};
    constexpr std::array config = {Squeeze(2), Print(10), Squeeze(3), Print()};
    roll_works<K, primes, config>(std::make_index_sequence<primes.size()>{});
  }
}
