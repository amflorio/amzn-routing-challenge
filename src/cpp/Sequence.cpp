#include <algorithm>
#include <deque>
#include <iostream>
#include "Learner.h"
#include "Sequence.h"
#include "TestRoute.h"

using namespace std;

Sequence::Sequence(vector<string> stps) : stops_{move(stps)} {
    for (size_t i=0; i<stops_.size(); ++i)
        stopid_to_idx[stops_[i]]=i;
}

double Sequence::deviation(const Sequence& sub) const {
    // 'this' sequence is the actual (not sure if operator is commutative)
    if (this->stops_.size()!=sub.stops_.size())
        cout<<"warning: sequence lengths differ (actual: "<<this->stops_.size()
                <<"  proposed: "<<sub.stops_.size()<<")"<<endl;
    vector<int> comp;
    for (size_t i=1; i<sub.stops_.size(); ++i)
        comp.push_back(this->stopid_to_idx.at(sub.stops_[i])-1);//-1 ignores dep
    int compsum=0;
    for (size_t i=1; i<comp.size(); ++i)
        compsum+=abs(comp[i]-comp[i-1])-1;
    size_t n=stops_.size()-1;
    return (2.0/(n*(n-1)))*compsum;
}

int Sequence::distance(const Sequence& s) const {
    int d=0;
    for (size_t i=0; i<stops_.size(); ++i) {
        size_t pos_s=s.stopid_to_idx.at(stops_[i]);
        d += i<pos_s ? pos_s-i : i-pos_s;
    }
    return d;
}

pair<double, size_t> Sequence::erpPerEditHelper(const Seq& actual,
        size_t h_actual, const Seq& sub, size_t h_sub, const TTMatrix& ttimes,
        const double g, unordered_map<sizet_pair, pair<double, size_t>,
        boost::hash<sizet_pair>>& memo) {
    if (memo.count({h_actual, h_sub})==1)
        return memo.at({h_actual, h_sub});
    double d=0;
    size_t count=0;
    if (h_sub==sub.size()) {                    // if sub is "empty"
        d=(actual.size()-h_actual)*g;
        count=actual.size()-h_actual;
    } else if (h_actual==actual.size()) {       // if actual is "empty"
        d=(sub.size()-h_sub)*g;
        count=sub.size()-h_sub;
    } else {
        auto next_actual=h_actual+1;
        auto next_sub=h_sub+1;
        auto A=erpPerEditHelper(actual,next_actual,sub,next_sub,ttimes,g,memo);
        auto B=erpPerEditHelper(actual,next_actual,sub,h_sub,ttimes,g,memo);
        auto C=erpPerEditHelper(actual,h_actual,sub,next_sub,ttimes,g,memo);
        auto headactual=actual[h_actual];
        auto headsub=sub[h_sub];
        // TODO: make this less obscure (no need to call distErp here)
        auto optA=A.first+distErp(headactual, headsub, ttimes, g);
        auto optB=B.first+distErp(headactual, "gap", ttimes, g);
        auto optC=C.first+distErp(headsub, "gap", ttimes, g);
        d=min({optA, optB, optC});
        if (d==optA) {
            if (headactual==headsub)
                count=A.second;
            else
                count=A.second+1;
        } else if (d==optB)
            count=B.second+1;
        else
            count=C.second+1;
    }
    memo.insert({{h_actual, h_sub}, {d, count}});
    return {d, count};
}

void Sequence::exportJSON(ofstream& os) const {
    for (size_t i=0; i<stops_.size(); ++i) {
        os<<"      \""<<stops_[i]<<"\": "<<i;
        if (i<stops_.size()-1)
            os<<",";        // last one has no comma ...
        os<<endl;
    }
}

unordered_map<string, double> Sequence::features(const Route& r,
        const unordered_map<string, double>& stats) const {
    if (miss_early==-1 || miss_late==-1)
        cout<<"warning: sequence early/late arrivals not set"<<endl;
    if (sim_macro==-1 || sim_micro==-1 || sim_nano==-1)
        cout<<"warning: sequence similarity not set"<<endl;
    if (trans_macro<=0 || trans_micro<=0 || trans_nano<=0)
        cout<<"warning: sequence transitions not set or invalid"<<endl;
    // route and aggregated features
    unordered_map<string, double> rtfeats;
    rtfeats.insert({"p_with_totness", stats.at("p_with_totness")});
    rtfeats.insert({"p_wout_totness", stats.at("p_wout_totness")});
    rtfeats.insert({"p_with_out_arrivals", stats.at("p_with_out_arrivals")});
    rtfeats.insert({"p_wout_out_arrivals", stats.at("p_wout_out_arrivals")});
    rtfeats.insert({"avg_earliness", stats.at("avg_earliness")});
    rtfeats.insert({"avg_lateness", stats.at("avg_lateness")});
    rtfeats.insert({"avg_totness", stats.at("avg_earliness")
            +stats.at("avg_lateness")});
    rtfeats.insert({"n_stops", r.stops().size()-1});
    rtfeats.insert({"n_tws", r.countTimeWindows(0, 361)});
    rtfeats.insert({"n_strict_tws", r.countTimeWindows(0, 241)});
    rtfeats.insert({"p_tws", rtfeats.at("n_tws")/rtfeats.at("n_stops")});
    rtfeats.insert({"p_strict_tws", rtfeats.at("n_strict_tws")
            /rtfeats.at("n_stops")});
    rtfeats.insert({"dropoff_nearest", r.distanceNearestDropoff()});
    rtfeats.insert({"dropoff_area", r.rectangle().area()});
    rtfeats.insert({"dropoff_density", rtfeats.at("n_stops")
            /rtfeats.at("dropoff_area")});
    rtfeats.insert({"n_packs", r.numPackages()});
    rtfeats.insert({"packs_per_stop", rtfeats.at("n_packs")
            /rtfeats.at("n_stops")});
    rtfeats.insert({"service_time", r.serviceTime()});
    rtfeats.insert({"distinct_macro", r.distinctMacroZones()});
    rtfeats.insert({"distinct_micro", r.distinctMicroZones()});
    rtfeats.insert({"distinct_nano", r.distinctNanoZones()});
    // sequence features
    unordered_map<string, double> seqfeats;
    seqfeats.insert({"r_duration", dur/stats.at("min_duration")});
    seqfeats.insert({"dur_by_stime", dur/rtfeats.at("service_time")});
    seqfeats.insert({"earliness", earliness_});
    if (rtfeats.at("avg_earliness")!=0)
        seqfeats.insert({"r_earliness",earliness_/rtfeats.at("avg_earliness")});
    seqfeats.insert({"lateness", lateness_});
    if (rtfeats.at("avg_lateness")!=0)
        seqfeats.insert({"r_lateness",lateness_/rtfeats.at("avg_lateness")});
    seqfeats.insert({"totness", earliness_+lateness_});
    seqfeats.insert({"max_stop_earliness", max_earliness});
    seqfeats.insert({"max_stop_lateness", max_lateness});
    seqfeats.insert({"max_stop_totness", max_earliness+max_lateness});
    seqfeats.insert({"miss_early", miss_early});
    seqfeats.insert({"miss_late", miss_late});
    seqfeats.insert({"out_arrivals", miss_early+miss_late});
    if (rtfeats.at("n_tws")!=0) {
        seqfeats.insert({"p_out_arrivals", (miss_early+miss_late)
                /rtfeats.at("n_tws")});
    }
    seqfeats.insert({"sim_macro", sim_macro});
    seqfeats.insert({"sim_micro", sim_micro});
    seqfeats.insert({"sim_nano", sim_nano});
    seqfeats.insert({"trans_by_distinct_macro",
            trans_macro/rtfeats.at("distinct_macro")});
    seqfeats.insert({"trans_by_distinct_micro",
            trans_micro/rtfeats.at("distinct_micro")});
    seqfeats.insert({"trans_by_distinct_nano",
            trans_nano/rtfeats.at("distinct_nano")});
    seqfeats.insert({"sim_by_trans_macro", (1.0*sim_macro)/trans_macro});
    seqfeats.insert({"sim_by_trans_micro", (1.0*sim_micro)/trans_micro});
    seqfeats.insert({"sim_by_trans_nano", (1.0*sim_nano)/trans_nano});
    seqfeats.insert({"d_sim_by_trans_macro", seqfeats.at("sim_by_trans_macro")
            -stats.at("min_sim_by_trans_macro")});
    seqfeats.insert({"d_sim_by_trans_micro", seqfeats.at("sim_by_trans_micro")
            -stats.at("min_sim_by_trans_micro")});
    seqfeats.insert({"d_sim_by_trans_nano", seqfeats.at("sim_by_trans_nano")
            -stats.at("min_sim_by_trans_nano")});
    if (stats.at("max_sim_by_trans_macro")!=0) {
        seqfeats.insert({"r_sim_by_trans_macro", ((1.0*sim_macro)/trans_macro)/
                stats.at("max_sim_by_trans_macro")});
    }
    if (stats.at("max_sim_by_trans_micro")!=0) {
        seqfeats.insert({"r_sim_by_trans_micro", ((1.0*sim_micro)/trans_micro)/
                stats.at("max_sim_by_trans_micro")});
    }
    if (stats.at("max_sim_by_trans_nano")!=0) {
        seqfeats.insert({"r_sim_by_trans_nano", ((1.0*sim_nano)/trans_nano)/
                stats.at("max_sim_by_trans_nano")});
    }
    unordered_map<string, double> expansion;
    // basis expansion between station and sequence features
    for (const auto& seqfeat : seqfeats)
        expansion.insert({r.station()+"_*_"+seqfeat.first, seqfeat.second});
    // basis expansion between sequence features and route features
    for (const auto& seqfeat : seqfeats)
        for (const auto& rtfeat : rtfeats) {
            expansion.insert({seqfeat.first+"_*_"+rtfeat.first,
                    seqfeat.second*rtfeat.second});
        }
    seqfeats.insert(expansion.begin(), expansion.end());
    // intercept term
    seqfeats.insert({"intercept", 1});
    // protection against nan's or inf's
    for (auto& kv : seqfeats)
        if (!isfinite(kv.second)) {
            cout<<"warning: feature \""<<kv.first<<"\" is '"<<kv.second
                    <<"'  ;  setting to zero"<<endl;
            kv.second=0;
        }
    return seqfeats;
}

unordered_map<string,double> Sequence::statistics(const vector<Sequence>& seqs){
    unordered_map<string, double> stats;
    double min_duration=numeric_limits<double>::max();
    double min_sim_by_trans_macro=numeric_limits<double>::max();
    double min_sim_by_trans_micro=numeric_limits<double>::max();
    double min_sim_by_trans_nano=numeric_limits<double>::max();
    double max_sim_by_trans_macro=0;
    double max_sim_by_trans_micro=0;
    double max_sim_by_trans_nano=0;
    size_t with_totness=0;
    size_t with_out_arrivals=0;
    double tot_earliness=0;
    double tot_lateness=0;
    for (const auto& seq : seqs) {
        if (seq.dur < min_duration)
            min_duration=seq.dur;
        const double sbtmacro=(1.0*seq.sim_macro)/seq.trans_macro;
        const double sbtmicro=(1.0*seq.sim_micro)/seq.trans_micro;
        const double sbtnano=(1.0*seq.sim_nano)/seq.trans_nano;
        if (sbtmacro < min_sim_by_trans_macro)
            min_sim_by_trans_macro=sbtmacro;
        if (sbtmicro < min_sim_by_trans_micro)
            min_sim_by_trans_micro=sbtmicro;
        if (sbtnano < min_sim_by_trans_nano)
            min_sim_by_trans_nano=sbtnano;
        if (sbtmacro > max_sim_by_trans_macro)
            max_sim_by_trans_macro=sbtmacro;
        if (sbtmicro > max_sim_by_trans_micro)
            max_sim_by_trans_micro=sbtmicro;
        if (sbtnano > max_sim_by_trans_nano)
            max_sim_by_trans_nano=sbtnano;
        if (seq.earliness()+seq.lateness()>=50)
            with_totness++;
        if (seq.miss_early+seq.miss_late>0)
            with_out_arrivals++;
        tot_earliness+=seq.earliness();
        tot_lateness+=seq.lateness();
    }
    stats.insert({"min_duration", min_duration});
    stats.insert({"min_sim_by_trans_macro", min_sim_by_trans_macro});
    stats.insert({"min_sim_by_trans_micro", min_sim_by_trans_micro});
    stats.insert({"min_sim_by_trans_nano", min_sim_by_trans_nano});
    stats.insert({"max_sim_by_trans_macro", max_sim_by_trans_macro});
    stats.insert({"max_sim_by_trans_micro", max_sim_by_trans_micro});
    stats.insert({"max_sim_by_trans_nano", max_sim_by_trans_nano});
    double p_with_totness=(1.0*with_totness)/seqs.size();
    stats.insert({"p_with_totness", p_with_totness});
    stats.insert({"p_wout_totness", 1-p_with_totness});
    double p_with_out_arrivals=(1.0*with_out_arrivals)/seqs.size();
    stats.insert({"p_with_out_arrivals", p_with_out_arrivals});
    stats.insert({"p_wout_out_arrivals", 1-p_with_out_arrivals});
    stats.insert({"avg_earliness", tot_earliness/seqs.size()});
    stats.insert({"avg_lateness", tot_lateness/seqs.size()});
    return stats;
}

int Sequence::macroTransitions(const Sequence& seq, const Route& r) {
    int n=1;
    if (seq.stops_.empty())
        cout<<"warning: no stops in sequence"<<endl;
    string lastzone;
    for (const auto& s : seq.stops_) {
        const auto& z=r.getStop(s).macroZone();
        if (z!="" && z!=lastzone) {
            n++;
            lastzone=z;
        }
    }
    return n;
}

int Sequence::microTransitions(const Sequence& seq, const Route& r) {
    int n=1;
    if (seq.stops_.empty())
        cout<<"warning: no stops in sequence"<<endl;
    string lastzone;
    for (const auto& s : seq.stops_) {
        const auto& z=r.getStop(s).microZone();
        if (z!="" && z!=lastzone) {
            n++;
            lastzone=z;
        }
    }
    return n;
}

int Sequence::nanoTransitions(const Sequence& seq, const Route& r) {
    int n=1;
    if (seq.stops_.empty())
        cout<<"warning: no stops in sequence"<<endl;
    string lastzone;
    for (const auto& s : seq.stops_) {
        const auto& z=r.getStop(s).nanoZone();
        if (z!="" && z!=lastzone) {
            n++;
            lastzone=z;
        }
    }
    return n;
}

double Sequence::score(const Route& r, const Sequence& prop,
        const Sequence& actual) {
    deque<string> act(actual.stops_.begin()+1, actual.stops_.end());//no station
    deque<string> prp(prop.stops_.begin()+1, prop.stops_.end());
    auto normtts=r.travelTimes().normalize();
    return actual.deviation(prop)*Sequence::erpPerEdit(act, prp, normtts, 1000);
}

