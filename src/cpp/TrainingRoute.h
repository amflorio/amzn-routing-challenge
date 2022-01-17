#ifndef trainingroute_h
#define trainingroute_h

#include <string>
#include <unordered_map>
#include "Route.h"
#include "Stop.h"
#include "TestRoute.h"

class TrainingRoute : public Route {
    public:
        enum class Score {high, medium, low};
        TrainingRoute(std::string id) : Route(std::move(id)) {}
        double computeScore(const Sequence& prop) const;
        Score score() const {return score_;}
        void setCostInvalid(double c) {costinvalid=c;}
        bool setScore(const std::string& score);
        bool setStops(const std::unordered_map<std::string,size_t>& stopid_ord);
        TestRoute toTestRoute() const;
    private:
        Score score_;           // score of the actual sequence
        double costinvalid=0;
};

#endif

