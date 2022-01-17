#include <iostream>
#include <string>
#include <boost/archive/text_iarchive.hpp>
#include <boost/archive/text_oarchive.hpp>
#include "DatasetBuilder.h"
#include "Learner.h"
#include "Model.h"
#include "SolutionInspector.h"
#include "Tester.h"

using namespace std;

int main(int argc, char* argv[]) {
    if (argc<2) {
        cerr<<"usage: "<<argv[0]<<" <mode>"<<endl;
        cerr<<"where <mode> ="<<endl;
        cerr<<"\t0  build model"<<endl;
        cerr<<"\t1  apply model"<<endl;
        cerr<<"\t2  build development dataset"<<endl;
        cerr<<"\t3  inspect solution"<<endl;
        cerr<<"\t4  modify current model"<<endl;
        return EXIT_FAILURE;
    }
    int mode=stoi(argv[1]);
    if (mode<0 || mode>4) {
        cerr<<"invalid mode"<<endl;
        return EXIT_FAILURE;
    }
    const string path_mbi="data/model_build_inputs/";
    const string path_mai="data/model_apply_inputs/";
    const string path_mao="data/model_apply_outputs/";
    const string path_msi="data/model_score_inputs/";
    const string path_mso="data/model_score_outputs/";
    const string modelfile="data/model_build_outputs/model.data";
    const string propseqs="data/model_apply_outputs/proposed_sequences.json";
    if (mode==0) {
        cout<<argv[0]<<": building model ..."<<endl;
        Learner l(path_mbi+"actual_sequences.json",
                path_mbi+"invalid_sequence_scores.json",
                path_mbi+"package_data.json", path_mbi+"route_data.json",
                path_mbi+"travel_times.json");
        l.learn(modelfile);
        //l.exportFeatures("data/model_build_outputs/");
    } else if (mode==1) {
        cout<<argv[0]<<": applying model ..."<<endl;
        Tester t(path_mai+"new_package_data.json",
                path_mai+"new_route_data.json",
                path_mai+"new_travel_times.json");
        t.readModel(modelfile);
        t.test();
        t.saveSequences(propseqs);
    } else if (mode==2) {
        cout<<argv[0]<<": building development dataset ..."<<endl;
        // load original dataset (training+validation+scores)
        DatasetBuilder db(path_mbi+"actual_sequences.json",
            path_mbi+"invalid_sequence_scores.json",
            path_mbi+"package_data.json", path_mbi+"route_data.json",
            path_mbi+"travel_times.json", path_msi+"new_actual_sequences.json",
            path_msi+"new_invalid_sequence_scores.json",
            path_mai+"new_package_data.json", path_mai+"new_route_data.json",
            path_mai+"new_travel_times.json");
        // convert part of training data to validation data
        db.convertToValidation();
        // save dataset
        const string path_dmbi="devdata/model_build_inputs/";
        const string path_dmai="devdata/model_apply_inputs/";
        const string path_dmsi="devdata/model_score_inputs/";
        db.saveDataset(path_dmbi+"actual_sequences.json",
            path_dmbi+"invalid_sequence_scores.json",
            path_dmbi+"package_data.json", path_dmbi+"route_data.json",
            path_dmbi+"travel_times.json",path_dmsi+"new_actual_sequences.json",
            path_dmsi+"new_invalid_sequence_scores.json",
            path_dmai+"new_package_data.json", path_dmai+"new_route_data.json",
            path_dmai+"new_travel_times.json");
    } else if (mode==3) {
        if (argc!=5) {
            cerr<<"usage: "<<argv[0]<<" 3 <propseqs> <scores> <csv>"<<endl;
            cerr<<"where"<<endl;
            cerr<<"    <propseqs>  JSON file: proposed sequences"<<endl;
            cerr<<"    <scores>    JSON file: scores"<<endl;
            cerr<<"    <csv>       'y' for CSV, 'n' for regular output"<<endl;
            return EXIT_FAILURE;
        }
        string propseqs(argv[2]);
        string scores(argv[3]);
        SolutionInspector si(path_mai+"new_package_data.json",
                path_mai+"new_route_data.json",
                path_mai+"new_travel_times.json", propseqs,
                path_msi+"new_actual_sequences.json", scores);
        si.readModel(modelfile);
        if (argv[4][0]=='y')
            si.inspectCSV();
        else
            si.inspect();
    } else {
        Model model;
        ifstream is(modelfile);
        boost::archive::text_iarchive ia(is);
        ia>>model;
        ofstream os(modelfile);
        boost::archive::text_oarchive oa(os);
        oa<<model;
    }
    return EXIT_SUCCESS;
}

