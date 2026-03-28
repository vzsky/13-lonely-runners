#include <iostream>
#include <vector>
#include <iomanip>

using namespace std;

const int P = 13;
const int N = 12;

void print_vector(const string& label, int vec[N]) {
    cout << label << ": (";
    for (int i = 0; i < N; ++i) cout << vec[i] << (i == N - 1 ? "" : ", ");
    cout << ")" << endl;
}

int main() {
    int v[N] = {12, 9, 0, 1, 1, 11, 3, 1, 1, 10, 8, 6};
    int failure_matrix[P][P]; // s by a (1-12)
    bool resolved = false;

    cout << "Analyzing tuple: ";
    print_vector("v", v);
    cout << "--------------------------------------------------------" << endl;

    for (int s = 0; s < P; ++s) {
        for (int a = 1; a < P; ++a) {
            int sv[N], r[N], sum[N];
            int first_fail = 0;

            for (int k = 1; k <= N; ++k) {
                r[k-1] = (k * a) % P;
                sv[k-1] = (s * v[k-1]) % P;
                sum[k-1] = (sv[k-1] + r[k-1]) % P;
                
                if (first_fail == 0 && (sum[k-1] == 0 || sum[k-1] == 12)) {
                    first_fail = k;
                }
            }

            if (first_fail == 0) {
                cout << "\n>>> FOUND RESOLVED CASE! s = " << s << ", a = " << a << endl;
                print_vector("sv      ", sv);
                print_vector("r(a/p)  ", r);
                print_vector("Sum mod 13", sum);
                cout << "All values in {1, ..., 11}. Tuple is RESOLVABLE.\n" << endl;
                resolved = true;
            }
            failure_matrix[s][a] = first_fail;
        }
    }

    // Print Failure Matrix
    cout << "Failure Matrix (Row: s, Col: a). Value = first k causing failure (0 = resolved)" << endl;
    cout << "s \\ a |";
    for (int a = 1; a < P; ++a) cout << setw(3) << a;
    cout << "\n------|" << string(3 * 12, '-') << endl;

    for (int s = 0; s < P; ++s) {
        cout << setw(5) << s << " |";
        for (int a = 1; a < P; ++a) {
            cout << setw(3) << failure_matrix[s][a];
        }
        cout << endl;
    }

    return 0;
}
