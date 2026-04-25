#include <cassert>
#include <format>
#include <tuple>
#include <utility>

#include "../../src/find_cover.h"
#include "../../src/utils.h"
#include "../../src/lift_strategy.h"

// ============================================================================
// Driver
// ============================================================================

template <int K, int P, typename Config> void check_prime(const Config& config)
{
  Log(std::format("now={}", print_time()));
  Log(std::format("Parameters: p = {}, k = {}", P, K));

  State<K, 1> st{};

  timeit([&]
  {
    PushLogScope("FindCover");
    st.S = find_cover::find_all_covers_parallel<K, P>();
    Log(std::format("Step 1 (l=1): S size = {}", st.S.size()));
  });

  // TODO add log scope
  const auto final_st = apply_config<P, K>(st, config);
  if (!final_st.S.empty())
    Log(std::format("Counter Example Mod {}", P));
  else 
    Log(std::format("Subproof Mod {} Done", P));
  Log();
}

template <int K, std::array primes, typename Config, std::size_t... Is>
void roll_works(const Config& config, std::index_sequence<Is...>)
{
  timeit("All work done", [&] { (check_prime<K, primes[Is]>(config), ...); });
}

// ============================================================================
// Main
// ============================================================================

int main()
{
    constexpr std::array primes = {47,  53,  59,  61,  67,  71,  73,  79,  83,  89,  97,  101, 103,
                                   107, 109, 113, 127, 131, 137, 139, 149, 151, 157, 163, 167, 173,
                                   179, 181, 191, 193, 197, 199, 211, 223, 227, 229, 233, 239, 241};

    constexpr auto config = std::make_tuple(Force<3>{}, Force<3>{});
    roll_works<8, primes>(config, std::make_index_sequence<primes.size()>{});
}

