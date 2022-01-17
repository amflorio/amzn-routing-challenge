#ifndef algoinput_h
#define algoinput_h

#include <numeric>
#include <string>
#include <unordered_map>
#include <vector>
#include <boost/serialization/unordered_map.hpp>
#include <boost/serialization/vector.hpp>
#include "BasicRoute.h"
#include "RoutingPattern.h"
#include "TrainingRoute.h"

class AlgoInput {
    friend class boost::serialization::access;
    private:
        std::unordered_map<std::string, std::vector<BasicRoute>> routes_;
        RoutingPattern patt;
        size_t countOverlaps(const TestRoute& r, double min, double max)
                const {
            if (!hasRoute(r.station()))
                return 0;
            size_t olaps=0;
            for (const auto& br : routes_.at(r.station())) {
                if (br.rectangle().intersects(r.rectangle())) {
                    auto olap=br.rectangle().intersectionArea(r.rectangle())
                            /r.rectangle().area();
                    if (olap>=min && olap<=max)
                        olaps++;
                }
            }
            return olaps;
        }
        template<class Archive> void serialize(Archive& ar,
                const unsigned int version) {
            ar & routes_;
            ar & patt;
        }
    public:
        void addRoute(const TrainingRoute& r);
        size_t countRoutes() const {
            return std::accumulate(routes_.begin(), routes_.end(), 0,
                    [](size_t a, const std::pair<std::string,
                    std::vector<BasicRoute>>& kv) {return a+kv.second.size();});
        }
        bool hasRoute(const std::string& station) const
                {return routes_.count(station)==1;}
        const RoutingPattern& pattern() const {return patt;}
        double ratioOverlaps(const TestRoute& r, double min, double max) const {
            return (1.0*countOverlaps(r, min, max))/countRoutes();
        }
        const std::vector<BasicRoute>& routes(const std::string& station) const
                {return routes_.at(station);}
        void setPattern(RoutingPattern p) {patt=std::move(p);}
};

#endif

