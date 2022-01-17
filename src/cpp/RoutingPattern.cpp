#include <iostream>
#include "RoutingPattern.h"

using namespace std;

void RoutingPattern::print() const {
    cout<<"printing routing patterns:"<<endl;
    cout<<"macro patterns:"<<endl;
    for (const auto& station : macro) {
        cout<<"station: "<<station.first<<endl;
        for (const auto& macro1 : station.second) {
            for (const auto& macro2 : macro1.second) {
                cout<<"   "<<macro1.first<<" -> "<<macro2.first<<"  "
                        <<macro2.second<<endl;
            }
        }
    }
    cout<<"micro patterns:"<<endl;
    for (const auto& station : micro) {
        cout<<"station: "<<station.first<<endl;
        for (const auto& micro1 : station.second) {
            for (const auto& micro2 : micro1.second) {
                cout<<"   "<<micro1.first<<" -> "<<micro2.first<<"  "
                        <<micro2.second<<endl;
            }
        }
    }
}

