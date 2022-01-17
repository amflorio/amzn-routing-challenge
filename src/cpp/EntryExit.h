#ifndef entryexit_h
#define entryexit_h

#include <string>
#include <vector>
#include "Algorithm.h"

class EntryExit : public Algorithm {
    private:
        const size_t poolsize;
        const double p_micro;
        const double p_nano;
        const bool localsearch;
    public:
        EntryExit(size_t s, double pm, double pn, bool ls) : poolsize{s},
                p_micro{pm}, p_nano{pn}, localsearch{ls} {
            id_="EE-"+std::to_string(p_micro)+"-"+std::to_string(p_nano);
        }
        std::vector<Sequence> findSequences(size_t n, const Route& r,
                const AlgoInput& input) const override;
        std::vector<Sequence> findSequences(const Route& r,
                const AlgoInput& input) const override {
            return findSequences(poolsize, r, input);
        }
        bool randomized() const override {return true;}
};

#endif

