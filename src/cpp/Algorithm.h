#ifndef algorithm_h
#define algorithm_h

#include <memory>
#include <string>
#include <vector>
#include "AlgoInput.h"
#include "Model.h"
#include "Sequence.h"

class Algorithm {
    protected:
        std::string id_;
    public:
        static std::unique_ptr<Algorithm> bestAlgorithm();
        static bool better(const Sequence& s1, const Sequence& s2);
        const std::string& id() const {return id_;}
        virtual std::vector<Sequence> findSequences(const Route& r,
                const AlgoInput& input) const=0;
        virtual std::vector<Sequence> findSequences(size_t n, const Route& r,
                const AlgoInput& input) const=0;
        static std::vector<std::shared_ptr<Algorithm>> pool(bool training);
        virtual bool randomized() const=0;
        static Sequence selectMySequence(const Route& r, const Algorithm& alg,
                const Model& model);
        static Sequence selectSequence(const Route& r, const Algorithm& alg,
                const Model& model);
        virtual ~Algorithm() {}
};

#endif

