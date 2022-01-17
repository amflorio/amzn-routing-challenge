#include "Algorithm.h"
#include "EntryExit.h"

using namespace std;

unique_ptr<Algorithm> Algorithm::bestAlgorithm() {
    return unique_ptr<Algorithm>(new EntryExit(5000, 1.25, 1.25, false));
}

bool Algorithm::better(const Sequence& s1, const Sequence& s2) {
    if (s1.nanoSimilarity()<0 || s1.nanoTransitions()<=0
            || s2.nanoSimilarity()<0 || s2.nanoTransitions()<=0)
        cout<<"warning: invalid sequence similarity and/or transitions"<<endl;
    return (1.0*s1.nanoSimilarity())/s1.nanoTransitions()
         > (1.0*s2.nanoSimilarity())/s2.nanoTransitions();
}

Sequence Algorithm::selectSequence(const Route& r, const Algorithm& alg,
        const Model& model) {
    cout<<r.id()<<" ;  station: "<<r.station()<<endl;
    auto pool=alg.findSequences(r, model.algoInput());
    for (auto& seq : pool)
        r.setupSimilarity(seq, model.algoInput().pattern());
    if (model.hasEvaluationModel()) {
        const auto stats=Sequence::statistics(pool);
        const auto p_seq=min_element(pool.begin(), pool.end(), better);
        const double max_sbtnano=(1.0*p_seq->nanoSimilarity())
                /p_seq->nanoTransitions();
        pool.erase(remove_if(pool.begin(), pool.end(), [&](const Sequence& s){
            return (1.0*s.nanoSimilarity())
                    /s.nanoTransitions()<0.925*max_sbtnano;
            }), pool.end());
        const auto& evlmodel=model.evaluationModel();
        // some manoeuvre to call only pool.size() times 'predict'
        vector<double> pscores(pool.size(), 0);
        for (size_t i=0; i<pool.size(); ++i)
            pscores[i]=evlmodel.predict(pool[i].features(r, stats));
        auto idxbest=min_element(pscores.begin(),pscores.end())-pscores.begin();
        cout<<"predicted score: "<<pscores[idxbest]<<endl;
        return pool[idxbest];
    } else {
        cout<<"warning: evaluation model not available"<<endl;
        return *min_element(pool.begin(), pool.end(), better);
    }
}

