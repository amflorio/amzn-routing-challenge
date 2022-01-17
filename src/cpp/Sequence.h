#ifndef sequence_h
#define sequence_h

#include <algorithm>
#include <deque>
#include <fstream>
#include <iostream>
#include <map>
#include <string>
#include <unordered_map>
#include <vector>
#include <boost/functional/hash.hpp>
#include "TTMatrix.h"

class AlgoInput;
class Route;
class TestRoute;
class Sequence {
    private:
        double dur=0;       // does not include service times
        double earliness_=0, lateness_=0;
        double max_earliness=0, max_lateness=0;
        std::vector<double> st_earliness, st_lateness;
        int miss_early=-1, miss_late=-1;
        int sim_macro=-1, sim_micro=-1, sim_nano=-1;    // similarity measures
        int trans_macro=-1, trans_micro=-1, trans_nano=-1;
        std::vector<std::string> stops_;
        std::unordered_map<std::string, size_t> stopid_to_idx;
        static double distErp(const std::string& s1, const std::string& s2,
                const TTMatrix& ttimes, const double g) {
            return s1=="gap"||s2=="gap" ? g : ttimes.travelTime(s1, s2);
        }
        typedef std::deque<std::string> Seq;
        typedef std::pair<size_t, size_t> sizet_pair;
        static std::pair<double, size_t> erpPerEditHelper(const Seq& actual,
                size_t h_actual, const Seq& sub, size_t h_sub,
                const TTMatrix& ttimes, const double g,
                std::unordered_map<sizet_pair, std::pair<double, size_t>,
                boost::hash<sizet_pair>>& memo);
        static double gapSum(const Seq& s, double g) {return s.size()*g;}
    public:
        Sequence(std::vector<std::string> stps);
        void addStop(std::string stopid) {
            stopid_to_idx[stopid]=stops_.size();
            stops_.push_back(std::move(stopid));
        }
        double deviation(const Sequence& s) const;
        int distance(const Sequence& s) const;
        double duration() const {return dur;}
        double earliness() const {return earliness_;}
        int earlyArrivals() const {return miss_early;}
        double earliness(int s) const {return st_earliness[s];}
        static double erpPerEdit(const Seq& actual, const Seq& sub,
                const TTMatrix& ttimes, const double g) {
            std::unordered_map<sizet_pair, std::pair<double, size_t>,
                    boost::hash<sizet_pair>> memo;
            auto res=erpPerEditHelper(actual, 0, sub, 0, ttimes, g, memo);
            return res.second==0 ? 0 : res.first/res.second;
        }
        void exportJSON(std::ofstream& os) const;
        std::unordered_map<std::string, double> features(const Route& r,
                const std::unordered_map<std::string, double>& stats) const;
        int lateArrivals() const {return miss_late;}
        double lateness() const {return lateness_;}
        double lateness(int s) const {return st_lateness[s];}
        double maxEarliness() const {return max_earliness;}
        double maxLateness() const {return max_lateness;}
        static int macroTransitions(const Sequence& seq, const Route& r);
        static int microTransitions(const Sequence& seq, const Route& r);
        static int nanoTransitions(const Sequence& seq, const Route& r);
        int macroSimilarity() const {return sim_macro;}
        int microSimilarity() const {return sim_micro;}
        int nanoSimilarity() const {return sim_nano;}
        int macroTransitions() const {return trans_macro;}
        int microTransitions() const {return trans_micro;}
        int nanoTransitions() const {return trans_nano;}
        static double score(const Route& r, const Sequence& prop,
                const Sequence& actual);
        void setDuration(double d) {dur=d;}
        void setEarliness(double e) {earliness_=e;}
        void setEarlyArrivals(int a) {miss_early=a;}
        void setLateArrivals(int a) {miss_late=a;}
        void setLateness(double l) {lateness_=l;}
        void setMaxEarliness(double e) {max_earliness=e;}
        void setMaxLateness(double l) {max_lateness=l;}
        void setSimilarity(int macro, int micro, int nano) {
            sim_macro=macro;
            sim_micro=micro;
            sim_nano=nano;
        }
        void setStopEarliness(std::vector<double> e){st_earliness=std::move(e);}
        void setStopLateness(std::vector<double> l){st_lateness=std::move(l);}
        void setTransitions(int macro, int micro, int nano) {
            trans_macro=macro;
            trans_micro=micro;
            trans_nano=nano;
        }
        static std::unordered_map<std::string, double> statistics(
                const std::vector<Sequence>& seqs);
        const std::vector<std::string>& stops() const {return stops_;}
        std::vector<std::string>& stops() {return stops_;}
        void swap(size_t i, size_t j) {
            std::swap(stops_[i], stops_[j]);
            stopid_to_idx.at(stops_[i])=i;
            stopid_to_idx.at(stops_[j])=j;
        }
};

#endif

