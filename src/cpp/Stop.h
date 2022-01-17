#ifndef stop_h
#define stop_h

#include <numeric>
#include <string>
#include <utility>
#include <vector>
#include "date/date.h"
#include "Package.h"

class Stop {
    public:
        enum class Type {dropoff, station};
        Stop(std::string id) : id_{std::move(id)} {}
        void addPackage(Package p);
        date::sys_seconds endTW() const {return endtw;}
        bool hasTW() const {return hastw;}
        const std::string& id() const {return id_;}
        double lat() const {return lat_;}
        double lon() const {return lon_;}
        date::sys_seconds midpointTW() const {
            return starttw+std::chrono::seconds(std::chrono::duration_cast<
                    std::chrono::seconds>(endtw-starttw)/2);
        }
        std::string macroZone() const {return type_==Type::station?"station"
                :zone_.substr(0, zone_.find("-"));}
        std::string microZone() const {return type_==Type::station?"station"
                :zone_.substr(0, zone_.find("."));}
        std::string nanoZone() const {return type_==Type::station?"station"
                :zone_;}
        const std::vector<Package>& packages() const {return packs;}
        std::vector<Package>& packages() {return packs;}
        double serviceTime() const {
            return std::accumulate(packs.begin(), packs.end(), 0.0,
                    [](double a, const Package& p){return a+p.serviceTime();});
        }
        void setLatLon(double lat, double lon) {lat_=lat; lon_=lon;}
        bool setType(const std::string& type);
        void setNanoZone(std::string z) {zone_=std::move(z);}
        date::sys_seconds startTW() const {return starttw;}
        Type type() const {return type_;}
        double volume() const {
            return std::accumulate(packs.begin(), packs.end(), 0.0,
                    [](double a, const Package& p){return a+p.volume();});
        }
    private:
        std::string id_;
        std::vector<Package> packs;
        Type type_;
        std::string zone_;
        double lat_=0, lon_=0;
        bool hastw=false;
        date::sys_seconds starttw, endtw;
};

#endif

