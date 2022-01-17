#include <iostream>
#include "Stop.h"

using namespace std;

void Stop::addPackage(Package p) {
    if (p.hasTW()) {
        if (!hastw) {
            hastw=true;
            starttw=p.startTW();
            endtw=p.endTW();
        } else {    // TODO: what if pack TW's have no overlap?
            if (p.startTW()>starttw)
                starttw=p.startTW();
            if (p.endTW()<endtw)
                endtw=p.endTW();
        }
        if (endtw<starttw)
            cout<<"warning: invalid stop time window"<<endl;
    }
    packs.push_back(move(p));
}

bool Stop::setType(const string& type) {
    if (type=="Dropoff") {
        type_=Type::dropoff;
        return true;
    }
    if (type=="Station") {
        type_=Type::station;
        return true;
    }
    return false;
}

