#ifndef datasetbuilder_h
#define datasetbuilder_h

#include <string>
#include <unordered_map>
#include "rapidjson/document.h"
#include "TrainingRoute.h"

class DatasetBuilder {
    private:
        rapidjson::Document actualseqsdom, invalidseqscrsdom, packagedatadom,
                routedatadom, traveltimesdom, newactualseqsdom,
                newinvalidseqscrsdom, newpackagedatadom, newroutedatadom,
                newtraveltimesdom;
        std::vector<TrainingRoute> highScoreRoutes() const;
        static std::vector<std::string> sampleRoutes(
                const std::vector<TrainingRoute>& routes, size_t thresh,
                double prop);
        static std::vector<std::pair<std::string, int>> sampleRoutes(
                const std::vector<TrainingRoute>& routes, size_t thresh,
                double prop, double cv_ratio);
        void updateActualSeqs(const std::vector<std::string>& tomove);
        void updateInvalidSeqScores(const std::vector<std::string>& tomove);
        void updatePackageData(const std::vector<std::string>& tomove);
        void updateRouteData(const std::vector<std::string>& tomove);
        void updateTravelTimes(const std::vector<std::string>& tomove);
    public:
        DatasetBuilder(const std::string& actualseqs,
            const std::string& invalidseqscrs, const std::string& packagedata,
            const std::string& routedata, const std::string& traveltimes,
            const std::string& newactualseqs,
            const std::string& newinvalidseqscrs,
            const std::string& newpackagedata, const std::string& newroutedata,
            const std::string& newtraveltimes);
        void convertToValidation();
        static std::vector<std::string> sampleHighScoreRoutes(
                const std::unordered_map<std::string, TrainingRoute>& routes,
                size_t thresh, double prop);
        static std::vector<std::pair<std::string, int>> sampleHighScoreRoutes(
                const std::unordered_map<std::string, TrainingRoute>& routes,
                size_t thresh, double prop, double cv_ratio);
        static std::vector<std::string> sampleMediumOrHighScoreRoutes(
                const std::unordered_map<std::string, TrainingRoute>& routes,
                size_t thresh, double prop);
        void saveDataset(const std::string& actualseqs,
            const std::string& invalidseqscrs, const std::string& packagedata,
            const std::string& routedata, const std::string& traveltimes,
            const std::string& newactualseqs,
            const std::string& newinvalidseqscrs,
            const std::string& newpackagedata, const std::string& newroutedata,
            const std::string& newtraveltimes) const;
};

#endif

