#include <cassert>
#include <iostream>
#include <unordered_set>
#include "AlgoInput.h"
#include "TestRoute.h"
#include "TSPHeuristic.h"

using namespace std;

unordered_map<string, double> TestRoute::features(const AlgoInput& input) const{
    unordered_map<string, double> features;
    const auto& r=*this;
    // station (one-hot encoding)
    features.insert({r.station(), 1});
    // number of dropoff stops
    features.insert({"n_stops", r.stops().size()-1});
    // number of overlapping routes in algoinput (varying ranges)
    const double eps=1e-10;
    features.insert({"p_olaps_0.80-1.00",input.ratioOverlaps(r,0.80+eps,1.00)});
    features.insert({"p_olaps_0.50-0.80",input.ratioOverlaps(r,0.50+eps,0.80)});
    features.insert({"p_olaps_0.80-1.00",input.ratioOverlaps(r,0.80+eps,1.00)});
    features.insert({"p_olaps_0.65-1.00",input.ratioOverlaps(r,0.65+eps,1.00)});
    features.insert({"p_olaps_0.50-1.00",input.ratioOverlaps(r,0.50+eps,1.00)});
    features.insert({"p_olaps_0.25-1.00",input.ratioOverlaps(r,0.25+eps,1.00)});
    // time windows: total number (varying lengths) and percentage
    features.insert({"n_tws", r.countTimeWindows(0, 361)});
    features.insert({"p_tws", features.at("n_tws")/features.at("n_stops")});
    features.insert({"n_strict_tws", r.countTimeWindows(0, 241)});
    features.insert({"p_strict_tws", features.at("n_strict_tws")
            /features.at("n_stops")});
    // distance from the station to the nearest dropoff stop
    features.insert({"dropoff_nearest", r.distanceNearestDropoff()});
    // area of the dropoff stops bounding rectangle
    features.insert({"dropoff_area", r.rectangle().area()});
    // dropoff density (dropoffs per unit of area)
    features.insert({"dropoff_density",
            features.at("n_stops")/features.at("dropoff_area")});
    // number of packages: total and avg. per stop
    features.insert({"n_packs", r.numPackages()});
    features.insert({"packs_per_stop", features.at("n_packs")
            /features.at("n_stops")});
    // total service time
    features.insert({"service_time", r.serviceTime()});
    // basis expansion: only between station feature and other features
    unordered_map<string, double> expansion;
    for (const auto& kv : features)
        if (kv.first!=r.station())
            expansion.insert({r.station()+"_*_"+kv.first, kv.second});
    features.insert(expansion.begin(), expansion.end());
    // macro zones (percentage encoding)
    auto mzones=r.ratioMacroZones();
    features.insert(mzones.begin(), mzones.end());
    // protection against nan's or inf's
    for (auto& kv : features)
        if (!isfinite(kv.second)) {
            cout<<"warning: feature \""<<kv.first<<"\" is '"<<kv.second
                    <<"'  ;  setting to zero"<<endl;
            kv.second=0;
        }
    return features;
}

