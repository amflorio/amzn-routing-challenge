#ifndef package_h
#define package_h

#include <string>
#include <utility>
#include "date/date.h"

class Package {
    public:
        enum class Status {delivered, attempted, rejected, undefined};
        Package(std::string id, const std::string& status);
        const std::string& id() const {return id_;}
        date::sys_seconds endTW() const {return endtw;}
        bool hasTW() const {return hastw;}
        double serviceTime() const {return stime;}
        void setServiceTime(double t) {stime=t;}
        void setTimeWindow(const std::string& start, const std::string& end);
        void setVolume(double v) {vol=v;}
        date::sys_seconds startTW() const {return starttw;}
        Status status() const {return status_;}
        double volume() const {return vol;}
    private:
        std::string id_;
        Status status_;
        bool hastw=false;
        date::sys_seconds starttw, endtw;
        double stime;
        double vol=0;
};

#endif

