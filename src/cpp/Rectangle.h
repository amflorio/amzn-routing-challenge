#ifndef rectangle_h
#define rectangle_h

#include <boost/serialization/serialization.hpp>

class Rectangle {
    friend class boost::serialization::access;
    private:
        double left, right, bottom, top;
        template<class Archive> void serialize(Archive& ar,
                const unsigned int version) {
            ar & left;
            ar & right;
            ar & bottom;
            ar & top;
        }
    public:
        double area() const {return (top-bottom)*(right-left);}
        double intersectionArea(const Rectangle& r) const {
            if (!intersects(r))
                return 0;
            return (std::min(top, r.top)-std::max(bottom, r.bottom))
                  *(std::min(right, r.right)-std::max(left, r.left));
        }
        bool intersects(const Rectangle& r) const {
            return left < r.right
               && right > r.left
                 && top > r.bottom
              && bottom < r.top;
        }
        void setBounds(double l, double r, double b, double t) {
            left=l; right=r; bottom=b; top=t;
        }
};

#endif

