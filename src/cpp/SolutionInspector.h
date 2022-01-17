#ifndef solutioninspector_h
#define solutioninspector_h

#include <string>
#include <unordered_map>
#include "rapidjson/document.h"
#include "Model.h"
#include "Sequence.h"
#include "TestRoute.h"

class SolutionInspector {
    private:
        rapidjson::Document packagedatadom, routedatadom, traveltimesdom,
                propseqsdom, newactualseqsdom, scoresdom;
        std::unordered_map<std::string, TestRoute> routes;
        std::unordered_map<std::string, Sequence> actualseqs;
        Model model;
        void loadRoutes();
        void loadSequences();
        void statsPerStation() const;
    public:
        SolutionInspector(const std::string& packagedata,
                const std::string& routedata, const std::string& traveltimes,
                const std::string& propseqs, const std::string& newactualseqs,
                const std::string& scores);
        void inspect() const;
        void inspectCSV() const;
        void readModel(const std::string& filename);
};

#endif

