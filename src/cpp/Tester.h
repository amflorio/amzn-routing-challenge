#ifndef tester_h
#define tester_h

#include <string>
#include <unordered_map>
#include <vector>
#include "rapidjson/document.h"
#include "BasicRoute.h"
#include "Model.h"
#include "Sequence.h"
#include "TestRoute.h"

class Tester {
    private:
        std::unordered_map<std::string, TestRoute> routes;  // test data
        Model model;
        void loadPackageData(const rapidjson::Document& dom);
        void loadRouteData(const rapidjson::Document& dom);
        void loadTravelTimes(const rapidjson::Document& dom);
        bool validateRoute(const std::string& routeid) const;
    public:
        Tester(const std::string& packagedata, const std::string& routedata,
                const std::string& traveltimes);
        void readModel(const std::string& filename);
        void saveSequences(const std::string& filename) const;
        void test();
};

#endif

