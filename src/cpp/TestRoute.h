#ifndef testroute_h
#define testroute_h

#include <string>
#include <unordered_map>
#include <vector>
#include "BasicStop.h"
#include "Route.h"
#include "RoutingPattern.h"

class AlgoInput;
class TestRoute : public Route {
    public:
        TestRoute(std::string id) : Route(std::move(id)) {}
        TestRoute(std::string id, std::string station,
            std::unordered_map<std::string, Stop> stops, date::sys_seconds dep,
            Rectangle r, TTMatrix ttmatrix) : Route(std::move(id),
            std::move(station), std::move(stops), std::move(dep), std::move(r),
            std::move(ttmatrix)) {}
        std::unordered_map<std::string, double> features(const AlgoInput& input)
                const;
};

#endif

