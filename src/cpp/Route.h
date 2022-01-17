#ifndef route_h
#define route_h

#include <numeric>
#include <string>
#include <unordered_map>
#include "date/date.h"
#include "Rectangle.h"
#include "RoutingPattern.h"
#include "Sequence.h"
#include "Stop.h"
#include "TTMatrix.h"

class Route {
    protected:
        std::string id_;
        std::string station_;
        std::unordered_map<std::string, Stop> stops_;
        date::sys_seconds departure_;
        Rectangle rect;         // minimum bounding rectangle of dropoff stops
        bool incomp=false;      // becomes true if data is inconsistent
        TTMatrix ttimes;
        Sequence seq;
        Sequence toSequence(const std::vector<std::string>& idx_to_stopid,
                const std::vector<int>& tour) const;
    public:
        Route(std::string id) : id_{std::move(id)}, ttimes(0), seq({}) {}
        Route(std::string id, std::string station,
            std::unordered_map<std::string, Stop> stops, date::sys_seconds dep,
            Rectangle r, TTMatrix ttmatrix) : id_{std::move(id)},
            station_{std::move(station)}, stops_{std::move(stops)},
            departure_{std::move(dep)}, rect{std::move(r)},
            ttimes{std::move(ttmatrix)}, seq({}) {}
        void addStop(const std::string& stopid)
                {stops_.emplace(stopid, Stop(stopid));}
        size_t countTimeWindows(int mindur, int maxdur) const {
            size_t ntws=0;
            for (const auto& kv : stops_) {
                const auto& s=kv.second;
                if (s.hasTW()) {
                    int minutes=std::chrono::duration_cast<std::chrono::minutes>
                        (s.endTW()-std::max(departure_, s.startTW())).count();
                    if (minutes>=mindur && minutes<=maxdur)
                        ntws++;
                }
            }
            return ntws;
        }
        date::sys_seconds departure() const {return departure_;}
        double distanceNearestDropoff() const {
            double station_lat=0, station_lon=0;
            for (const auto& kv : stops_) {
                const auto& s=kv.second;
                if (s.type()==Stop::Type::station) {
                    station_lat=s.lat();
                    station_lon=s.lon();
                    break;
                }
            }
            const double lat0=station_lat*(3.14159/180);    // to radians
            const double station_x=station_lon;
            const double station_y=station_lat*std::cos(lat0);
            double nearest=std::numeric_limits<double>::max();
            for (const auto& kv : stops_) {
                const auto& s=kv.second;
                if (s.type()==Stop::Type::dropoff) {
                    double stop_x=s.lon();
                    double stop_y=s.lat()*std::cos(lat0);
                    double diff_x=station_x-stop_x;
                    double diff_y=station_y-stop_y;
                    double dist=std::sqrt( diff_x*diff_x + diff_y*diff_y );
                    if (dist<nearest)
                        nearest=dist;
                }
            }
            return nearest;
        }
        int distinctMacroZones() const;
        int distinctMicroZones() const;
        int distinctNanoZones() const;
        void exportTikZ(const Sequence& s, const std::string& texfile) const;
        void exportTikZ(const std::string& texfile) const;
        Stop& getStop(const std::string& stopid) {return stops_.at(stopid);}
        const Stop& getStop(const std::string& stopid) const
                {return stops_.at(stopid);}
        bool hasStop(const std::string& stopid) const
                {return stops_.count(stopid)==1;}
        const std::string& id() const {return id_;}
        bool incomplete() const {return incomp;}
        std::string mainMacroZone() const;
        std::string mainMicroZone() const;
        size_t numPackages() const {
            return std::accumulate(stops_.begin(), stops_.end(), 0,
                    [](size_t a, const std::pair<std::string, Stop>& kv)
                    {return a+kv.second.packages().size();});
        }
        std::unordered_map<std::string, double> ratioMacroZones() const;
        const Rectangle& rectangle() const {return rect;}
        const Sequence& sequence() const {return seq;}
        Sequence& sequence() {return seq;} 
        double serviceTime() const {
            return std::accumulate(stops_.begin(), stops_.end(), 0.0,
                    [](double a, const std::pair<std::string, Stop>& kv)
                    {return a+kv.second.serviceTime();});
        }
        void setDeparture(const std::string& datetime);
        void setIncomplete() {incomp=true;}
        void setSequence(Sequence s) {seq=std::move(s);}
        void setStation(std::string s) {station_=std::move(s);}
        void setTravelTimes(TTMatrix ttmatrix) {ttimes=std::move(ttmatrix);}
        void setupRectangle();
        void setupSimilarity(Sequence& seq, const RoutingPattern& patt) const;
        void setupTiming(Sequence& seq) const;
        const std::string& station() const {return station_;}
        const std::unordered_map<std::string, Stop>& stops() const
                {return stops_;}
        const TTMatrix& travelTimes() const {return ttimes;}
        bool validateTravelTimeMatrix(const TTMatrix& ttmatrix) const;
};

#endif

