#include <algorithm>
#include <iostream>
#include <random>
#include "DatasetBuilder.h"
#include "JSONParser.h"

using namespace std;
using namespace rapidjson;

DatasetBuilder::DatasetBuilder(const string& actualseqs,
        const string& invalidseqscrs, const string& packagedata,
        const string& routedata, const string& traveltimes,
        const string& newactualseqs, const string& newinvalidseqscrs,
        const string& newpackagedata, const string& newroutedata,
        const string& newtraveltimes) {
    cout<<"loading original dataset ..."<<endl;
    actualseqsdom=JSONParser::parse(actualseqs);
    invalidseqscrsdom=JSONParser::parse(invalidseqscrs);
    routedatadom=JSONParser::parse(routedata);
    packagedatadom=JSONParser::parse(packagedata);
    traveltimesdom=JSONParser::parse(traveltimes);
    /*
    // load existing validation data
    newactualseqsdom=JSONParser::parse(newactualseqs);
    newinvalidseqscrsdom=JSONParser::parse(newinvalidseqscrs);
    newroutedatadom=JSONParser::parse(newroutedata);
    newpackagedatadom=JSONParser::parse(newpackagedata);
    newtraveltimesdom=JSONParser::parse(newtraveltimes);
    */
    // this will overwrite existing validation data
    newactualseqsdom.Parse("{}");
    newinvalidseqscrsdom.Parse("{}");
    newroutedatadom.Parse("{}");
    newpackagedatadom.Parse("{}");
    newtraveltimesdom.Parse("{}");
}

void DatasetBuilder::convertToValidation() {
    // select initially all high score routes
    auto routes=highScoreRoutes();
    cout<<routes.size()<<" high scores routes"<<endl;
    // stratified route sampling based on macro zones
    auto tomove=sampleRoutes(routes, 3, 0.10);
    cout<<tomove.size()<<" routes sampled to move to validation set"<<endl;
    // update DOMs accordingly
    updateActualSeqs(tomove);
    updateInvalidSeqScores(tomove);
    updateRouteData(tomove);
    updatePackageData(tomove);
    updateTravelTimes(tomove);
}

vector<TrainingRoute> DatasetBuilder::highScoreRoutes() const {
    vector<TrainingRoute> routes;
    for (const auto& route : routedatadom.GetObject()) {
        TrainingRoute rt(route.name.GetString());
        const auto& routeinfo=route.value;
        rt.setScore(routeinfo["route_score"].GetString());
        if (rt.score()!=TrainingRoute::Score::high)
            continue;
        rt.setStation(routeinfo["station_code"].GetString());
        for (const auto& stop : routeinfo["stops"].GetObject()) {
            rt.addStop(stop.name.GetString());
            Stop& st=rt.getStop(stop.name.GetString());
            const auto& stopinfo=stop.value;
            st.setType(stopinfo["type"].GetString());
            if (stopinfo["zone_id"].IsString())
                st.setNanoZone(rt.station()+":"
                        +stopinfo["zone_id"].GetString());
        }
        routes.push_back(move(rt));
    }
    return routes;
}

vector<string> DatasetBuilder::sampleHighScoreRoutes(
        const unordered_map<string, TrainingRoute>& routes, size_t thresh,
        double prop) {
    vector<TrainingRoute> high;
    for (const auto& kv : routes)
        if (kv.second.score()==TrainingRoute::Score::high)
            high.push_back(kv.second);
    return sampleRoutes(high, thresh, prop);
}

vector<pair<string, int>> DatasetBuilder::sampleHighScoreRoutes(
        const unordered_map<string, TrainingRoute>& routes, size_t thresh,
        double prop, double cv_ratio) {
    vector<TrainingRoute> high;
    for (const auto& kv : routes)
        if (kv.second.score()==TrainingRoute::Score::high)
            high.push_back(kv.second);
    return sampleRoutes(high, thresh, prop, cv_ratio);
}

vector<string> DatasetBuilder::sampleRoutes(
        const vector<TrainingRoute>& routes, size_t thresh, double prop) {
    // organize routes by macro zones
    unordered_map<string, vector<string>> map;
    for (const auto& r : routes)
        map[r.mainMacroZone()].push_back(r.id());
    // shuffle all route ids within each macro zone bucket
    random_device rd;
    mt19937 g(rd());
    for (auto& kv : map)
        shuffle(kv.second.begin(), kv.second.end(), g);
    unordered_map<string, vector<string>> tomove;
    vector<string> sampled_ids;
    for (const auto& kv : map)
        if (kv.second.size()>=thresh) {
            size_t n=max(1ul, (size_t)(prop*kv.second.size()+0.5));
            for (size_t i=0; i<n; ++i) {
                tomove[kv.first].push_back(kv.second[i]);
                sampled_ids.push_back(kv.second[i]);
            }
        }
    for (const auto& kv : map)
        cout<<kv.first<<": "<<tomove[kv.first].size()<<"/"<<kv.second.size()
                <<endl;
    return sampled_ids;
}

vector<pair<string, int>> DatasetBuilder::sampleRoutes(
        const vector<TrainingRoute>& routes, size_t thresh, double prop,
        double cv_ratio) {
    // organize routes by macro zones
    unordered_map<string, vector<string>> map;
    for (const auto& r : routes)
        map[r.mainMacroZone()].push_back(r.id());
    // shuffle all route ids within each macro zone bucket
    random_device rd;
    mt19937 g(rd());
    for (auto& kv : map)
        shuffle(kv.second.begin(), kv.second.end(), g);
    unordered_map<string, vector<string>> tomove;
    vector<pair<string, int>> sampled_ids;
    const size_t cv_int=1/cv_ratio+0.5;
    cout<<"reserving 1 every "<<cv_int<<" routes to cross-validation"<<endl;
    size_t k=0;
    for (const auto& kv : map)
        if (kv.second.size()>=thresh) {
            const size_t n=max(1ul, (size_t)(prop*kv.second.size()+0.5));
            for (size_t i=0; i<n&&i<kv.second.size(); ++i) {
                tomove[kv.first].push_back(kv.second[i]);
                sampled_ids.push_back({kv.second[i], k++%cv_int==0?1:0});
            }
        }
    for (const auto& kv : map)
        cout<<kv.first<<": "<<tomove[kv.first].size()<<"/"<<kv.second.size()
                <<endl;
    return sampled_ids;
}

void DatasetBuilder::saveDataset(const string& actualseqs,
        const string& invalidseqscrs, const string& packagedata,
        const string& routedata, const string& traveltimes,
        const string& newactualseqs, const string& newinvalidseqscrs,
        const string& newpackagedata, const string& newroutedata,
        const string& newtraveltimes) const {
    cout<<"saving development dataset ..."<<endl;
    JSONParser::save(actualseqsdom, actualseqs);
    JSONParser::save(invalidseqscrsdom, invalidseqscrs);
    JSONParser::save(routedatadom, routedata);
    JSONParser::save(packagedatadom, packagedata);
    JSONParser::save(traveltimesdom, traveltimes);
    JSONParser::save(newactualseqsdom, newactualseqs);
    JSONParser::save(newinvalidseqscrsdom, newinvalidseqscrs);
    JSONParser::save(newroutedatadom, newroutedata);
    JSONParser::save(newpackagedatadom, newpackagedata);
    JSONParser::save(newtraveltimesdom, newtraveltimes);
}

void DatasetBuilder::updateActualSeqs(const vector<string>& tomove) {
    for (const auto& id : tomove) {
        Value v(id.c_str(), newactualseqsdom.GetAllocator());
        newactualseqsdom.AddMember(v.Move(), actualseqsdom[id.c_str()],
                newactualseqsdom.GetAllocator());
        actualseqsdom.RemoveMember(id.c_str());
    }
}

void DatasetBuilder::updateInvalidSeqScores(const vector<string>& tomove) {
    for (const auto& id : tomove) {
        Value v(id.c_str(), newinvalidseqscrsdom.GetAllocator());
        newinvalidseqscrsdom.AddMember(v.Move(), invalidseqscrsdom[id.c_str()],
                newinvalidseqscrsdom.GetAllocator());
        invalidseqscrsdom.RemoveMember(id.c_str());
    }
}

void DatasetBuilder::updatePackageData(const vector<string>& tomove) {
    // first we remove 'scan_status' from the (originally) training data
    for (const auto& id : tomove) {
        auto& r=packagedatadom[id.c_str()];
        for (auto it_stop=r.MemberBegin(); it_stop!=r.MemberEnd(); ++it_stop) {
            auto& packs=it_stop->value;
            for (auto it_p=packs.MemberBegin(); it_p!=packs.MemberEnd(); ++it_p)
                it_p->value.RemoveMember("scan_status");
        }
    }
    // decided not to remove duplicated packages
    // now we just move as usual
    for (const auto& id : tomove) {
        Value v(id.c_str(), newpackagedatadom.GetAllocator());
        newpackagedatadom.AddMember(v.Move(), packagedatadom[id.c_str()],
                newpackagedatadom.GetAllocator());
        packagedatadom.RemoveMember(id.c_str());
    }
}

void DatasetBuilder::updateRouteData(const vector<string>& tomove) {
    // first we remove 'route_score' from the (originally) training data
    for (const auto& id : tomove)
        routedatadom[id.c_str()].RemoveMember("route_score");
    // now we just move as usual
    for (const auto& id : tomove) {
        Value v(id.c_str(), newroutedatadom.GetAllocator());
        newroutedatadom.AddMember(v.Move(), routedatadom[id.c_str()],
                newroutedatadom.GetAllocator());
        routedatadom.RemoveMember(id.c_str());
    }
}

void DatasetBuilder::updateTravelTimes(const vector<string>& tomove) {
    for (const auto& id : tomove) {
        Value v(id.c_str(), newtraveltimesdom.GetAllocator());
        newtraveltimesdom.AddMember(v.Move(), traveltimesdom[id.c_str()],
                newtraveltimesdom.GetAllocator());
        traveltimesdom.RemoveMember(id.c_str());
    }
}

