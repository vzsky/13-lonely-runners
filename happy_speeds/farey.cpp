#include <iostream>
#include <vector>
#include <set>
#include <cmath>
#include <algorithm>

using namespace std;

// Function to get primes up to a limit using Sieve of Eratosthenes
vector<int> get_primes(int limit) {
    vector<bool> is_prime(limit + 1, true);
    is_prime[0] = is_prime[1] = false;
    for (int p = 2; p * p <= limit; p++) {
        if (is_prime[p]) {
            for (int i = p * p; i <= limit; i += p)
                is_prime[i] = false;
        }
    }
    vector<int> primes;
    for (int p = 2; p <= limit; p++) {
        if (is_prime[p]) primes.push_back(p);
    }
    return primes;
}

// Function to compute S(p) for a given p and a pre-computed set F_k
set<double> compute_S(int p, const set<double>& Fk) {
    set<double> Sp;
    for (int n = 0; n < p; ++n) {
        double target = (double)n / p;
        // Find the largest element in Fk <= target
        auto it = Fk.upper_bound(target + 1e-12); // Small epsilon for precision
        if (it != Fk.begin()) {
            Sp.insert(*(--it));
        }
    }
    return Sp;
}

int main() {
    int p, k;
    cout << "Enter p and k: ";
    cin >> p >> k;

    // 1. Compute F(k): all rationals in [0, 1) with denominator <= k
    set<double> Fk;
    for (int d = 1; d <= k; ++d) {
        for (int n = 0; n < d; ++n) {
            Fk.insert((double)n / d);
        }
    }

    // 2. Compute S(p)
    set<double> Sp = compute_S(p, Fk);

    // 3. Find primes q < 1500 such that S(p) is a subset of S(q)
    vector<int> primes = get_primes(1500);
    vector<int> valid_qs;

    for (int q : primes) {
        set<double> Sq = compute_S(q, Fk);
        
        // Check if Sp is a subset of Sq
        bool is_subset = true;
        for (double val : Sp) {
            // Check if val exists in Sq (with precision tolerance)
            auto it = Sq.lower_bound(val - 1e-12);
            if (it == Sq.end() || abs(*it - val) > 1e-12) {
                is_subset = false;
                break;
            }
        }
        if (is_subset) valid_qs.push_back(q);
    }

    cout << "Primes q < 1500 such that S(" << p << ") is a subset of S(q):" << endl;
    for (int q : valid_qs) cout << q << " ";
    cout << endl;

    return 0;
}