#ifndef lassoregression_h
#define lassoregression_h

#include <cmath>
#include <iostream>
#include <string>
#include <unordered_map>
#include <unordered_set>
#include <vector>
#include <boost/serialization/serialization.hpp>

class Model;
class LassoRegression {
    friend class boost::serialization::access;
    private:
        class DataPoint {
            friend class boost::serialization::access;
            private:
                DataPoint() {}      // for serialization
            public:
                std::unordered_map<std::string, double> x;
                double y;
                DataPoint(std::unordered_map<std::string, double> x_, double y_)
                        : x{std::move(x_)}, y{y_} {}
                template<class Archive> void serialize(Archive& ar,
                        const unsigned int version) {
                    ar & x;
                    ar & y;
                }
        };
        std::vector<DataPoint> tr_data, cv_data;
        std::unordered_map<std::string, size_t> feat_to_idx;
        std::vector<std::string> idx_to_feat;
        std::unordered_map<std::string, double> norm_min, norm_max;
        double beta_0=0;
        std::unordered_map<std::string, double> beta;
        Model* mastermodel=nullptr;
        void checkData() const;
        void computeNormMinMax();
        std::unordered_set<std::string> featureSet() const;
        double normalize(const std::pair<std::string, double>& kv) const {
            const auto& feat=kv.first;
            return (kv.second-norm_min.at(feat))
                    /(norm_max.at(feat)-norm_min.at(feat));
        }
        void printBeta() const;
        void solveAllData(double l1);
        std::pair<double, double> solveL1withCV(const double l1);
        void updateIndices();
        template<class Archive> void serialize(Archive& ar,
                const unsigned int version) {
            ar & tr_data;
            ar & cv_data;
            ar & feat_to_idx;
            ar & idx_to_feat;
            ar & norm_min;
            ar & norm_max;
            ar & beta_0;
            ar & beta;
        }
    public:
        void addDataPoint(std::unordered_map<std::string, double> x, double y,
                bool cv) {
            if (!std::isfinite(y)) {
                std::cout<<"warning: y value is '"<<y<<"', ignoring data point"
                        <<std::endl;
                return;
            }
            for (const auto& kv : x)
                if (!std::isfinite(kv.second)) {
                    std::cout<<"warning: feature \""<<kv.first<<"\" is '"
                            <<kv.second<<"', ignoring data point"<<std::endl;
                    return;
                }
            if (cv)
                cv_data.emplace_back(std::move(x), y);
            else
                tr_data.emplace_back(std::move(x), y);
        }
        void clearData() {tr_data.clear(), cv_data.clear();}
        void exportCSV(const std::string& csvfile) const;
        size_t numCVDataPoints() const {return cv_data.size();}
        size_t numTrainingDataPoints() const {return tr_data.size();}
        double predict(const std::unordered_map<std::string, double>& x) const;
        void setMasterModel(Model* m) {mastermodel=m;}
        void save();
        void solve(const double l1_ini);
};

#endif

