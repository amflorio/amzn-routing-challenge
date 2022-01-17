#include <algorithm>
#include <iostream>
#include <random>
#include "EntryExit.h"
#include "LocalSearch.h"
#include "SequenceBuilder.h"

using namespace std;

vector<Sequence> EntryExit::findSequences(size_t n, const Route& r,
        const AlgoInput& input) const {
    vector<string> entrystops, exitstops;
    vector<string> dropoffs;
    for (const auto& kv : r.stops()) {
        const auto& stop=kv.second;
        if (stop.type()==Stop::Type::dropoff) {
            dropoffs.push_back(stop.id());
            size_t entries=input.pattern().countEntriesNano(stop.nanoZone());
            size_t exits=input.pattern().countExitsNano(stop.nanoZone());
            while (entries-- > 0)
                entrystops.push_back(stop.id());
            while (exits-- > 0)
                exitstops.push_back(stop.id());
        }
    }
    if (entrystops.empty())
        entrystops=dropoffs;
    if (exitstops.empty())
        exitstops=dropoffs;
    vector<pair<string, string>> combis;
    random_device rd;
    mt19937 g(rd());
    uniform_int_distribution<> randomentry(0, entrystops.size()-1);
    uniform_int_distribution<> randomexit(0, exitstops.size()-1);
    for (size_t i=0; i<n; ++i) {
        size_t idxentry=randomentry(g);
        size_t idxexit=randomexit(g);
        if (entrystops[idxentry]!=exitstops[idxexit])
            combis.push_back({entrystops[idxentry], exitstops[idxexit]});
    }
    cout<<combis.size()<<" entry/exit pairs available\n";
    auto pool=SequenceBuilder::buildRandom(r, combis, p_micro, p_nano);
    for (size_t i=0; i<combis.size(); ++i) {
        if (pool[i].stops()[1]!=combis[i].first
                || pool[i].stops().back()!=combis[i].second)
            cout<<"warning: invalid entry or exit stop"<<endl;
    }
    if (pool.empty()) {
        cout<<"no entry/exit sequence available: pooling "<<n
                <<" sequences randomly"<<endl;
        pool=SequenceBuilder::buildRandom(r, n, p_micro, p_nano);
    }
    cout<<"pool size: "<<pool.size()<<'\n';
    if (localsearch) {
        cout<<"applying local search ..."<<endl;
        for (auto& seq : pool)
            if (LocalSearch::myOpt(seq, r))
                r.setupTiming(seq);
    }
    return pool;
}

