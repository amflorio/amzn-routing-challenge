#include <algorithm>
#include <iomanip>
#include <iostream>
#include <boost/archive/text_iarchive.hpp>
#include <boost/serialization/string.hpp>
#include "date/date.h"
#include "JSONParser.h"
#include "SolutionInspector.h"

using namespace std;

SolutionInspector::SolutionInspector(const string& packagedata,
        const string& routedata, const string& traveltimes,
        const string& propseqs, const string& newactualseqs,
        const string& scores) {
    cout<<"loading data ..."<<endl;
    packagedatadom=JSONParser::parse(packagedata);
    routedatadom=JSONParser::parse(routedata);
    traveltimesdom=JSONParser::parse(traveltimes);
    propseqsdom=JSONParser::parse(propseqs);
    newactualseqsdom=JSONParser::parse(newactualseqs);
    scoresdom=JSONParser::parse(scores);
    loadRoutes();
    loadSequences();
}

void SolutionInspector::inspect() const {
    /*
    const auto& patt=model.pattern();
    patt.print();
    */
    cout<<"inspecting "<<routes.size()<<" proposed sequences   (subm. score: "
            <<scoresdom["submission_score"].GetDouble()<<")"<<endl;
    statsPerStation();
    vector<TestRoute> vroutes;
    for (const auto& kv : routes)
        vroutes.push_back(kv.second);
    sort(vroutes.begin(), vroutes.end(),
            [&](const TestRoute& r1, const TestRoute& r2)
            {return scoresdom["route_scores"][r1.id().c_str()].GetDouble()
                  > scoresdom["route_scores"][r2.id().c_str()].GetDouble();});
    for (const auto& rt : vroutes) {
        cout<<rt.id()<<"  score: "<<scoresdom["route_scores"][rt.id().c_str()]
                .GetDouble()<<endl;
        using namespace date;
        cout<<"departure: "<<rt.departure()<<endl;
        const auto pseq=rt.sequence();
        const auto aseq=actualseqs.at(rt.id());
        cout<<"score (recomputed): "<<Sequence::score(rt, pseq, aseq)<<endl;
        // export proposed and actual sequences to TikZ/TeX
        rt.exportTikZ("out/TikZ/"+rt.station()+"_"+rt.id()+"_prop.tex");
        rt.exportTikZ(aseq, "out/TikZ/"+rt.station()+"_"+rt.id()+"_act.tex");
        cout<<"\t\tproposed\t\tactual"<<endl;
        cout<<"duration: \t"<<pseq.duration()<<"\t\t\t"<<aseq.duration()<<endl;
        cout<<"earliness: \t"<<pseq.earliness()<<"\t\t\t"<<aseq.earliness()
                <<endl;
        cout<<"lateness: \t"<<pseq.lateness()<<"\t\t\t"<<aseq.lateness()<<endl;
        cout<<"early arriv.: \t"<<pseq.earlyArrivals()<<"\t\t\t"
                <<aseq.earlyArrivals()<<endl;
        cout<<"late arriv.: \t"<<pseq.lateArrivals()<<"\t\t\t"
                <<aseq.lateArrivals()<<endl;
        cout<<"macro trans.: \t"<<Sequence::macroTransitions(pseq, rt)<<"\t\t\t"
                <<Sequence::macroTransitions(aseq, rt)<<endl;
        cout<<"micro trans.: \t"<<Sequence::microTransitions(pseq, rt)<<"\t\t\t"
                <<Sequence::microTransitions(aseq, rt)<<endl;
        cout<<"nano trans.: \t"<<Sequence::nanoTransitions(pseq, rt)<<"\t\t\t"
                <<Sequence::nanoTransitions(aseq, rt)<<endl;
        cout<<"stops:"<<endl;
        for (size_t i=0; i<rt.stops().size(); ++i) {
            const auto& pstop=rt.getStop(pseq.stops()[i]);
            const auto& astop=rt.getStop(aseq.stops()[i]);
            string pstr((pstop.hasTW()?"*":" ")+pstop.id()+" ("
                +to_string(pstop.packages().size())+","
                +to_string((int)(0.5+pstop.serviceTime()))+","
                +to_string((int)(0.5+pstop.volume()))+") "+pstop.microZone());
            string astr((astop.hasTW()?"*":" ")+astop.id()+" ("
                +to_string(astop.packages().size())+","
                +to_string((int)(0.5+astop.serviceTime()))+","
                +to_string((int)(0.5+astop.volume()))+") "+astop.microZone());
            cout<<"\t"<<left<<setw(30)<<pstr<<"\t"<<setw(30)<<astr<<endl;
            /*
            if (i>0) {
                pstr=rt.getStop(pseq.stops()[i-1]).microZone()+" to "
                    +pstop.microZone()+": "+to_string(patt.countMicro(
                    rt.station(), rt.getStop(pseq.stops()[i-1]).microZone(),
                    pstop.microZone()));
                astr=rt.getStop(aseq.stops()[i-1]).microZone()+" to "
                    +astop.microZone()+": "+to_string(patt.countMicro(
                    rt.station(), rt.getStop(aseq.stops()[i-1]).microZone(),
                    astop.microZone()));
                cout<<"\t"<<left<<setw(30)<<pstr<<"\t"<<setw(30)<<astr<<endl;
            }
            */
        }
        cin.get();
    }
}

void SolutionInspector::inspectCSV() const {
    cout<<"submission score,"<<scoresdom["submission_score"].GetDouble()<<endl;
    vector<TestRoute> vroutes;
    for (const auto& kv : routes)
        vroutes.push_back(kv.second);
    sort(vroutes.begin(), vroutes.end(),
            [&](const TestRoute& r1, const TestRoute& r2)
            {return scoresdom["route_scores"][r1.id().c_str()].GetDouble()
                    >scoresdom["route_scores"][r2.id().c_str()].GetDouble();});
    cout<<"routeID,station,score,departure,propDur,propEarliness,propLateness,"
        "propEarlyArrivals,propLateArrivals,actDur,actEarliness,actLateness,"
        "actEarlyArrivals,actLateArrivals,omax,dist,stops,withTW,withStrictTW,"
        "deviation,propSimMacro,propSimMicro,propSimNano,propMacroTrans,"
        "propMicroTrans,propNanoTrans,propMaxEarliness,propMaxLateness"<<endl;
    for (const auto& rt : vroutes) {
        double omax=0;
        if (model.algoInput().hasRoute(rt.station())) {
            const auto& trnroutes=model.algoInput().routes(rt.station());
            for (const auto& tr : trnroutes) {
                if (tr.score()==TrainingRoute::Score::low)
                    continue;
                if (tr.rectangle().intersects(rt.rectangle())) {
                    double olap=tr.rectangle().intersectionArea(rt.rectangle())
                            /rt.rectangle().area();
                    if (olap>omax)
                        omax=olap;
                }
            }
        }
        const auto pseq=rt.sequence();
        const auto aseq=actualseqs.at(rt.id());
        auto withTW=count_if(rt.stops().begin(), rt.stops().end(),
                [](const pair<string, Stop>& p){return p.second.hasTW();});
        auto withStrictTW=count_if(rt.stops().begin(), rt.stops().end(),
                [&](const pair<string, Stop>& p){return p.second.hasTW() &&
                chrono::duration_cast<chrono::minutes>(p.second.endTW()
                -max(rt.departure(), p.second.startTW())).count()<=361;});
        using namespace date;
        double propMaxEarliness=0, propMaxLateness=0;
        for (size_t i=0; i<pseq.stops().size(); ++i) {
            if (pseq.earliness(i)>propMaxEarliness)
                propMaxEarliness=pseq.earliness(i);
            if (pseq.lateness(i)>propMaxLateness)
                propMaxLateness=pseq.lateness(i);
        }
        cout<<rt.id()<<","<<rt.station()<<","<<scoresdom["route_scores"]
            [rt.id().c_str()].GetDouble()<<","<<rt.departure()<<","
            <<pseq.duration()<<","<<pseq.earliness()<<","<<pseq.lateness()<<","
            <<pseq.earlyArrivals()<<","<<pseq.lateArrivals()<<","
            <<aseq.duration()<<","<<aseq.earliness()<<","<<aseq.lateness()<<","
            <<aseq.earlyArrivals()<<","<<aseq.lateArrivals()<<","<<omax<<","
            <<pseq.distance(aseq)<<","<<rt.stops().size()<<","<<withTW<<","
            <<withStrictTW<<","<<aseq.deviation(pseq)<<","
            <<pseq.macroSimilarity()<<","<<pseq.microSimilarity()<<","
            <<pseq.nanoSimilarity()<<","<<pseq.macroTransitions()<<","
            <<pseq.microTransitions()<<","<<pseq.nanoTransitions()<<","
            <<propMaxEarliness<<","<<propMaxLateness<<endl;
    }
}

void SolutionInspector::loadRoutes() {
    for (const auto& route : routedatadom.GetObject()) {
        TestRoute rt(route.name.GetString());
        const auto& routeinfo=route.value;
        rt.setStation(routeinfo["station_code"].GetString());
        rt.setDeparture(routeinfo["date_YYYY_MM_DD"].GetString()+string(" ")
                +routeinfo["departure_time_utc"].GetString());
        for (const auto& stop : routeinfo["stops"].GetObject()) {
            rt.addStop(stop.name.GetString());
            Stop& st=rt.getStop(stop.name.GetString());
            const auto& stopinfo=stop.value;
            st.setType(stopinfo["type"].GetString());
            if (stopinfo["zone_id"].IsString())
                st.setNanoZone(rt.station()+":"
                        +stopinfo["zone_id"].GetString());
            if (stopinfo["lat"].IsDouble() && stopinfo["lng"].IsDouble())
                st.setLatLon(stopinfo["lat"].GetDouble(),
                        stopinfo["lng"].GetDouble());
        }
        routes.insert({rt.id(), rt});
    }
    for (const auto& route : packagedatadom.GetObject()) {
        TestRoute& rt=routes.at(route.name.GetString());
        for (const auto& stop : route.value.GetObject()) {
            Stop& st=rt.getStop(stop.name.GetString());
            for (const auto& pack : stop.value.GetObject()) {
                const auto& packinfo=pack.value;
                Package p(pack.name.GetString(), "UNDEFINED");
                const auto& tw=packinfo["time_window"];
                if (tw["start_time_utc"].IsString()
                        && tw["end_time_utc"].IsString()) {
                    p.setTimeWindow(tw["start_time_utc"].GetString(),
                            tw["end_time_utc"].GetString());
                }
                p.setServiceTime(packinfo["planned_service_time_seconds"]
                        .GetDouble());
                p.setVolume(packinfo["dimensions"]["depth_cm"].GetDouble()
                        *packinfo["dimensions"]["height_cm"].GetDouble()
                        *packinfo["dimensions"]["width_cm"].GetDouble());
                st.addPackage(move(p));
            }
        }
    }
    for (const auto& route : traveltimesdom.GetObject()) {
        TestRoute& rt=routes.at(route.name.GetString());
        TTMatrix ttimes(rt.stops().size());
        for (const auto& from : route.value.GetObject()) {
            auto fromid=from.name.GetString();
            for (const auto& to : from.value.GetObject())
                ttimes.setTravelTime(fromid, to.name.GetString(),
                        to.value.GetDouble());
        }
        rt.setTravelTimes(move(ttimes));
    }
}

void SolutionInspector::loadSequences() {
    for (const auto& route : propseqsdom.GetObject()) {
        TestRoute& rt=routes.at(route.name.GetString());
        vector<string> stopseq(rt.stops().size());
        for (const auto& stop : route.value["proposed"].GetObject())
            stopseq[stop.value.GetInt()]=stop.name.GetString();
        Sequence seq(stopseq);
        rt.setupTiming(seq);
        rt.setSequence(move(seq));
    }
    for (const auto& route : newactualseqsdom.GetObject()) {
        TestRoute& rt=routes.at(route.name.GetString());
        vector<string> stopseq(rt.stops().size());
        for (const auto& stop : route.value["actual"].GetObject())
            stopseq[stop.value.GetInt()]=stop.name.GetString();
        Sequence seq(stopseq);
        rt.setupTiming(seq);
        actualseqs.insert({rt.id(), move(seq)});
    }
}

void SolutionInspector::readModel(const string& filename) {
    ifstream is(filename);
    boost::archive::text_iarchive ia(is);
    ia>>model;
    for (auto& kv : routes) {   // TODO: move this to a new 'setup' function
        auto& rt=kv.second;
        rt.setupSimilarity(rt.sequence(), model.algoInput().pattern());
        rt.setupSimilarity(actualseqs.at(rt.id()), model.algoInput().pattern());
        rt.setupRectangle();
    }
}

void SolutionInspector::statsPerStation() const {
    unordered_map<string, vector<double>> scores;
    for (const auto& kv : routes) {
        scores[kv.second.station()].push_back(
                scoresdom["route_scores"][kv.first.c_str()].GetDouble());
    }
    cout<<"average score per station:"<<endl;
    for (const auto& kv : scores) {
        double sum=accumulate(kv.second.begin(), kv.second.end(), 0.0);
        cout<<kv.first<<": "<<sum<<"/"<<kv.second.size()<<"="
                <<sum/kv.second.size()<<endl;
    }
}

