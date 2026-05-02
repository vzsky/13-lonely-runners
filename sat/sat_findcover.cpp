#include <cadical.hpp>

#include <array>
#include <bitset>
#include <cassert>
#include <iostream>
#include <set>
#include <vector>

static constexpr int P = 197;
static constexpr int K = 9;

static constexpr int SATISFIABLE_RETURN_CODE = 10;

struct Context
{
  using CoveredBitset = std::bitset<P / 2>;
  std::array<CoveredBitset, P / 2> cover{};

  Context()
  {
    for (int i = 0; i < P / 2; ++i)
      for (int t = 1; t <= P / 2; ++t)
      {
        int pos       = P / 2 - t;
        int rem       = (1LL * t * (i + 1)) % P;
        cover[i][pos] = (rem * (K + 1) < P) || ((P - rem) * (K + 1) < P);
      }
  }
};

static const Context context{};

class CNF
{
public:
  using Clause = std::vector<int>;

  void add_clause(const Clause& c) { clauses.push_back(c); }

  void fix_true(int lit) { add_clause({lit}); }
  void fix_false(int lit) { add_clause({-lit}); }

  void add_or(const std::vector<int>& lits)
  {
    assert(!lits.empty());
    add_clause(lits);
  }

  void add_exactly_k(const std::vector<int>& x, int K)
  {
    add_at_least_k(x, K);
    add_at_most_k(x, K);
  }

  // at least K in x are true
  void add_at_least_k(const std::vector<int>& x, int K)
  {
    const int n = x.size();
    if (K <= 0) return;
    if (K > n) return add_contradiction();

    std::vector<int> neg;
    for (int v : x) neg.push_back(-v);
    add_at_most_k(neg, n - K);
  }

  // at most K in x are true
  void add_at_most_k(const std::vector<int>& x, int K)
  {
    const int n = x.size();
    if (K >= n) return;
    if (K < 0) return add_contradiction();
    if (K == 0)
    {
      for (int v : x) fix_false(v);
      return;
    }

    std::vector<std::vector<int>> s(n, std::vector<int>(K, 0));

    for (int i = 0; i < n; ++i)
      for (int j = 0; j < K; ++j) s[i][j] = new_var();

    // i = 0
    add_clause({-x[0], s[0][0]});
    for (int j = 1; j < K; ++j) fix_false(s[0][j]);

    // i > 0
    for (int i = 1; i < n; ++i)
    {
      add_clause({-x[i], s[i][0]});
      add_clause({-s[i - 1][0], s[i][0]});

      for (int j = 1; j < K; ++j)
      {
        add_clause({-x[i], -s[i - 1][j - 1], s[i][j]});
        add_clause({-s[i - 1][j], s[i][j]});
      }

      add_clause({-x[i], -s[i - 1][K - 1]});
    }
  }

  const std::vector<Clause>& get_clauses() const { return clauses; }

  int new_var() { return ++var_count; }
  int num_vars() const { return var_count; }

  friend std::ostream& operator<<(std::ostream& os, const CNF& cnf)
  {
    os << "Vars:" << cnf.num_vars() << " Clauses:" << cnf.get_clauses().size() << "\n";

    for (const auto& cl : cnf.get_clauses())
    {
      for (int lit : cl) os << lit << " ";
      os << "\n";
    }

    return os;
  }

private:
  std::vector<Clause> clauses;
  int var_count = 0;
  void add_contradiction()
  {
    fix_true(1);
    fix_false(1);
  }
};

class CoverEncoding
{
public:
  static constexpr int N = P / 2;

  CoverEncoding() : cnf{}, speedIsChosen(N)
  {
    for (int i = 0; i < N; ++i) speedIsChosen[i] = cnf.new_var();

    // Fix speed 1 to be chosen
    cnf.fix_true(speedIsChosen[0]);

    cnf.add_at_most_k(speedIsChosen, K); // we allow duplicate

    add_coverage_constraints();
  }

  const std::vector<int>& speedVariables() const { return speedIsChosen; }
  const CNF& getCNF() const { return cnf; }

private:
  CNF cnf;
  std::vector<int> speedIsChosen;

  void add_coverage_constraints()
  {
    for (int pos = 0; pos < N; ++pos)
    {
      std::vector<int> clause;
      clause.reserve(N);

      for (int i = 0; i < N; ++i)
      {
        if (context.cover[i][pos]) clause.push_back(speedIsChosen[i]);
      }

      assert(!clause.empty());

      // one of the speed covering pos must be chosen.
      cnf.add_or(clause);
    }
  }
};

inline int mod_pow(int a, int e, int p)
{
  int res = 1 % p;
  a %= p;

  while (e > 0)
  {
    if (e & 1) res = (int)((long long)res * a % p);
    a = (int)((long long)a * a % p);
    e >>= 1;
  }

  return res;
}

inline int mod_inverse(int a, int p) { return mod_pow(a, p - 2, p); }

std::vector<int> get_canonical_representation(std::vector<int>&& speeds, int prime)
{
  assert(speeds.size() == K);

  std::array<long long, K> prefix;
  prefix[0] = speeds[0];
  for (int i = 1; i < K; ++i) prefix[i] = prefix[i - 1] * speeds[i] % prime;

  const auto is_less = [](const auto& a, const auto& b)
  {
    for (int i = 0; i < K; ++i)
      if (a[i] != b[i]) return a[i] < b[i];
    return false;
  };

  std::vector<int> best(K);
  int run = mod_inverse(prefix[K - 1], prime);
  for (int i = K - 1; i >= 0; --i)
  {
    int inv = (i > 0) ? run * prefix[i - 1] % prime : run;
    if (i > 0) run = run * speeds[i] % prime;

    auto tmp = speeds;
    for (int j = 0; j < K; ++j)
    {
      int v  = static_cast<long long>(speeds[j]) * inv % prime;
      tmp[j] = std::min(v, prime - v);
    }
    std::sort(tmp.begin(), tmp.end());

    if (i == K - 1 || is_less(tmp, best)) best = tmp;
  }

  return best;
}

int main()
{
  std::vector<std::vector<int>> all_models;
  CoverEncoding enc{};

  CaDiCaL::Solver solver;
  solver.set("factor", 0);
  solver.set("factorcheck", 0);

  int max_var = enc.getCNF().num_vars();

  for (const auto& cl : enc.getCNF().get_clauses())
  {
    for (int lit : cl) solver.add(lit);
    solver.add(0);
  }

  std::cout << enc.getCNF().get_clauses().size() << " base clauses" << std::endl;

  while (true)
  {
    int res = solver.solve();
    if (res != SATISFIABLE_RETURN_CODE) break;

    std::vector<int> model;
    std::vector<int> blocking;

    const auto& vars = enc.speedVariables();

    for (int i = 0; i < CoverEncoding::N; ++i)
    {
      int v   = vars[i];
      int val = solver.val(v);

      if (val > 0)
      {
        model.push_back(i + 1);
        blocking.push_back(-v);
      }
      else
      {
        blocking.push_back(v);
      }
    }

    // TODO: actually gotta sort out duplicates here.

    std::cout << "MODEL: ";
    for (int x : model) std::cout << x << " ";
    std::cout << "\n";

    all_models.push_back(model);

    for (int lit : blocking) solver.add(lit);
    solver.add(0);
  }
  std::cout << "Solution count = " << all_models.size() << std::endl;
  std::set<std::vector<int>> canonical;
  for (auto x : all_models)
  {
    canonical.insert(get_canonical_representation(std::move(x), P));
  }
  std::cout << "Canonical count = " << canonical.size() << std::endl;
}

