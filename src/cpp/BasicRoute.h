#ifndef basicroute_h
#define basicroute_h

#include <cmath>
#include <vector>
#include <boost/serialization/binary_object.hpp>
#include <boost/serialization/serialization.hpp>
#include "date/date.h"
#include "BasicStop.h"
#include "Rectangle.h"
#include "TrainingRoute.h"

class BasicRoute {
    friend class boost::serialization::access;
    private:
        Rectangle rect;
        std::vector<BasicStop> stops_;
        int score_;
        date::sys_seconds departure_;
        double twstrictness;
        template<class Archive> void serialize(Archive& ar,
                const unsigned int version) {
            ar & rect;
            ar & stops_;
            ar & score_;
            ar & boost::serialization::make_binary_object(&departure_,
                    sizeof(departure_));
            ar & twstrictness;
        }
    public:
        date::sys_seconds departure() const {return departure_;}
        const Rectangle& rectangle() const {return rect;}
        void setDeparture(date::sys_seconds d) {departure_=d;}
        void setRectangle(Rectangle r) {rect=std::move(r);}
        void setScore(TrainingRoute::Score s) {
            // we store score as basic type otherwise serialization is annoying
            score_=s==TrainingRoute::Score::high?2
                    :s==TrainingRoute::Score::medium?1
                    :0;
        }
        TrainingRoute::Score score() const {
            return score_==2?TrainingRoute::Score::high
                    :score_==1?TrainingRoute::Score::medium
                    :TrainingRoute::Score::low;
        }
        void setStops(std::vector<BasicStop> v) {stops_=std::move(v);}
        void setTWStrictness(double s) {    // cannot serialize nan's nor inf's
            twstrictness=std::isfinite(s)?s:0;
        }
        const std::vector<BasicStop>& stops() const {return stops_;}
        double twStrictness() const {return twstrictness;}
};

#endif

