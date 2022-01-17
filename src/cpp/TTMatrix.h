#ifndef ttmatrix_h
#define ttmatrix_h

#include <iostream>
#include <string>
#include <unordered_map>
#include <vector>

class TTMatrix {
    private:
        std::vector<std::vector<double>> ttimes;
        std::unordered_map<std::string, size_t> str_to_idx;
    public:
        TTMatrix(size_t dim) : ttimes(dim, std::vector<double>(dim)) {}
        bool consistent() const {
            for (size_t i=0; i<ttimes.size(); ++i)
                for (size_t j=0; j<ttimes.size(); ++j)
                    if (i!=j && ttimes[i][j]<-1e-3) {
                        std::cout<<"warning: invalid travel time value"
                                <<std::endl;
                        return false;
                    }
            return true;
        }
        TTMatrix normalize() const;
        void setTravelTime(const std::string& from, const std::string& to,
                double t) {
            if (str_to_idx.count(from)==0)
                str_to_idx.insert({from, str_to_idx.size()});
            if (str_to_idx.count(to)==0)
                str_to_idx.insert({to, str_to_idx.size()});
            size_t idx_from=str_to_idx.at(from);
            size_t idx_to=str_to_idx.at(to);
            if (idx_from>=ttimes.size() || idx_to>=ttimes.size())
                std::cout<<"warning: index out of range"<<std::endl;
            ttimes[idx_from][idx_to]=t;
        }
        double travelTime(const std::string& from, const std::string& to)
                const {
            if (str_to_idx.count(from)==0)
                std::cout<<"warning: (from) stop \""<<from<<"\" does not exist"
                        <<std::endl;
            if (str_to_idx.count(to)==0)
                std::cout<<"warning: (to) stop \""<<to<<"\" does not exist"
                        <<std::endl;
            return ttimes[str_to_idx.at(from)][str_to_idx.at(to)];
        }
};

#endif

