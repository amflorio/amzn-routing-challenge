#include <algorithm>
#include <deque>
#include <iostream>
#include "TrainingRoute.h"

using namespace std;

// TODO: refactor code duplicated in Sequence::score
double TrainingRoute::computeScore(const Sequence& prop) const {
    deque<string> act(seq.stops().begin()+1, seq.stops().end());  // no station
    deque<string> prp(prop.stops().begin()+1, prop.stops().end());
    auto normtts=ttimes.normalize();
    return seq.deviation(prop)*Sequence::erpPerEdit(act, prp, normtts, 1000);
}

bool TrainingRoute::setScore(const string& score) {
    if (score=="High") {
        score_=Score::high;
        return true;
    }
    if (score=="Medium") {
        score_=Score::medium;
        return true;
    }
    if (score=="Low") {
        score_=Score::low;
        return true;
    }
    return false;
}

bool TrainingRoute::setStops(const unordered_map<string, size_t>& stopid_ord) {
    vector<string> stopseq(stopid_ord.size());
    for (const auto& kv : stopid_ord) {
        addStop(kv.first);
        if (kv.second<0 || kv.second>=stopseq.size()) {
            cout<<"warning: stop order out of range"<<endl;
            return false;
        }
        stopseq[kv.second]=kv.first;
    }
    seq=Sequence(move(stopseq));
    return true;
}

TestRoute TrainingRoute::toTestRoute() const {
    return TestRoute(id_, station_, stops_, departure_, rect, ttimes);
}

