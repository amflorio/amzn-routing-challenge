#ifndef tspheuristic_h
#define tspheuristic_h

#include <random>
#include <vector>
#include "TSPSolution.h"

class TSPHeuristic {
    private:
        std::vector<std::vector<double>> costs;
        std::random_device rd;
        std::mt19937 g;
        TSPSolution cheapestInsertion() const;
        TSPSolution farthestInsertion() const;
    public:
        TSPHeuristic(std::vector<std::vector<double>> csts)
                : costs{std::move(csts)}, g(rd()) {}
        std::vector<TSPSolution> pool(size_t n, size_t guide, bool fix);
        TSPSolution randomInsertion(size_t guide, bool fix);
        TSPSolution solve();
};

#endif

