#include <algorithm>
#include <cmath>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <limits>
#include <unordered_set>
#include <boost/functional/hash.hpp>
#include "Route.h"
#include "TSPHeuristic.h"

using namespace std;

int Route::distinctMacroZones() const {
    unordered_set<string> zones;
    for (const auto& kv : stops_)
        zones.insert(kv.second.macroZone());
    return zones.size();
}

int Route::distinctMicroZones() const {
    unordered_set<string> zones;
    for (const auto& kv : stops_)
        zones.insert(kv.second.microZone());
    return zones.size();
}

int Route::distinctNanoZones() const {
    unordered_set<string> zones;
    for (const auto& kv : stops_)
        zones.insert(kv.second.nanoZone());
    return zones.size();
}

void Route::exportTikZ(const string& texfile) const {
    exportTikZ(seq, texfile);
}

void Route::exportTikZ(const Sequence& s, const string& texfile) const {
    ofstream os(texfile);
    os<<setprecision(15);
    os<<"\\PassOptionsToPackage{usenames,x11names}{xcolor}"<<endl;
    os<<"\\documentclass[tikz]{standalone}"<<endl;
    os<<"\\usetikzlibrary{decorations.markings}"<<endl;
    os<<"\\tikzset{>=latex}"<<endl;
    os<<"\\tikzset{->-/.style={decoration={markings, mark=at position .5 with "
            "{\\arrow{>}}},postaction={decorate}}}"<<endl;
    os<<"\\begin{document}"<<endl;
    os<<"\\begin{tikzpicture}"<<endl;
    vector<pair<double, double>> xycoords;
    double lat0=getStop(s.stops()[0]).lat()*(3.14159/180);  // station's lat
    for (const auto& sid : s.stops()) {
        const auto& stop=getStop(sid);
        xycoords.emplace_back(stop.lon(), stop.lat()*cos(lat0));
    }
    double offsetx=accumulate(xycoords.begin(), xycoords.end(), 0.0,
            [](double a, const pair<double, double>& xy)
            {return a+xy.first;})/xycoords.size();
    double offsety=accumulate(xycoords.begin(), xycoords.end(), 0.0,
            [](double a, const pair<double, double>& xy)
            {return a+xy.second;})/xycoords.size();
    double scaling=400;
    for (size_t i=0; i<xycoords.size(); ++i) {
        double fromx = ((i>0 ? xycoords[i-1].first  : xycoords.back().first)
                -offsetx)*scaling;
        double fromy = ((i>0 ? xycoords[i-1].second : xycoords.back().second)
                -offsety)*scaling;
        double tox = (xycoords[i].first-offsetx)*scaling;
        double toy = (xycoords[i].second-offsety)*scaling;
        os<<"\\draw [->-] ("<<fromx<<","<<fromy<<") -- ("<<tox<<","<<toy<<");"
                <<endl;
        double e=s.earliness(i);
        double l=s.lateness(i);
        double sc=100;
        if (e>0) {
            os<<"\\draw[green] ("<<tox<<","<<toy<<") circle ("<<e/sc
                    <<"pt) node[below] {"<<e<<"};"<<endl;
        }
        if (l>0) {
            os<<"\\draw[red] ("<<tox<<","<<toy<<") circle ("<<l/sc
                    <<"pt) node[below] {"<<l<<"};"<<endl;
        }
    }
    os<<"\\end{tikzpicture}"<<endl;
    os<<"\\end{document}"<<endl;
}

string Route::mainMacroZone() const {
    if (stops_.empty()) {
        cout<<"warning: no stops in route"<<endl;
        return "";
    }
    unordered_map<string, int> zcounter;
    for (const auto& kv : stops_)
        zcounter[kv.second.macroZone()]++;
    return max_element(zcounter.begin(), zcounter.end(),
            [](const pair<string, int>& kv1, const pair<string, int>& kv2)
            {return kv1.second<kv2.second;})->first;
}

string Route::mainMicroZone() const {
    if (stops_.empty()) {
        cout<<"warning: no stops in route"<<endl;
        return "";
    }
    unordered_map<string, int> zcounter;
    for (const auto& kv : stops_)
        zcounter[kv.second.microZone()]++;
    return max_element(zcounter.begin(), zcounter.end(),
            [](const pair<string, int>& kv1, const pair<string, int>& kv2)
            {return kv1.second<kv2.second;})->first;
}

unordered_map<string, double> Route::ratioMacroZones() const {
    unordered_map<string, double> ratios;
    size_t tot=0;
    for (const auto& kv : stops_) {
        const auto& s=kv.second;
        if (s.type()==Stop::Type::dropoff) {
            auto macro=s.macroZone();
            if (macro!="") {
                ratios[macro]+=1;
                tot++;
            }
        }
    }
    for (auto& kv : ratios)
        kv.second/=tot;
    return ratios;
}

void Route::setDeparture(const string& datetime) {
    using namespace date;
    istringstream in(datetime);
    in>>parse("%Y-%m-%d %H:%M:%S", departure_);
}

void Route::setupRectangle() {
    double lat0=0;
    for (const auto& kv : stops_) {
        const auto& s=kv.second;
        if (s.type()==Stop::Type::station)
            lat0=s.lat()*(3.14159/180);
    }
    double left=numeric_limits<double>::max();
    double right=numeric_limits<double>::lowest();
    double bottom=numeric_limits<double>::max();
    double top=numeric_limits<double>::lowest();
    for (const auto& kv : stops_) {
        const auto& s=kv.second;
        if (s.type()==Stop::Type::dropoff) {
            const double x=s.lon();
            const double y=s.lat()*cos(lat0);
            if (x<left)
                left=x;
            if (x>right)
                right=x;
            if (y<bottom)
                bottom=y;
            if (y>top)
                top=y;
        }
    }
    rect.setBounds(left, right, bottom, top);
}

void Route::setupSimilarity(Sequence& seq, const RoutingPattern& patt) const {
    auto stops=seq.stops();
    // remove dropoff stops with unknown zones
    stops.erase(remove_if(stops.begin(), stops.end(), [&](const string& s)
            {return getStop(s).type()==Stop::Type::dropoff
                 && getStop(s).nanoZone()=="";}), stops.end());
    typedef pair<string, string> strpair;
    unordered_set<strpair, boost::hash<strpair>> macro, micro, nano;
    int macrosim=0, microsim=0, nanosim=0;
    for (size_t i=0; i<stops.size(); ++i) {
        const auto& a=getStop(i==0?stops.back():stops[i-1]);
        const auto& b=getStop(stops[i]);
        if (macro.count({a.macroZone(), b.macroZone()})==0) {
            macrosim+=patt.countMacro(station_, a.macroZone(), b.macroZone());
            macro.insert({a.macroZone(), b.macroZone()});
        }
        if (micro.count({a.microZone(), b.microZone()})==0) {
            microsim+=patt.countMicro(station_, a.microZone(), b.microZone());
            micro.insert({a.microZone(), b.microZone()});
        }
        if (nano.count({a.nanoZone(), b.nanoZone()})==0) {
            nanosim+=patt.countNano(station_, a.nanoZone(), b.nanoZone());
            nano.insert({a.nanoZone(), b.nanoZone()});
        }
    }
    seq.setSimilarity(macrosim, microsim, nanosim);
    seq.setTransitions(Sequence::macroTransitions(seq, *this),
            Sequence::microTransitions(seq, *this),
            Sequence::nanoTransitions(seq, *this));
}

void Route::setupTiming(Sequence& seq) const {
    const auto& stopseq=seq.stops();
    if (stopseq.empty() || getStop(stopseq[0]).type()!=Stop::Type::station)
        cout<<"warning: invalid sequence"<<endl;
    // compute duration of the actual sequence
    double dur=0;
    for (size_t i=1; i<stopseq.size(); ++i)
        dur+=ttimes.travelTime(stopseq[i-1], stopseq[i]);
    dur+=ttimes.travelTime(stopseq.back(), stopseq[0]);     // back to station
    seq.setDuration(dur);
    auto tp_early=departure_;           // worst-case (too early)
    auto tp_late=departure_;            // worst-case (too late)
    double earliness=0, lateness=0;
    vector<double> st_earliness(stopseq.size(), 0);
    vector<double> st_lateness(stopseq.size(), 0);
    int miss_early=0, miss_late=0;
    double max_earliness=0, max_lateness=0;
    for (size_t i=1; i<stopseq.size(); ++i) {
        const Stop& s=getStop(stopseq[i]);
        tp_early+=chrono::seconds(static_cast<int>(0.5
                +0.75*ttimes.travelTime(stopseq[i-1], s.id())));
        tp_late+=chrono::seconds(static_cast<int>(0.5
                +1.25*ttimes.travelTime(stopseq[i-1], s.id())));
        if (s.hasTW()) {
            if (tp_early<s.startTW()) {
                double e=chrono::duration_cast<chrono::seconds>(
                        s.startTW()-tp_early).count();
                earliness+=e;
                st_earliness[i]=e;
                miss_early++;
                if (e>max_earliness)
                    max_earliness=e;
            }
            if (tp_late>s.endTW()) {
                double l=chrono::duration_cast<chrono::seconds>(
                        tp_late-s.endTW()).count();
                lateness+=l;
                st_lateness[i]=l;
                miss_late++;
                if (l>max_lateness)
                    max_lateness=l;
            }
        }
        // takes into account service times of packages delivered at s
        double stime=accumulate(s.packages().begin(), s.packages().end(), 0.0,
               [](double acc, const Package& p){return acc+p.serviceTime();});
        tp_early+=chrono::seconds(static_cast<int>(0.5+0.90*stime));
        tp_late+=chrono::seconds(static_cast<int>(0.5+1.10*stime));
    }
    seq.setEarliness(earliness);
    seq.setLateness(lateness);
    seq.setStopEarliness(st_earliness);
    seq.setStopLateness(st_lateness);
    seq.setEarlyArrivals(miss_early);
    seq.setLateArrivals(miss_late);
    seq.setMaxEarliness(max_earliness);
    seq.setMaxLateness(max_lateness);
}

