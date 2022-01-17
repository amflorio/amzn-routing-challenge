#include <iostream>
#include "Package.h"

using namespace std;

Package::Package(string id, const string& status) : id_{move(id)} {
    if (status=="DELIVERED")
        status_=Status::delivered;
    else if (status=="DELIVERY_ATTEMPTED")
        status_=Status::attempted;
    else if (status=="REJECTED")
        status_=Status::rejected;
    else if (status=="UNDEFINED")
        status_=Status::undefined;
}

void Package::setTimeWindow(const string& start, const string& end) {
    using namespace date;
    hastw=true;
    istringstream in(start);
    in>>parse("%Y-%m-%d %H:%M:%S", starttw);
    in=istringstream(end);
    in>>parse("%Y-%m-%d %H:%M:%S", endtw);
    if (endtw<starttw)
        cout<<"warning: invalid package time window"<<endl;
}

