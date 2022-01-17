#include <algorithm>
#include "LocalSearch.h"
#include "Route.h"
#include "Sequence.h"

using namespace std;

bool LocalSearch::myOpt(Sequence& seq, const Route& r) {
    bool success=false;
    auto& stops=seq.stops();
    const auto& TT=r.travelTimes();
    bool improv=true;
    while (improv) {
        improv=false;
        for (size_t i=1; i+3<stops.size(); ++i) {   // don't change 1st nor last
            const auto& curr=stops[i];
            const auto& n1=stops[i+1];
            const auto& n2=stops[i+2];
            const auto& n3=stops[i+3];
            const double currtt=TT.travelTime(curr,n1)+TT.travelTime(n1,n2)
                    +TT.travelTime(n2,n3);
            const double swaptt=TT.travelTime(curr,n2)+TT.travelTime(n2,n1)
                    +TT.travelTime(n1,n3);
            if (swaptt<currtt) {
                seq.swap(i+1, i+2);
                improv=true;
                success=true;
                if (i>=3)
                    i-=3;
                else
                    i=0;
            }
        }
    }
    return success;
}

