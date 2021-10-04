#include <iostream>
#include <cmath>
#include <vector>

uint32_t cwMin = 15;
uint32_t cwMax = 1023;
uint32_t numRetry = 7;
uint32_t Ts = 2870;
uint32_t Tc = Ts;
uint32_t Te = 9;
uint32_t L = 2000;

double p(double tau, int32_t n) {
    return 1 - pow(1 - tau, n - 1);
}

double tau(double p) {
    double val = 0;
    double f = 0;
    double tmp = 1;
    for (size_t i = 0; i < numRetry; i++, tmp *= p) {
        f += tmp;
        uint32_t wi = (cwMin + 1) << i;
        if (wi > (cwMax + 1)) {
            wi = cwMax + 1;
        }

        val += tmp * ((wi + 1.0) / 2.0); 
    }
    return f / val;
}

double eqf(double p_, uint32_t n) {
    return p_ - p(tau(p_), n);
}

double lineSearch(uint32_t n, double eps=1e-5) {
    double pBest = 0;
    double res = abs(eqf(0, n));
    for (double p = 0.0; p < 1.0; p += eps) {
        if (abs(eqf(p, n)) < res) {
            res = abs(eqf(p, n));
            pBest = p;
        }
    }
    return tau(pBest);
}

double binSearch(uint32_t n, double eps = 1e-9) {
    double p_l = 0.0;
    double p_r = 1.0;
    while (p_r - p_l > eps) {
        double p_c = (p_l + p_r) / 2.0;
        if (eqf(p_l, n) * eqf(p_c, n) < 0) {
            p_r = p_c;
        } else {
            p_l = p_c;
        }
    }
    return tau(p_l);
}

int main() {
    std::vector<uint32_t> numSt = {1, 2, 4, 8, 16, 32, 64, 128, 200, 21, 47, 72, 27, 10};
    for (auto n : numSt) {
        std::cout << "numST: " << n << "        Bandwidth: ";
        double tau = binSearch(n);
        double pe = pow(1 - tau, n);
        double ps = n * tau * pow(1 - tau, n - 1);
        double pc = 1 - pe - ps;
        double Tavg = Te * pe + Ts * ps + Tc * pc;
        double S = (L * ps) / Tavg;
        if (n == 1) {
            S = L / (Ts + (cwMin / 2.0) * Te);
        }
        std::cout << S * 8 << " Mbit/s" <<  std::endl;
    }
    return 0;
}
