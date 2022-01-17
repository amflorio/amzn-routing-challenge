#ifndef learner_h
#define learner_h

#include <string>
#include <unordered_map>
#include "rapidjson/document.h"
#include "AlgoInput.h"
#include "Model.h"
#include "TrainingRoute.h"

class Learner {
    private:
        std::unordered_map<std::string, TrainingRoute> allroutes;
        Model model;
        static AlgoInput createAlgoInput(
                const std::unordered_map<std::string, TrainingRoute> routes,
                const std::vector<std::string>& minus);
        std::vector<std::pair<std::vector<std::string>, std::vector<int>>>
                createBatches(size_t nbatches, size_t thresh, double prop,
                double cv_ratio) const;
        void exportDataCSV(const std::string& csvfile) const;
        void exportZonesTex(const std::string& texfile) const;
        std::unordered_map<std::string, double> extractFeatures(
                const TestRoute& r, const AlgoInput& input) const;
        static bool hasMacroZoneTransition(const std::vector<std::string>& stps,
                const TrainingRoute& r, const std::string& from,
                const std::string& to);
        static bool hasMicroZoneTransition(const std::vector<std::string>& stps,
                const TrainingRoute& r, const std::string& from,
                const std::string& to);
        static bool hasNanoZoneTransition(const std::vector<std::string>& stps,
                const TrainingRoute& r, const std::string& from,
                const std::string& to);
        void learnEvaluationModel();
        void learnAlgorithmModels();
        void loadActualSequences(const rapidjson::Document& dom);
        void loadInvalidSequenceScores(const rapidjson::Document& dom);
        void loadPackageData(const rapidjson::Document& dom);
        void loadRouteData(const rapidjson::Document& dom);
        void loadTravelTimes(const rapidjson::Document& dom);
        void printPartial(std::map<std::string, std::map<std::string,
                std::vector<double>>>& allscores) const;
        void printStatistics() const;
        void printZones() const;
        std::pair<double, double> toXYCoords(double lat, double lon) const;
        bool validateRoute(const std::string& routeid) const;
    public:
        Learner(const std::string& actualseqs,
                const std::string& invalidseqscrs,
                const std::string& packagedata, const std::string& routedata,
                const std::string& traveltimes);
        void exportFeatures(const std::string& path) const;
        void learn(const std::string& modelfile);
        static std::vector<std::string> removeUnkwnownZones(
                std::vector<std::string> stops, const Route& r);
};

#endif

