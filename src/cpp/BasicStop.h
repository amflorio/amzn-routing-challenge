#ifndef basicstop_h
#define basicstop_h

#include <boost/serialization/serialization.hpp>

class BasicStop {
    friend class boost::serialization::access;
    private:
        double lat_=0, lon_=0;
        size_t npacks=0;
        template<class Archive> void serialize(Archive& ar,
                const unsigned int version) {
            ar & lat_;
            ar & lon_;
            ar & npacks;
        }
    public:
        double lat() const {return lat_;}
        double lon() const {return lon_;}
        size_t numPackages() const {return npacks;}
        void setNumPackages(size_t n) {npacks=n;}
        void setLatLon(double lat, double lon) {lat_=lat; lon_=lon;}
};

#endif

