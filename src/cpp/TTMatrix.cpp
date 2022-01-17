#include <cmath>
#include <limits>
#include "TTMatrix.h"

using namespace std;

TTMatrix TTMatrix::normalize() const {
    // compute travel times average and std deviation
    double sum_tt=0;
    for (size_t i=0; i<ttimes.size(); ++i)
        for (size_t j=0; j<ttimes.size(); ++j)
            sum_tt+=ttimes[i][j];
    double avg_tt=sum_tt/(ttimes.size()*ttimes.size());
    double sum_sq=0;
    for (size_t i=0; i<ttimes.size(); ++i)
        for (size_t j=0; j<ttimes.size(); ++j)
            sum_sq+=(ttimes[i][j]-avg_tt)*(ttimes[i][j]-avg_tt);
    double std_tt=sqrt(sum_sq/(ttimes.size()*ttimes.size()));
    // first pass: normalizing and finding minimum normalized travel time
    TTMatrix norm=*this;
    double min_tt=numeric_limits<double>::max();
    for (size_t i=0; i<norm.ttimes.size(); ++i)
        for (size_t j=0; j<norm.ttimes.size(); ++j) {
            double norm_tt=(this->ttimes[i][j]-avg_tt)/std_tt;
            norm.ttimes[i][j]=norm_tt;
            if (isfinite(norm_tt) && norm_tt<min_tt)
                min_tt=norm_tt;
        }
    // second pass: shift travel times to eliminate negative values
    for (size_t i=0; i<norm.ttimes.size(); ++i)
        for (size_t j=0; j<norm.ttimes.size(); ++j)
            norm.ttimes[i][j]=isfinite(norm.ttimes[i][j])
                    ?norm.ttimes[i][j]-min_tt:0;
    return norm;
}

