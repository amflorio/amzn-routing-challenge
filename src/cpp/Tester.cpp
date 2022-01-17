#include <algorithm>
#include <fstream>
#include <iostream>
#include <boost/archive/text_iarchive.hpp>
#include <boost/serialization/string.hpp>
#include "Algorithm.h"
#include "EntryExit.h"
#include "JSONParser.h"
#include "Sequence.h"
#include "Tester.h"

using namespace std;
using namespace rapidjson;

Tester::Tester(const string& packagedata, const string& routedata,
        const string& traveltimes) {
    cout<<"reading/processing route data ..."<<endl;
    loadRouteData(JSONParser::parse(routedata));
    cout<<"reading/processing package data ..."<<endl;
    loadPackageData(JSONParser::parse(packagedata));
    cout<<"reading/processing travel times ..."<<endl;
    loadTravelTimes(JSONParser::parse(traveltimes));
    cout<<routes.size()<<" routes loaded"<<endl;
    cout<<"removing routes with incomplete data ..."<<endl;
    for (auto it=routes.begin(); it!=routes.end();)
        if (it->second.incomplete())
            it=routes.erase(it);
        else
            ++it;
    cout<<routes.size()<<" routes for testing"<<endl;
}

void Tester::loadPackageData(const Document& dom) {
    for (const auto& route : dom.GetObject()) {
        if (!validateRoute(route.name.GetString()))
            continue;
        TestRoute& rt=routes.at(route.name.GetString());
        for (const auto& stop : route.value.GetObject()) {
            if (!rt.hasStop(stop.name.GetString())) {
                cout<<"warning: stop not found in route"<<endl;
                rt.setIncomplete();
                goto force_next_route;      // or break
            }
            Stop& st=rt.getStop(stop.name.GetString());
            for (const auto& pack : stop.value.GetObject()) {
                const auto& packinfo=pack.value;
                if (!packinfo.HasMember("time_window")
                        || !packinfo.HasMember("planned_service_time_seconds")){
                    cout<<"warning: missing key in package info"<<endl;
                    rt.setIncomplete();
                    goto force_next_route;
                }
                Package p(pack.name.GetString(), "UNDEFINED");
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

void Tester::loadRouteData(const Document& dom) {
    for (const auto& route : dom.GetObject()) {
        TestRoute rt(route.name.GetString());
        const auto& routeinfo=route.value;
        if (!routeinfo.HasMember("station_code")
                || !routeinfo.HasMember("date_YYYY_MM_DD")
                || !routeinfo.HasMember("departure_time_utc")
                || !routeinfo.HasMember("stops")) {
            cout<<"warning: missing key in route info"<<endl;
            rt.setIncomplete();
            continue;
        }
        if (!routeinfo["station_code"].IsString()
                || !routeinfo["date_YYYY_MM_DD"].IsString()
                || !routeinfo["departure_time_utc"].IsString()
                || !routeinfo["stops"].IsObject()) {
            cout<<"warning: invalid value type in route info"<<endl;
            rt.setIncomplete();
            continue;
        }
        rt.setStation(routeinfo["station_code"].GetString());
        rt.setDeparture(routeinfo["date_YYYY_MM_DD"].GetString()+string(" ")
                +routeinfo["departure_time_utc"].GetString());
        for (const auto& stop : routeinfo["stops"].GetObject()) {
            rt.addStop(stop.name.GetString());
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
        routes.insert({rt.id(), rt});
force_next_route:
        ;
    }
}

void Tester::loadTravelTimes(const Document& dom) {
    int counter=0;      // to indicate progress
    for (const auto& route : dom.GetObject()) {
        if (!validateRoute(route.name.GetString()))
            continue;
        TestRoute& rt=routes.at(route.name.GetString());
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
        if (ttimes.consistent())
            rt.setTravelTimes(move(ttimes));
        else {
            cout<<"warning: travel time matrix inconsistent"<<endl;
            rt.setIncomplete();
        }
force_next_route:
        if (++counter%100==0)
            cout<<"\r"<<counter*100/routes.size()<<"\%"<<flush;
    }
    cout<<"\r100\%"<<endl;
}

void Tester::readModel(const string& filename) {
    ifstream is(filename);
    boost::archive::text_iarchive ia(is);
    ia>>model;
}

void Tester::saveSequences(const string& filename) const {
    ofstream os(filename);
    os<<"{"<<endl;
    size_t cnt=0;   // counter to determine the last route (as it has no comma)
    for (const auto& kv : routes) {
        cnt++;
        os<<"  \""<<kv.first<<"\": {"<<endl;
        os<<"    \"proposed\": {"<<endl;
        kv.second.sequence().exportJSON(os);
        os<<"    }"<<endl;
        os<<"  }"<<(cnt!=routes.size()?",":"")<<endl;
    }
    os<<"}"<<endl;
}

void Tester::test() {
    auto alg=Algorithm::bestAlgorithm();
    for (auto& kv : routes) {
        auto& r=kv.second;
        r.setSequence(Algorithm::selectSequence(r, *alg, model));
    }
}

bool Tester::validateRoute(const string& routeid) const {
    if (routes.count(routeid)==0) {
        cout<<"warning: route id not found"<<endl;
        return false;
    }
    if (routes.at(routeid).incomplete()) {
        cout<<"warning: route with incomplete data"<<endl;
        return false;
    }
    return true;
}

