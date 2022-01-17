#include <algorithm>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <memory>
#include <numeric>
#include <random>
#include <string>
#include <mlpack/core.hpp>
#include <mlpack/methods/lars/lars.hpp>
#include "LassoRegression.h"
#include "Model.h"
#include "Stopwatch.h"

using namespace std;

void LassoRegression::checkData() const {
    for (const auto& dp : cv_data) {
        if (!isfinite(dp.y))
            cout<<"warning: y value is '"<<dp.y<<"'"<<endl;
        for (const auto& kv : dp.x)
            if (!isfinite(kv.second)) {
                cout<<"warning: feature \""<<kv.first<<"\" is '"<<kv.second
                        <<"'"<<endl;
            }
    }
    for (const auto& dp : tr_data) {
        if (!isfinite(dp.y))
            cout<<"warning: y value is '"<<dp.y<<"'"<<endl;
        for (const auto& kv : dp.x)
            if (!isfinite(kv.second)) {
                cout<<"warning: feature \""<<kv.first<<"\" is '"<<kv.second
                        <<"'"<<endl;
            }
    }
}

void LassoRegression::computeNormMinMax() {
    const auto featset=featureSet();
    unordered_map<string, vector<double>> featvals;
    for (const auto& dp : cv_data)
        for (const auto& feat : featset)
            if (dp.x.count(feat)==1)
                featvals[feat].push_back(dp.x.at(feat));
            else
                featvals[feat].push_back(0);
    for (const auto& dp : tr_data)
        for (const auto& feat : featset)
            if (dp.x.count(feat)==1)
                featvals[feat].push_back(dp.x.at(feat));
            else
                featvals[feat].push_back(0);
    for (const auto& kv : featvals) {
        norm_min[kv.first]=*min_element(kv.second.begin(), kv.second.end());
        norm_max[kv.first]=*max_element(kv.second.begin(), kv.second.end());
        if (norm_max[kv.first]-norm_min[kv.first]<=0) {
            cout<<"warning: feature \""<<kv.first<<"\":"<<endl;
            cout<<"         invalid 0-1 normalization parameters:  min: "
                    <<norm_min[kv.first]<<" ,  max: "<<norm_max[kv.first]<<endl;
            cout<<"         resetting to min=0 and max=1"<<endl;
            norm_min[kv.first]=0;
            norm_max[kv.first]=1;
        }
    }
}

void LassoRegression::exportCSV(const string& csvfile) const {
    cout<<"exporting regression model to "<<csvfile<<" ..."<<endl;
    ofstream csv(csvfile);
    csv<<setprecision(15);
    const auto featset=featureSet();
    for (const auto& feat : featset)
        csv<<feat<<",";
    csv<<"y"<<endl;
    for (const auto& dp : cv_data) {
        for (const auto& feat : featset)
            csv<<(dp.x.count(feat)==1 ? to_string(dp.x.at(feat)) : "0")<<",";
        csv<<dp.y<<endl;
    }
    for (const auto& dp : tr_data) {
        for (const auto& feat : featset)
            csv<<(dp.x.count(feat)==1 ? to_string(dp.x.at(feat)) : "0")<<",";
        csv<<dp.y<<endl;
    }
}

unordered_set<string> LassoRegression::featureSet() const {
    unordered_set<string> featset;
    for (const auto& dp : cv_data)
        for (const auto& kv : dp.x)
            featset.insert(kv.first);
    for (const auto& dp : tr_data)
        for (const auto& kv : dp.x)
            featset.insert(kv.first);
    return featset;
}

double LassoRegression::predict(const unordered_map<string, double>& x) const {
    if (beta.empty())
        return numeric_limits<double>::max();
    double pred=beta_0;
    for (const auto& kv : x)
        if (beta.count(kv.first)==1)
            pred+=beta.at(kv.first)*normalize(kv);
    return pred;
}

void LassoRegression::printBeta() const {
    cout<<"beta_zero (intercept): "<<beta_0<<endl;
    for (const auto& kv : beta)
        cout<<"beta[\""<<kv.first<<"\"]: "<<kv.second<<endl;
}

// this is extremely unnice but we need to protect against mlpack deadlocking
void LassoRegression::save() {
    // assuming that if mastermodel is set then this is an evaluation model
    if (mastermodel!=nullptr) {
        LassoRegression modelcopy;
        // deep copy, everything but the data
        modelcopy.feat_to_idx=feat_to_idx;
        modelcopy.idx_to_feat=idx_to_feat;
        modelcopy.norm_min=norm_min;
        modelcopy.norm_max=norm_max;
        modelcopy.beta_0=beta_0;
        modelcopy.beta=beta;
        mastermodel->setEvaluationModel(move(modelcopy));
        // persistent save: if mlpack deadlocks, at least we have something
        mastermodel->save();
    } else
        cout<<"warning: master model not set"<<endl;
}

void LassoRegression::solve(const double l1_ini) {
    // parameters
    const double l1_sm=1.07;            // step multiplier
    cout<<"lasso: setting up data ..."<<endl;
    checkData();
    computeNormMinMax();
    updateIndices();
    cout<<"lasso: "<<tr_data.size()<<" training data points ;  "<<cv_data.size()
            <<" CV data points ;  "<<feat_to_idx.size()<<" features"<<endl;
    Stopwatch sw;
    cout<<"tuning l1 coefficient by cross validation"<<endl;
    double min_err=numeric_limits<double>::max();
    double l1_best=l1_ini;
    int noupdate=0;
    size_t n_its=0;
    for (double l1=l1_ini; n_its<200; ++n_its) {
        const auto errors=solveL1withCV(l1);
        if (errors.first<min_err*1.01) {
            printBeta();
            if (mastermodel!=nullptr)
                save();     // mlpack sometimes deadlocks ...
            if (errors.first<min_err)
                cout<<"*";
            else
                cout<<" ";
            min_err=min(errors.first, min_err);
            l1_best=l1;
            cout<<"*";
            noupdate=0;
        } else {
            cout<<"  ";
            noupdate++;
        }
        cout<<"l1="<<l1<<": in-sample error: "<<errors.second<<" CV error: "
                <<errors.first<<"  ("<<sw.elapsedSeconds()<<" s)"<<endl;
        if (errors.first>1.015*min_err && noupdate>=5)
            break;
        l1*=l1_sm;
    }
    cout<<"setting l1 regularization coefficient to "<<l1_best<<endl;
    solveAllData(l1_best);
    printBeta();
    cout<<"l1 coefficient tuning: elapsed time: "<<sw.elapsedSeconds()<<" s"
            <<endl;
}

void LassoRegression::solveAllData(double l1) {
    cout<<"solving for l1="<<l1<<" (all data)"<<endl;
    arma::mat X(tr_data.size()+cv_data.size(), feat_to_idx.size(),
            arma::fill::zeros);
    arma::rowvec Y(tr_data.size()+cv_data.size(), arma::fill::zeros);
    for (size_t i=0; i<tr_data.size(); ++i) {
        const auto& dp=tr_data[i];
        for (const auto& kv : dp.x)
            X(i, feat_to_idx.at(kv.first))=normalize(kv);
        Y(i)=dp.y;
    }
    for (size_t i=0; i<cv_data.size(); ++i) {
        const auto& dp=cv_data[i];
        for (const auto& kv : dp.x)
            X(i+tr_data.size(), feat_to_idx.at(kv.first))=normalize(kv);
        Y(i+tr_data.size())=dp.y;
    }
    cout<<"covariates matrix populated:  "<<X.n_rows<<" rows,  "<<X.n_cols
            <<" columns,  "<<arma::accu(X!=0)<<" nonzeros"<<endl;
    using namespace mlpack::regression;
    shared_ptr<LARS> lars=make_shared<LARS>(X, Y, false, false, l1);
    auto B=lars->Beta();
    cout<<"beta vector:  "<<B.n_rows<<" rows,  "<<B.n_cols<<" columns,  "
            <<arma::accu(B!=0)<<" nonzeros"<<endl;
    if (B.n_rows!=feat_to_idx.size())
        cout<<"warning: dimensions not matching"<<endl;
    beta.clear();       // in case we are modifying the current model
    for (size_t i=0; i<B.n_rows; ++i) {
        if (B(i)!=0) {
            cout<<"beta_"<<i<<" (\""<<idx_to_feat.at(i)<<"\"): "<<B(i)<<endl;
            beta.insert({idx_to_feat.at(i), B(i)});
        }
        if (!isfinite(B(i))) {
            cout<<"warning: invalid beta"<<endl;
            cout<<"         calling recursively with l1="<<l1*1.05<<endl;
            solveAllData(l1*1.05);
            return;
        }
    }
    // save intercept term
    arma::mat X0(1, feat_to_idx.size(), arma::fill::zeros);
    arma::rowvec B0;
    lars->Predict(X0, B0, true);
    beta_0=B0(0);
    cout<<"beta_zero (intercept): "<<beta_0<<endl;
    arma::rowvec predY;
    lars->Predict(X, predY, true);
    double loss=0;
    for (size_t i=0; i<predY.n_cols; ++i) {
        if (i<tr_data.size()) {
            if (abs(predY(i)-predict(tr_data.at(i).x))>1e-6
                    || abs(Y(i)-tr_data.at(i).y)>1e-6)
                cout<<"warning: values mismatching"<<endl;
        } else {
            if (abs(predY(i)-predict(cv_data.at(i-tr_data.size()).x))>1e-6
                    || abs(Y(i)-cv_data.at(i-tr_data.size()).y)>1e-6)
                cout<<"warning: values mismatching"<<endl;
        }
        double diff=predY(i)-Y(i);
        loss+=diff*diff;
    }
    loss/=predY.n_cols;
    cout<<"l1="<<l1<<": training error: "<<loss<<endl;
}

pair<double, double> LassoRegression::solveL1withCV(const double l1) {
    double totcvloss=0, tottrloss=0;
    // matrix for training
    arma::mat X_tr(tr_data.size(), feat_to_idx.size(), arma::fill::zeros);
    // matrix for CV
    arma::mat X_cv(cv_data.size(), feat_to_idx.size(), arma::fill::zeros);
    // Y vectors for training and CV
    arma::rowvec Y_tr(tr_data.size(), arma::fill::zeros);
    arma::rowvec Y_cv(cv_data.size(), arma::fill::zeros);
    for (size_t i=0; i<tr_data.size(); ++i) {
        const auto& dp=tr_data[i];
        for (const auto& kv : dp.x)
            X_tr(i, feat_to_idx.at(kv.first))=normalize(kv);
        Y_tr(i)=dp.y;
    }
    for (size_t i=0; i<cv_data.size(); ++i) {
        const auto& dp=cv_data[i];
        for (const auto& kv : dp.x)
            X_cv(i, feat_to_idx.at(kv.first))=normalize(kv);
        Y_cv(i)=dp.y;
    }
    using namespace mlpack::regression;
    LARS lars(X_tr, Y_tr, false, false, l1);
    auto B=lars.Beta();
    cout<<"beta vector:  "<<B.n_rows<<" rows,  "<<B.n_cols<<" columns,  "
            <<arma::accu(B!=0)<<" nonzeros"<<endl;
    if (B.n_rows!=feat_to_idx.size())
        cout<<"warning: dimensions not matching"<<endl;
    beta.clear();       // in case we are modifying the current model
    if (B.has_nan()) {
        cout<<"invalid (NaN) element in beta vector"<<endl;
        return {numeric_limits<double>::max(), numeric_limits<double>::max()};
    }
    for (size_t i=0; i<B.n_rows; ++i) {
        if (B(i)!=0) {
            cout<<"beta_"<<i<<" (\""<<idx_to_feat.at(i)<<"\"): "<<B(i)<<endl;
            beta.insert({idx_to_feat.at(i), B(i)});
        }
        if (!isfinite(B(i))) {
            cout<<"warning: invalid beta"<<endl;
            return {numeric_limits<double>::max(),
                    numeric_limits<double>::max()};
        }
    }
    // save intercept term
    arma::mat X0(1, feat_to_idx.size(), arma::fill::zeros);
    arma::rowvec B0;
    lars.Predict(X0, B0, true);
    beta_0=B0(0);
    cout<<"beta_zero (intercept): "<<beta_0<<endl;
    {   // in-sample error
        arma::rowvec predY;
        lars.Predict(X_tr, predY, true);
        if (predY.n_cols!=tr_data.size())
            cout<<"warning: dimensions not matching"<<endl;
        double loss=0;
        for (size_t j=0; j<predY.n_cols; ++j) {
            double diff=predY(j)-Y_tr(j);
            loss+=diff*diff;
        }
        tottrloss+=loss/predY.n_cols;
    }
    {   // cross validation error
        arma::rowvec predY;
        lars.Predict(X_cv, predY, true);
        if (predY.n_cols!=cv_data.size())
            cout<<"warning: dimensions not matching"<<endl;
        double loss=0;
        for (size_t j=0; j<predY.n_cols; ++j) {
            double diff=predY(j)-Y_cv(j);
            loss+=diff*diff;
        }
        totcvloss+=loss/predY.n_cols;
    }
    return {totcvloss, tottrloss};
}

void LassoRegression::updateIndices() {
    if (feat_to_idx.size()!=idx_to_feat.size())
        cout<<"warning: sizes of indices differ"<<endl;
    for (const auto& dp : tr_data)
        for (const auto& kv : dp.x)
            if (feat_to_idx.count(kv.first)==0) {
                feat_to_idx.insert({kv.first, idx_to_feat.size()});
                idx_to_feat.push_back(kv.first);
            }
    for (const auto& dp : cv_data)
        for (const auto& kv : dp.x)
            if (feat_to_idx.count(kv.first)==0) {
                feat_to_idx.insert({kv.first, idx_to_feat.size()});
                idx_to_feat.push_back(kv.first);
            }
}

