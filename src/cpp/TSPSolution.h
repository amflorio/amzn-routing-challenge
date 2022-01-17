#ifndef tspsolution_h
#define tspsolution_h

#include <vector>

class TSPSolution {
    private:
        std::vector<size_t> tour_;
        double val;
    public:
        TSPSolution(std::vector<size_t> t, double v) : tour_{std::move(t)},
                val{v} {}
        const std::vector<size_t>& tour() const {return tour_;}
        double value() const {return val;}
};

#endif

