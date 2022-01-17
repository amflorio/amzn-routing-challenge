#include <algorithm>
#include <cmath>
#include <cstdio>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <limits>
#include <random>
#include <unordered_set>
#include <boost/archive/text_oarchive.hpp>
#include <boost/functional/hash.hpp>
#include "AlgoInput.h"
#include "Algorithm.h"
#include "DatasetBuilder.h"
#include "JSONParser.h"
#include "LassoRegression.h"
#include "Learner.h"
#include "Statistics.h"
#include "Stopwatch.h"

using namespace std;
using namespace rapidjson;

Learner::Learner(const string& actualseqs, const string& invalidseqscrs,
        const string& packagedata, const string& routedata,
        const string& traveltimes) {
    cout<<"reading/processing actual sequences ..."<<endl;
    loadActualSequences(JSONParser::parse(actualseqs));
    cout<<"reading/processing invalid sequence scores ..."<<endl;
    loadInvalidSequenceScores(JSONParser::parse(invalidseqscrs));
    cout<<"reading/processing route data ..."<<endl;
    loadRouteData(JSONParser::parse(routedata));
    cout<<"reading/processing package data ..."<<endl;
    loadPackageData(JSONParser::parse(packagedata));
    cout<<"reading/processing travel times ..."<<endl;
    loadTravelTimes(JSONParser::parse(traveltimes));
    cout<<allroutes.size()<<" routes loaded"<<endl;
    cout<<"removing routes with incomplete data ..."<<endl;
    for (auto it=allroutes.begin(); it!=allroutes.end();)
        if (it->second.incomplete())
            it=allroutes.erase(it);
        else
            ++it;
    cout<<allroutes.size()<<" routes available for learning"<<endl;
    const string csvfile="data/model_build_outputs/allroutes.csv";
    cout<<"exporting route data to "<<csvfile<<endl;
    exportDataCSV(csvfile);
    const string texfile="data/model_build_outputs/zones.tex";
    cout<<"exporting zone locations to "<<texfile<<endl;
    exportZonesTex(texfile);
    printStatistics();
    printZones();
}

AlgoInput Learner::createAlgoInput(const unordered_map<string,
        TrainingRoute> routes, const vector<string>& minus) {
    // from vector to unordered_set for fast queries
    unordered_set<string> setminus(minus.begin(), minus.end());
    AlgoInput input;
    RoutingPattern patt;
    for (const auto& kv : routes) {
        if (setminus.count(kv.first)==1)
            continue;
        const auto& r=kv.second;
        input.addRoute(r);
        if (r.score()==TrainingRoute::Score::low)
            continue;
        auto stops=removeUnkwnownZones(r.sequence().stops(), r);
        typedef pair<string, string> strpair;
        unordered_set<strpair, boost::hash<strpair>> macro, micro, nano;
        for (size_t i=0; i<stops.size(); ++i) {
            const auto& a=r.getStop(i==0?stops.back():stops[i-1]);
            const auto& b=r.getStop(stops[i]);
            if (a.macroZone()!=b.macroZone()
                    && macro.count({a.macroZone(), b.macroZone()})==0) {
                patt.addMacro(r.station(), a.macroZone(), b.macroZone());
                if (r.score()==TrainingRoute::Score::high)
                    patt.addMacro(r.station(), a.macroZone(), b.macroZone());
                macro.insert({a.macroZone(), b.macroZone()});
            }
            if (a.microZone()!=b.microZone()
                    && micro.count({a.microZone(), b.microZone()})==0) {
                patt.addMicro(r.station(), a.microZone(), b.microZone());
                if (r.score()==TrainingRoute::Score::high)
                    patt.addMicro(r.station(), a.microZone(), b.microZone());
                micro.insert({a.microZone(), b.microZone()});
            }
            if (a.nanoZone()!=b.nanoZone()
                    && nano.count({a.nanoZone(), b.nanoZone()})==0) {
                patt.addNano(r.station(), a.nanoZone(), b.nanoZone());
                if (r.score()==TrainingRoute::Score::high)
                    patt.addNano(r.station(), a.nanoZone(), b.nanoZone());
                nano.insert({a.nanoZone(), b.nanoZone()});
            }
        }
        // entries and exits
        patt.addEntryMacro(r.getStop(stops[1]).macroZone());
        patt.addEntryMicro(r.getStop(stops[1]).microZone());
        patt.addEntryNano(r.getStop(stops[1]).nanoZone());
        patt.addExitMacro(r.getStop(stops.back()).macroZone());
        patt.addExitMicro(r.getStop(stops.back()).microZone());
        patt.addExitNano(r.getStop(stops.back()).nanoZone());
    }
    input.setPattern(move(patt));
    return input;
}

vector<pair<vector<string>, vector<int>>> Learner::createBatches(
        size_t nbatches, size_t thresh, double prop, double cv_ratio) const {
    auto trndata=DatasetBuilder::sampleHighScoreRoutes(allroutes, thresh, prop,
            cv_ratio);
    cout<<"splitting training data ("<<trndata.size()
            <<" high score routes) into "<<nbatches<<" batches ..."<<endl;
    random_device rd;
    mt19937 g(rd());
    shuffle(trndata.begin(), trndata.end(), g);
    vector<pair<vector<string>, vector<int>>> batches(nbatches);
    for (size_t i=0; i<trndata.size(); ++i) {
        batches[i%nbatches].first.push_back(trndata[i].first);
        batches[i%nbatches].second.push_back(trndata[i].second);
    }
    for (size_t i=0; i<nbatches; ++i) {
        const size_t cvroutes=accumulate(batches[i].second.begin(),
                batches[i].second.end(), 0, [](size_t a, int cv)
                {return a+(cv==1?1:0);});
        cout<<"batch "<<i+1<<": "<<batches[i].first.size()-cvroutes
            <<" training routes, "<<cvroutes<<" cross-validation routes"<<endl;
    }
    return batches;
}

void Learner::exportDataCSV(const string& csvfile) const {
    ofstream csv(csvfile);
    csv<<"routeID,station,dayOfWeek,score,durSeq,stops,packages,stopsWithTW,"
        "serviceTime,mainMacro,mainMicro,distinctMacro,distinctMicro,"
        "distinctNano,earliness,lateness,departure"<<endl;
    for (const auto& kv : allroutes) {
        const auto& r=kv.second;
        const string score=r.score()==TrainingRoute::Score::high?"1_high":
                r.score()==TrainingRoute::Score::medium?"2_medium":
                r.score()==TrainingRoute::Score::low?"3_low":"unknown";
        auto wd=date::weekday {date::floor<date::days>(r.departure())};
        auto withTW=count_if(r.stops().begin(), r.stops().end(),
                [](const pair<string, Stop>& p){return p.second.hasTW();});
        const auto& seq=r.sequence();
        using namespace date;
        csv<<r.id()<<","<<r.station()<<","<<wd<<","<<score<<","<<seq.duration()
            <<","<<r.stops().size()<<","<<r.numPackages()<<","<<withTW<<","
            <<r.serviceTime()<<","<<r.mainMacroZone()<<","<<r.mainMicroZone()
            <<","<<r.distinctMacroZones()<<","<<r.distinctMicroZones()<<","
            <<r.distinctNanoZones()<<","<<seq.earliness()<<","<<seq.lateness()
            <<","<<r.departure()<<endl;
    }
}

void Learner::exportFeatures(const string& path) const {    // TODO: rename
    cout<<"exporting features to CSV files ..."<<endl;
    typedef pair<string, string> transition;
    unordered_map<string, unordered_set<transition, boost::hash<transition>>>
            nanotrns, microtrns, macrotrns;
    for (const auto& kv : allroutes) {
        const auto& r=kv.second;
        const auto& seq=r.sequence();
        auto stops=removeUnkwnownZones(seq.stops(), r);
        for (size_t i=0; i<stops.size(); ++i) {
            const Stop& from=r.getStop(stops[i]);
            const Stop& to=r.getStop(i!=stops.size()-1 ? stops[i+1] : stops[0]);
            if (from.nanoZone()!=to.nanoZone())
               nanotrns[r.station()].insert({from.nanoZone(), to.nanoZone()});
            if (from.microZone()!=to.microZone())
               microtrns[r.station()].insert({from.microZone(),to.microZone()});
            if (from.macroZone()!=to.macroZone())
               macrotrns[r.station()].insert({from.macroZone(),to.macroZone()});
        }
    }
    for (const auto& kv : microtrns) {
        ofstream csv(path+kv.first+".csv");
        csv<<"routeID,station,dayOfWeek,durSeq,stops,packages,stopsWithTW,"
            "serviceTime,mainMacro,mainMicro,distinctMacro,distinctMicro,"
            "distinctNano,earliness,lateness,departure,";
        for (const auto& trn : kv.second)
            csv<<trn.first<<"->"<<trn.second<<",";
        csv<<"score"<<endl;
        for (const auto& kv2 : allroutes) {
            const auto& r=kv2.second;
            if (r.station()!=kv.first)
                continue;
            auto wd=date::weekday {date::floor<date::days>(r.departure())};
            auto withTW=count_if(r.stops().begin(), r.stops().end(),
                    [](const pair<string, Stop>& p){return p.second.hasTW();});
            const auto& seq=r.sequence();
            using namespace date;
            csv<<r.id()<<","<<r.station()<<","<<wd<<","<<seq.duration()<<","
                <<r.stops().size()<<","<<r.numPackages()<<","<<withTW<<","
                <<r.serviceTime()<<","<<r.mainMacroZone()<<","
                <<r.mainMicroZone()<<","<<r.distinctMacroZones()<<","
                <<r.distinctMicroZones()<<","<<r.distinctNanoZones()<<","
                <<","<<seq.earliness()<<","<<seq.lateness()<<","<<r.departure()
                <<",";
            auto stops=removeUnkwnownZones(seq.stops(), r);
            for (const auto& trn : kv.second)
                csv<<(hasMicroZoneTransition(stops, r, trn.first, trn.second)
                        ?"1,":"0,");
            const string score=r.score()==TrainingRoute::Score::high?"high":
                    r.score()==TrainingRoute::Score::medium?"medium":
                    r.score()==TrainingRoute::Score::low?"low":"unknown";
            csv<<score<<endl;
        }
        cout<<"done "<<kv.first<<endl;
    }
    cout<<"exporting algo models to CSV files ..."<<endl;
    const auto& algmodels=model.algorithmModels();
    for (const auto& kv : algmodels)
        kv.second.exportCSV(path+kv.first+".csv");
}

void Learner::exportZonesTex(const string& texfile) const {
    unordered_map<string, vector<tuple<double, double>>> zonestopcoords;
    unordered_map<string, tuple<double, double>> stations;
    for (const auto& kv : allroutes) {
        const auto& r=kv.second;
        if (r.station().find("DLA")!=string::npos)
            for (const auto& kv : r.stops()) {
                const Stop& s=kv.second;
                if (s.nanoZone()!="" && s.type()==Stop::Type::dropoff)
                    zonestopcoords[s.nanoZone()].push_back({s.lat(), s.lon()});
                else if (s.type()==Stop::Type::station)
                    stations[r.station()]={s.lat(), s.lon()};
            }
    }
    unordered_map<string, tuple<double, double, int>> zonecoords;
    for (const auto& z : zonestopcoords) {
        tuple<double, double, int> com {0,0,0};     // zone's "center of mass"
        for (const auto& coords : z.second) {
            get<0>(com)+=get<0>(coords);
            get<1>(com)+=get<1>(coords);
        }
        get<0>(com)/=z.second.size();
        get<1>(com)/=z.second.size();
        get<2>(com)=z.second.size();
        zonecoords[z.first]=com;
    }
    ofstream tex(texfile);
    tex<<setprecision(numeric_limits<double>::digits10);
    tex<<"\\PassOptionsToPackage{usenames,x11names}{xcolor}"<<endl;
    tex<<"\\documentclass[tikz]{standalone}"<<endl;
    tex<<"\\tikzset{>=latex}"<<endl;
    tex<<"\\begin{document}"<<endl;
    tex<<"\\begin{tikzpicture}"<<endl;
    auto color=[](const string& s){return s.find("DLA3")!=string::npos?"green":
            s.find("DLA4")!=string::npos?"purple":
            s.find("DLA5")!=string::npos?"blue":
            s.find("DLA7")!=string::npos?"red":
            s.find("DLA8")!=string::npos?"brown":
            s.find("DLA9")!=string::npos?"orange":"gray";};
    for (const auto& z : zonecoords) {
        auto xy=toXYCoords(get<0>(z.second), get<1>(z.second));
        tex<<"\\filldraw ["<<color(z.first)<<"] ("<<xy.first<<","<<xy.second
                <<") circle (1pt) node[below] {\\tiny "<<z.first<<" ("
                <<get<2>(z.second)<<")};"<<endl;
    }
    for (const auto& s : stations) {
        auto xy=toXYCoords(get<0>(s.second), get<1>(s.second));
        tex<<"\\filldraw ["<<color(s.first)<<"] ("<<xy.first<<","<<xy.second
                <<") circle (100pt);"<<endl;
    }
    tex<<"\\end{tikzpicture}"<<endl;
    tex<<"\\end{document}"<<endl;
}

bool Learner::hasMacroZoneTransition(const vector<string>& stops,
        const TrainingRoute& r, const string& from, const string& to) {
    for (size_t i=0; i<stops.size(); ++i) {
        const Stop& f=r.getStop(stops[i]);
        const Stop& t=r.getStop(i!=stops.size()-1 ? stops[i+1] : stops[0]);
        if (f.macroZone()==from && t.macroZone()==to)
            return true;
    }
    return false;
}

bool Learner::hasMicroZoneTransition(const vector<string>& stops,
        const TrainingRoute& r, const string& from, const string& to) {
    for (size_t i=0; i<stops.size(); ++i) {
        const Stop& f=r.getStop(stops[i]);
        const Stop& t=r.getStop(i!=stops.size()-1 ? stops[i+1] : stops[0]);
        if (f.microZone()==from && t.microZone()==to)
            return true;
    }
    return false;
}

bool Learner::hasNanoZoneTransition(const vector<string>& stops,
        const TrainingRoute& r, const string& from, const string& to) {
    for (size_t i=0; i<stops.size(); ++i) {
        const Stop& f=r.getStop(stops[i]);
        const Stop& t=r.getStop(i!=stops.size()-1 ? stops[i+1] : stops[0]);
        if (f.nanoZone()==from && t.nanoZone()==to)
            return true;
    }
    return false;
}

void Learner::learn(const string& modelfile) {
    model.setModelFile(modelfile);
    model.setAlgoInput(createAlgoInput(allroutes, {}));
    model.save();
    learnEvaluationModel();
}

void Learner::learnEvaluationModel() {
    // parameters
    const size_t nbatches=50;       // number of batches
    const size_t psize=1000;        // poolsize
    const size_t dps_per_route=10;  // number of data points to save per route
    cout<<"learning sequence evaluation model ..."<<endl;
    cout<<"nbatches="<<nbatches<<" ; psize="<<psize<<" ; dps_per_route="
            <<dps_per_route<<endl;
    auto batches=createBatches(nbatches, 1, 1, 0.175);
    auto alg=Algorithm::bestAlgorithm();
    Stopwatch sw;
    LassoRegression evlmodel;
    evlmodel.setMasterModel(&model);
    for (size_t b=0; b<batches.size(); ++b) {
        cout<<"batch: "<<b+1<<"/"<<batches.size()<<endl;
        vector<TestRoute> routes;
        for (const auto& id : batches[b].first)
            routes.push_back(allroutes.at(id).toTestRoute());
        cout<<routes.size()<<" test/CV routes (elapsed: "<<sw.elapsedSeconds()
                <<" s)"<<endl;
        const auto algoI=createAlgoInput(allroutes, batches[b].first);
        size_t nroute=0;
        for (size_t ridx=0; ridx<routes.size(); ++ridx) {
            const auto& r=routes[ridx];
            auto seqs=alg->findSequences(psize, r, algoI);
            for (auto& seq : seqs)
                r.setupSimilarity(seq, algoI.pattern());
            sort(seqs.begin(), seqs.end(), Algorithm::better);
            auto stats=Sequence::statistics(seqs);
            for (size_t i=0; i<seqs.size()&&i<dps_per_route; ++i) {
                double sc=allroutes.at(r.id()).computeScore(seqs[i]);
                bool cv=batches[b].second[ridx]==1;     // cross validation?
                evlmodel.addDataPoint(seqs[i].features(r, stats), sc, cv);
            }
            cout<<++nroute<<"/"<<routes.size()<<" routes done"<<endl;
        }
        cout<<evlmodel.numTrainingDataPoints()<<" training DPs and "
                <<evlmodel.numCVDataPoints()
                <<" cross-validation DPs in evaluation model"<<endl;
    }
    evlmodel.solve(1);
    evlmodel.clearData();
    model.setEvaluationModel(move(evlmodel));
}

void Learner::loadActualSequences(const Document& dom) {
    for (const auto& seq : dom.GetObject()) {
        // some minimal protection against inconsistent input data
        if (allroutes.count(seq.name.GetString())==1) {
            cout<<"warning: duplicated route id, skipping record"<<endl;
            continue;
        }
        const auto& seqstops=seq.value;
        if (!seqstops.HasMember("actual")) {
            cout<<"warning: not an \"actual\" sequence, skipping record"<<endl;
            continue;
        }
        TrainingRoute rt(seq.name.GetString());
        unordered_map<string, size_t> stopseq;
        for (const auto& stop : seqstops["actual"].GetObject())
            stopseq.emplace(stop.name.GetString(), stop.value.GetInt());
        if (!rt.setStops(move(stopseq))) {
            cout<<"warning: could not set route stops"<<endl;
            continue;
        }
        allroutes.insert({rt.id(), rt});
    }
}

void Learner::loadInvalidSequenceScores(const Document& dom) {
    for (const auto& route : dom.GetObject()) {
        if (!validateRoute(route.name.GetString()))
            continue;
        TrainingRoute& rt=allroutes.at(route.name.GetString());
        rt.setCostInvalid(route.value.GetDouble());
    }
}

void Learner::loadPackageData(const Document& dom) {
    for (const auto& route : dom.GetObject()) {
        if (!validateRoute(route.name.GetString()))
            continue;
        TrainingRoute& rt=allroutes.at(route.name.GetString());
        for (const auto& stop : route.value.GetObject()) {
            if (!rt.hasStop(stop.name.GetString())) {
                cout<<"warning: stop not found in route"<<endl;
                rt.setIncomplete();
                goto force_next_route;      // or break
            }
            Stop& st=rt.getStop(stop.name.GetString());
            for (const auto& pack : stop.value.GetObject()) {
                const auto& packinfo=pack.value;
                if (!packinfo.HasMember("scan_status")
                        || !packinfo.HasMember("time_window")
                        || !packinfo.HasMember("planned_service_time_seconds")){
                    cout<<"warning: missing key in package info"<<endl;
                    rt.setIncomplete();
                    goto force_next_route;
                }
                Package p(pack.name.GetString(),
                        packinfo["scan_status"].GetString());
                const auto& tw=packinfo["time_window"];
                if (!tw.HasMember("start_time_utc")
                        || !tw.HasMember("end_time_utc")) {
                    cout<<"warning: missing time window info"<<endl;
                    rt.setIncomplete();
                    goto force_next_route;      // here we can't break
                }
                if (tw["start_time_utc"].IsString()
                        && tw["end_time_utc"].IsString()) {
                    p.setTimeWindow(tw["start_time_utc"].GetString(),
                            tw["end_time_utc"].GetString());
                }
                p.setServiceTime(packinfo["planned_service_time_seconds"]
                        .GetDouble());
                st.addPackage(move(p));
            }
        }
force_next_route:
        ;
    }
}

void Learner::loadRouteData(const Document& dom) {
    for (const auto& route : dom.GetObject()) {
        if (!validateRoute(route.name.GetString()))
            continue;
        TrainingRoute& rt=allroutes.at(route.name.GetString());
        const auto& routeinfo=route.value;
        if (!routeinfo.HasMember("station_code")
                || !routeinfo.HasMember("date_YYYY_MM_DD")
                || !routeinfo.HasMember("departure_time_utc")
                || !routeinfo.HasMember("route_score")
                || !routeinfo.HasMember("stops")) {
            cout<<"warning: missing key in route info"<<endl;
            rt.setIncomplete();
            continue;
        }
        if (!routeinfo["station_code"].IsString()
                || !routeinfo["date_YYYY_MM_DD"].IsString()
                || !routeinfo["departure_time_utc"].IsString()
                || !routeinfo["route_score"].IsString()
                || !routeinfo["stops"].IsObject()) {
            cout<<"warning: invalid value type in route info"<<endl;
            rt.setIncomplete();
            continue;
        }
        rt.setStation(routeinfo["station_code"].GetString());
        rt.setDeparture(routeinfo["date_YYYY_MM_DD"].GetString()+string(" ")
                +routeinfo["departure_time_utc"].GetString());
        if (!rt.setScore(routeinfo["route_score"].GetString())) {
            cout<<"warning: could not set route score"<<endl;
            rt.setIncomplete();
            continue;
        }
        for (const auto& stop : routeinfo["stops"].GetObject()) {
            if (!rt.hasStop(stop.name.GetString())) {
                cout<<"warning: stop not found in route"<<endl;
                rt.setIncomplete();
                goto force_next_route;
            }
            Stop& st=rt.getStop(stop.name.GetString());
            const auto& stopinfo=stop.value;
            if (!st.setType(stopinfo["type"].GetString())) {
                cout<<"warning: could not set stop type"<<endl;
                rt.setIncomplete();
                goto force_next_route;
            }
            if (stopinfo["zone_id"].IsString())
                st.setNanoZone(rt.station()+":"
                        +stopinfo["zone_id"].GetString());
            if (stopinfo["lat"].IsDouble() && stopinfo["lng"].IsDouble())
                st.setLatLon(stopinfo["lat"].GetDouble(),
                        stopinfo["lng"].GetDouble());
            else {
                cout<<"warning: could not set lat/lon coordinates"<<endl;
                rt.setIncomplete();
                goto force_next_route;
            }
        }
        rt.setupRectangle();
        if (rt.getStop(rt.sequence().stops()[0]).type()!=Stop::Type::station) {
            cout<<"warning: sequence does not begin at a station"<<endl;
            rt.setIncomplete();
        }
force_next_route:
        ;
    }
}

void Learner::loadTravelTimes(const Document& dom) {
    int counter=0;      // to indicate progress
    for (const auto& route : dom.GetObject()) {
        if (!validateRoute(route.name.GetString()))
            continue;
        TrainingRoute& rt=allroutes.at(route.name.GetString());
        TTMatrix ttimes(rt.stops().size());
        for (const auto& from : route.value.GetObject()) {
            if (!rt.hasStop(from.name.GetString())) {
                cout<<"warning: stop does not belong to route"<<endl;
                rt.setIncomplete();
                goto force_next_route;
            }
            auto fromid=from.name.GetString();
            for (const auto& to : from.value.GetObject())
                ttimes.setTravelTime(fromid, to.name.GetString(),
                        to.value.GetDouble());
        }
        if (ttimes.consistent()) {
            rt.setTravelTimes(move(ttimes));
            rt.setupTiming(rt.sequence());
            //rt.setupFastDuration();
        } else {
            cout<<"warning: travel time matrix inconsistent"<<endl;
            rt.setIncomplete();
        }
force_next_route:
        if (++counter%100==0)
            cout<<"\r"<<counter*100/allroutes.size()<<"\%"<<flush;
    }
    cout<<"\r100\%"<<endl;
}

void Learner::printPartial(map<string, map<string, vector<double>>>& scores)
        const {
    cout<<"average best scores per station per algorithm (partial):"<<endl;
    for (auto& kv1 : scores) {
        cout<<"station: "<<kv1.first<<endl;
        for (auto& kv2 : kv1.second) {
            double avg=accumulate(kv2.second.begin(), kv2.second.end(), 0.0)
                    /kv2.second.size();
            cout<<"\t"<<kv2.first<<": "<<avg<<endl;
            kv2.second={avg};       // saves up some space
        }
    }
}

void Learner::printStatistics() const {
    int high=0, medium=0, low=0;
    vector<double> seq_durations;
    vector<int> stops_per_route;
    vector<int> packages_per_stop;
    vector<double> service_times;
    int delivered=0, attempted=0, rejected=0, punknown=0;
    int withTW=0;
    vector<double> durTWs;
    for (const auto& kv : allroutes) {
        const auto& r=kv.second;
        if (r.score()==TrainingRoute::Score::high)
            high++;
        else if (r.score()==TrainingRoute::Score::medium)
            medium++;
        else if (r.score()==TrainingRoute::Score::low)
            low++;
        seq_durations.push_back(r.sequence().duration());
        stops_per_route.push_back(r.stops().size());
        for (const auto& kv : r.stops()) {
            const auto& s=kv.second;
            packages_per_stop.push_back(s.packages().size());
            for (const auto& p : s.packages()) {
                if (p.status()==Package::Status::delivered)
                    delivered++;
                else if (p.status()==Package::Status::attempted)
                    attempted++;
                else if (p.status()==Package::Status::rejected)
                    rejected++;
                else
                    punknown++;
                if (p.hasTW()) {
                    withTW++;
                    durTWs.push_back(chrono::duration_cast<chrono::seconds>
                            (p.endTW()-p.startTW()).count());
                }
                service_times.push_back(p.serviceTime());
            }
        }
    }
    cout<<"statistics on number of stops per route:"<<endl;
    Statistics::basicStats<int>(stops_per_route);
    cout<<"statistics on number of packages per stop:"<<endl;
    Statistics::basicStats<int>(packages_per_stop);
    cout<<"statistics on scan status of packages:"<<endl;
    int tot=delivered+attempted+rejected+punknown;
    cout<<"delivered: "<<delivered<<"  ("<<1.0*delivered/tot<<")"<<endl;
    cout<<"attempted: "<<attempted<<"  ("<<1.0*attempted/tot<<")"<<endl;
    cout<<"rejected: "<<rejected<<"  ("<<1.0*rejected/tot<<")"<<endl;
    cout<<"unknown: "<<punknown<<"  ("<<1.0*punknown/tot<<")"<<endl;
    cout<<"statistics on time windows:"<<endl;
    cout<<"packages with time windows: "<<withTW<<"  ("
            <<1.0*withTW/(delivered+attempted+rejected)<<")"<<endl;
    cout<<"durations of time windows:"<<endl;
    Statistics::basicStats<double>(durTWs);
    cout<<"statistics on service times:"<<endl;
    Statistics::basicStats<double>(service_times);
    cout<<"statistics on route scores:"<<endl;
    tot=high+medium+low;
    cout<<"high: "<<high<<"  ("<<1.0*high/tot<<")"<<endl;
    cout<<"medium: "<<medium<<"  ("<<1.0*medium/tot<<")"<<endl;
    cout<<"low: "<<low<<"  ("<<1.0*low/tot<<")"<<endl;
    cout<<"statistics on actual sequence durations (no service time):"<<endl;
    Statistics::basicStats<double>(seq_durations);
}

void Learner::printZones() const {
    unordered_map<string, unordered_map<string, unordered_map<string,
            unordered_set<string>>>> zones;   // station, macro, micro and nano
    for (const auto& kv : allroutes) {
        const auto& r=kv.second;
        if (zones.count(r.station())==0)
            zones[r.station()];     // default constructor
        for (const auto& kv : r.stops()) {
            const auto& s=kv.second;
            if (zones[r.station()].count(s.macroZone())==0)
                zones[r.station()][s.macroZone()];  // default constructor
            if (zones[r.station()][s.macroZone()].count(s.microZone())==0)
                zones[r.station()][s.macroZone()][s.microZone()];   // dflt
            zones[r.station()][s.macroZone()][s.microZone()].insert(
                    s.nanoZone());
        }
    }
    for (const auto& station : zones) {
        cout<<"station: "<<station.first<<endl;
        cout<<"   macro zones: "<<station.second.size()<<endl;
        int nmicro=0, nnano=0;
        for (const auto& macro : station.second) {
            nmicro+=macro.second.size();
            for (const auto& micro : macro.second)
                nnano+=micro.second.size();
        }
        cout<<"   micro zones: "<<nmicro<<endl;
        cout<<"   nano zones: "<<nnano<<endl;
    }
}

vector<string> Learner::removeUnkwnownZones(vector<string> stops,
        const Route& r) {       // TODO: move function to another module
    // remove dropoff stops with unknown zones
    stops.erase(remove_if(stops.begin(), stops.end(), [&r](const string& s)
            {return r.getStop(s).type()==Stop::Type::dropoff
                 && r.getStop(s).nanoZone()=="";}), stops.end());
    return stops;
}

pair<double, double> Learner::toXYCoords(double lat, double lon) const {
    // TODO: this currently works only for LA area as parameters are hardcoded
    const double pi=3.14159265359;
    const double lat0=(34.052235/180)*pi;   // Los Angeles' latitude
    const double offsetx=97.8;
    const double offsety=-34;
    const double scaling=300;
    return {(lon*cos(lat0)+offsetx)*scaling, (lat+offsety)*scaling};
}

bool Learner::validateRoute(const string& routeid) const {
    if (allroutes.count(routeid)==0) {
        cout<<"warning: route id not found"<<endl;
        return false;
    }
    if (allroutes.at(routeid).incomplete()) {
        cout<<"warning: route with incomplete data"<<endl;
        return false;
    }
    return true;
}

