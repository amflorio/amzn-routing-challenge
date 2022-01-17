#include <algorithm>
#include "SequenceBuilder.h"
#include "TSPHeuristic.h"

using namespace std;

void SequenceBuilder::adjustIndicesAndCosts(
        unordered_map<string, size_t>& stop_to_idx,
        vector<string>& idx_to_stop, vector<vector<double>>& costs,
        const string& entry, const string& exit) {
    if (entry==exit)
        cout<<"warning: entry and exit are the same"<<endl;
    // entry must become index 1
    if (stop_to_idx.at(entry)!=1) {
        const auto curr_entry_idx=stop_to_idx.at(entry);
        const auto curr_stop_at_1=idx_to_stop[1];
        // update stop_to_idx
        stop_to_idx.at(entry)=1;
        stop_to_idx.at(curr_stop_at_1)=curr_entry_idx;
        // update idx_to_stop
        idx_to_stop[1]=entry;
        idx_to_stop[curr_entry_idx]=curr_stop_at_1;
        // update cost matrix
        swap(costs[curr_entry_idx], costs[1]);          // swap rows
        for (size_t i=0; i<costs.size(); ++i)           // swap columns
            swap(costs[i][curr_entry_idx], costs[i][1]);
    }
    // exit must become index 2
    if (stop_to_idx.at(exit)!=2) {
        const auto curr_exit_idx=stop_to_idx.at(exit);
        const auto curr_stop_at_2=idx_to_stop[2];
        // update stop_to_idx
        stop_to_idx.at(exit)=2;
        stop_to_idx.at(curr_stop_at_2)=curr_exit_idx;
        // update idx_to_stop
        idx_to_stop[2]=exit;
        idx_to_stop[curr_exit_idx]=curr_stop_at_2;
        // update cost matrix
        swap(costs[curr_exit_idx], costs[2]);           // swap rows
        for (size_t i=0; i<costs.size(); ++i)           // swap columns
            swap(costs[i][curr_exit_idx], costs[i][2]);
    }
}

vector<Sequence> SequenceBuilder::buildGuided(const Route& r, size_t n,
        const vector<BasicStop>& guide) {
    if (guide.empty())
        cout<<"warning: guide is empty"<<endl;
    // create indices to convert from stopids (strings) to numerical indexes
    unordered_map<string, size_t> stop_to_idx;
    vector<string> idx_to_stop(guide.size(), "guide");
    // note: the station is already included in the guide tour as index 0
    const auto& stops=r.stops();
    for (const auto& kv : stops) {
        if (r.getStop(kv.first).type()==Stop::Type::station) {
            stop_to_idx[kv.first]=0;
            idx_to_stop[0]=kv.first;
            // consistency check
            if (guide[0].lat()!=r.getStop(kv.first).lat()
                    || guide[0].lon()!=r.getStop(kv.first).lon())
                cout<<"warning: guide tour not starting at station"<<endl;
        } else {
            stop_to_idx[kv.first]=idx_to_stop.size();
            idx_to_stop.push_back(kv.first);
        }
    }
    // create cost matrix for the TSP heuristic (symmetric in this case)
    const size_t N=guide.size()+stops.size()-1;
    vector<vector<double>> costs(N, vector<double>(N, 0));
    // reference latitude for equirectangular projection: station lat
    const double lat0=r.getStop(idx_to_stop[0]).lat()*(3.14159/180);
    for (size_t i=1; i<N; ++i)
        for (size_t j=0; j<i; ++j) {
            double lat_i=i<guide.size()?guide[i].lat()
                    :r.getStop(idx_to_stop[i]).lat();
            double lon_i=i<guide.size()?guide[i].lon()
                    :r.getStop(idx_to_stop[i]).lon();
            double lat_j=j<guide.size()?guide[j].lat()
                    :r.getStop(idx_to_stop[j]).lat();
            double lon_j=j<guide.size()?guide[j].lon()
                    :r.getStop(idx_to_stop[j]).lon();
            // we don't bother about scaling here
            double diffx=abs(lon_i-lon_j)*cos(lat0);
            double diffy=abs(lat_i-lat_j);
            costs[i][j]=sqrt(diffx*diffx+diffy*diffy);
            costs[j][i]=costs[i][j];
        }
    // pool diverse set of sequences that are similar to the guide tour
    TSPHeuristic tsp(move(costs));
    auto tsppool=tsp.pool(n, guide.size(), true);
    // convert all solutions to Sequence's and save
    vector<Sequence> seqpool;
    for (const auto& sol : tsppool) {
        auto tour=sol.tour();
        // remove guid'ed nodes
        tour.erase(remove_if(tour.begin(), tour.end(),
                [&idx_to_stop](size_t i){return idx_to_stop[i]=="guide";}),
                tour.end());
        auto seq=toSequence(r, idx_to_stop, move(tour));
        r.setupTiming(seq);
        seqpool.push_back(move(seq));
    }
    return seqpool;
}

vector<Sequence> SequenceBuilder::buildOrdered(const Route& r, size_t n,
        const vector<string>& order, double p_micro, double p_nano) {
    unordered_map<string, size_t> stop_to_idx;
    vector<string> idx_to_stop(1);    // reserve index 0 to station
    for (size_t i=0; i<order.size(); ++i) {
        stop_to_idx[order[i]]=i+1;
        idx_to_stop.push_back(order[i]);
    }
    for (const auto& kv : r.stops()) {
        if (r.getStop(kv.first).type()==Stop::Type::station) {
            stop_to_idx[kv.first]=0;
            idx_to_stop[0]=kv.first;
        } else if (stop_to_idx.count(kv.first)==0) {
            stop_to_idx[kv.first]=idx_to_stop.size();
            idx_to_stop.push_back(kv.first);
        }
    }
    TSPHeuristic tsp(createCostMatrix(r, stop_to_idx, p_micro, p_nano));
    auto tsppool=tsp.pool(n, order.size()+1, false);
    vector<Sequence> seqpool;
    seqpool.reserve(tsppool.size());
    for (const auto& sol : tsppool) {
        auto seq=toSequence(r, idx_to_stop, sol.tour());
        r.setupTiming(seq);
        seqpool.push_back(move(seq));
    }
    return seqpool;
}

vector<Sequence> SequenceBuilder::buildRandom(const Route& r,
        const vector<pair<string, string>>& combis, double p_micro,
        double p_nano) {
    // create indices to convert from stopids (strings) to numerical indexes
    unordered_map<string, size_t> stop_to_idx;
    vector<string> idx_to_stop(1);    // reserve index 0 to station
    for (const auto& kv : r.stops()) {
        if (r.getStop(kv.first).type()==Stop::Type::station) {
            stop_to_idx[kv.first]=0;
            idx_to_stop[0]=kv.first;
        } else {
            stop_to_idx[kv.first]=idx_to_stop.size();
            idx_to_stop.push_back(kv.first);
        }
    }
    auto costs=createCostMatrix(r, stop_to_idx, p_micro, p_nano);
    vector<Sequence> seqpool;
    for (const auto& p : combis) {
        if (p.first==p.second)
            cout<<"warning: entry and exit are the same"<<endl;
        // adjust indices and costs such that entry/exit become indices 1/2
        adjustIndicesAndCosts(stop_to_idx,idx_to_stop,costs,p.first,p.second);
        if (stop_to_idx.at(p.first)!=1 || stop_to_idx.at(p.second)!=2)
            cout<<"warning: incorrect entry or exit index"<<endl;
        // 3=station, entry and exit
        seqpool.push_back(toSequence(r, idx_to_stop,
                TSPHeuristic(costs).randomInsertion(3, true).tour()));
        r.setupTiming(seqpool.back());
    }
    return seqpool;
}

vector<Sequence> SequenceBuilder::buildRandom(const Route& r, size_t n,
        double p_micro, double p_nano) {
    // create indices to convert from stopids (strings) to numerical indexes
    unordered_map<string, size_t> stop_to_idx;
    vector<string> idx_to_stop;
    const auto& stops=r.stops();
    for (const auto& kv : stops) {
        stop_to_idx[kv.first]=idx_to_stop.size();
        idx_to_stop.push_back(kv.first);
    }
    // pool diverse set of (non-optimal) TSP solutions
    TSPHeuristic tsp(createCostMatrix(r, stop_to_idx, p_micro, p_nano));
    auto tsppool=tsp.pool(n, 0, false);
    // convert all solutions to Sequence's and save
    vector<Sequence> seqpool;
    seqpool.reserve(tsppool.size());
    for (const auto& sol : tsppool) {
        auto seq=toSequence(r, idx_to_stop, sol.tour());
        r.setupTiming(seq);
        /* equality does not hold anymore when applying transition penalties
        if (abs(sol.value()-seq.duration())>1e-8)
            cout<<"warning: sequence duration inconsistent"<<endl;
        */
        seqpool.push_back(move(seq));
    }
    return seqpool;
}

vector<vector<double>> SequenceBuilder::createCostMatrix(const Route& r,
        const unordered_map<string, size_t>& stop_to_idx, double p_micro,
        double p_nano) {
    const auto& stops=r.stops();
    // create cost matrix from travel times
    vector<vector<double>> costs(stops.size(), vector<double>(stops.size(), 0));
    const auto& ttimes=r.travelTimes();
    for (const auto& kv1 : stops)
        for (const auto& kv2 : stops)
            if (kv1.first!=kv2.first) {
                double t=ttimes.travelTime(kv1.first, kv2.first);
                const auto& microz1=r.getStop(kv1.first).microZone();
                const auto& microz2=r.getStop(kv2.first).microZone();
                if (microz1=="" || microz2=="" || microz1!=microz2)
                    t*=(1+p_micro);
                const auto& nanoz1=r.getStop(kv1.first).nanoZone();
                const auto& nanoz2=r.getStop(kv2.first).nanoZone();
                if (nanoz1=="" || nanoz2=="" || nanoz1!=nanoz2)
                    t*=(1+p_nano);
                costs[stop_to_idx.at(kv1.first)][stop_to_idx.at(kv2.first)]=t;
            }
    return costs;
}

Sequence SequenceBuilder::toSequence(const Route& r,
        const vector<string>& idx_to_stop, const vector<size_t>& tour) {
    size_t k=0;     // advance until tour starts at the depot
    while (r.getStop(idx_to_stop[tour[k]]).type()!=Stop::Type::station)
        k++;
    Sequence seq({});
    for (size_t i=k; i<tour.size(); ++i)
        seq.addStop(idx_to_stop[tour[i]]);
    for (size_t i=0; i<k; ++i)
        seq.addStop(idx_to_stop[tour[i]]);
    return seq;
}

