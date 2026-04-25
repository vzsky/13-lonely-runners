#include <cassert>
#include <format>
#include <tuple>
#include <utility>

#include "src/find_cover.h"
#include "src/utils.h"
#include "src/lift_strategy.h"

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
  constexpr int K = 9;

  if constexpr (K == 6)
  {
    constexpr std::array primes = {47,  53,  59,  61,  67,  71,  73,  79,  83,  89,  97,  101, 103,
                                   107, 109, 113, 127, 131, 137, 139, 149, 151, 157, 163, 167, 173,
                                   179, 181, 191, 193, 197, 199, 211, 223, 227, 229, 233, 239, 241};

    constexpr auto config = std::make_tuple(TightLargePrime{}, Force<7>{});
    roll_works<K, primes>(config, std::make_index_sequence<primes.size()>{});
  }

  if constexpr (K == 7)
  {
    constexpr std::array primes = {47,  53,  59,  61,  67,  71,  73,  79,  83,  89,  97,  101, 103,
                                   107, 109, 113, 127, 131, 137, 139, 149, 151, 157, 163, 167, 173,
                                   179, 181, 191, 193, 197, 199, 211, 223, 227, 229, 233, 239, 241};

    constexpr auto config = std::make_tuple(Force<2>{}, Force<2>{}, Force<2>{});
    roll_works<K, primes>(config, std::make_index_sequence<primes.size()>{});
  }

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
    constexpr std::array primes = {127, 131, 137, 139, 149, 151, 157, 163, 167, 173, 179, 181, 191,
                                   193, 197, 199, 211, 223, 227, 229, 233, 239, 241, 251, 257, 263,
                                   269, 271, 277, 281, 283, 293, 307, 311, 313, 317, 331, 337, 347,
                                   349, 353, 359, 367, 373, 379, 383, 389, 397, 401, 409, 419, 421,
                                   431, 433, 439, 443, 449, 457, 461, 463, 467};

    constexpr auto config = std::make_tuple(TightLargePrime{}, Squeeze<2>{});
    roll_works<K, primes>(config, std::make_index_sequence<primes.size()>{});
  }

  if constexpr (K == 11)
  {
    constexpr std::array primes = {
        23,  131, 137, 139, 149, 151, 157, 163, 167, 173, 179, 181, 191, 193, 197, 199, 211, 223, 227,
        229, 233, 239, 241, 251, 257, 263, 269, 271, 277, 281, 283, 293, 307, 311, 313, 317, 331, 337,
        347, 349, 353, 359, 367, 373, 379, 383, 389, 397, 401, 409, 419, 421, 431, 433, 439, 443, 449,
        457, 461, 463, 467, 479, 487, 491, 499, 503, 509, 521, 523, 541, 547, 557, 563, 569, 571, 577};

    constexpr auto config = std::make_tuple( //
        Force<2>{}, Force<2>{}, Squeeze<2>{}, Force<2>{}, Force<2>{}, Force<3>{}, Squeeze<3>{});
    roll_works<K, primes>(config, std::make_index_sequence<primes.size()>{});
  }

  if constexpr (K == 12)
  {
    constexpr std::array primes = {139, 149, 151, 167, 179, 181, 191, 193, 197, 199, 211, 223, 227, 229, 233,
                                   239, 251, 257, 263, 269, 271, 277, 281, 283, 307, 311, 313, 347, 379, 433,
                                   241, 293, 317, 331, 337, 349, 353, 359, 367, 373, 383, 389, 397, 401, 409,
                                   419, 421, 431, 433, 439, 443, 503, 509, 521, 523, 571, 577, 613, 617, 619,
                                   541, 547, 557, 599, 601, 607, 631, 641, 659, 661, 677, 683};

    constexpr auto config = std::make_tuple(TightLargePrime{}, Squeeze<2>{}, Squeeze<3>{}, Print{});
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

    constexpr auto config = std::make_tuple(Squeeze<2>{}, Print<10>{}, Squeeze<3>{}, Print<10>{});
    roll_works<K, primes>(config, std::make_index_sequence<primes.size()>{});
  }
}
