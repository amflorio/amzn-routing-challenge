#include <unordered_set>
#include "AlgoInput.h"

using namespace std;

void AlgoInput::addRoute(const TrainingRoute& r) {
    BasicRoute br;
    br.setDeparture(r.departure());
    br.setRectangle(r.rectangle());
    br.setScore(r.score());
    double strictness=0;        // measures how strict TWs are
    unordered_set<string> packs;
    vector<BasicStop> stops;
    for (const auto& stopid : r.sequence().stops()) {
        const auto& s=r.getStop(stopid);
        BasicStop bs;
        bs.setLatLon(s.lat(), s.lon());
        size_t npacks=0;
        for (const auto& pack : s.packages()) {
            if (packs.count(pack.id())==0) {
                packs.insert(pack.id());
                npacks++;
            }
        }
        bs.setNumPackages(npacks);
        stops.push_back(move(bs));
        if (s.hasTW()) {
            double twlength=chrono::duration_cast<chrono::minutes>
                    (s.endTW()-s.startTW()).count();
            strictness+=1.0/twlength;
        }
    }
    // remove (dropoff) stops with zero packages
    stops.erase(remove_if(stops.begin()+1, stops.end(),
            [](const BasicStop& s){return s.numPackages()==0;}), stops.end());
    br.setStops(move(stops));
    br.setTWStrictness(strictness);
    routes_[r.station()].push_back(move(br));
}

