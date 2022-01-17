#ifndef sequencebuilder_h
#define sequencebuilder_h

#include <string>
#include <vector>
#include "BasicStop.h"
#include "Route.h"
#include "Sequence.h"

class SequenceBuilder {
    private:
        static void adjustIndicesAndCosts(
                std::unordered_map<std::string, size_t>& stop_to_idx,
                std::vector<std::string>& idx_to_stop,
                std::vector<std::vector<double>>& costs,
                const std::string& entry, const std::string& exit);
        static std::vector<std::vector<double>> createCostMatrix(const Route& r,
                const std::unordered_map<std::string, size_t>& stop_to_idx,
                double p_micro, double p_nano);
        static Sequence toSequence(const Route& r,
                const std::vector<std::string>& idx_to_stop,
                const std::vector<size_t>& tour);
    public:
        static std::vector<Sequence> buildGuided(const Route& r, size_t n,
                const std::vector<BasicStop>& guide);
        static std::vector<Sequence> buildOrdered(const Route& r, size_t n,
                const std::vector<std::string>& order, double p_micro,
                double p_nano);
        static std::vector<Sequence> buildRandom(const Route& r, size_t n,
                double p_micro, double p_nano);
        static std::vector<Sequence> buildRandom(const Route& r,
                const std::vector<std::pair<std::string, std::string>>& combis,
                double p_micro, double p_nano);
};

#endif

