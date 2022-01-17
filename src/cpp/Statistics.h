#ifndef statistics_h
#define statistics_h

#include <iostream>
#include <limits>
#include <vector>

class Statistics {
    public:
        template<typename T>
        static void basicStats(const std::vector<T>& vals) {
            // min, max and avg
            T minn=std::numeric_limits<T>::max();
            T maxn=0;
            T sum=0;
            for (T val : vals) {
                minn=std::min(minn, val);
                maxn=std::max(maxn, val);
                sum+=val;
            }
            std::cout<<"min: "<<minn<<std::endl;
            std::cout<<"max: "<<maxn<<std::endl;
            std::cout<<"avg: "<<(1.0*sum)/vals.size()<<std::endl;
        }
};

#endif

