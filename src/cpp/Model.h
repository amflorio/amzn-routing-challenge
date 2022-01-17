#ifndef model_h
#define model_h

#include <iostream>
#include <map>
#include <string>
#include <utility>
#include <boost/archive/text_oarchive.hpp>
#include <boost/serialization/map.hpp>
#include <boost/serialization/serialization.hpp>
#include <boost/serialization/string.hpp>
#include "AlgoInput.h"
#include "LassoRegression.h"

class Model {
    friend class boost::serialization::access;
    private:
        AlgoInput input;
        std::map<std::string, LassoRegression> algmodels;
        LassoRegression evlmodel;
        bool evlmodelset=false;
        std::string modelfile;
        template<class Archive> void serialize(Archive& ar,
                const unsigned int version) {
            ar & input;
            ar & algmodels;
            ar & evlmodel;
            ar & evlmodelset;
        }
    public:
        void addAlgorithmModel(std::string id, LassoRegression model) {
            algmodels.insert({std::move(id), std::move(model)});
        }
        const AlgoInput& algoInput() const {return input;}
        const std::map<std::string, LassoRegression>& algorithmModels() const
                {return algmodels;}
        bool hasEvaluationModel() const {return evlmodelset;}
        void save() const {
            std::cout<<"saving model ..."<<std::endl;
            std::ofstream os(modelfile);
            boost::archive::text_oarchive oa(os);
            oa<<*this;
        }
        void setModelFile(const std::string& filename) {
            modelfile=filename;
        }
        const LassoRegression& evaluationModel() const {
            if (!evlmodelset)
                std::cout<<"warning: evaluation model not set"<<std::endl;
            return evlmodel;
        }
        void setAlgoInput(AlgoInput i) {input=std::move(i);}
        void setEvaluationModel(LassoRegression m) {
            evlmodel=std::move(m);
            evlmodelset=true;
        }
};

#endif

