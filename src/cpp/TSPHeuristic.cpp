#include <algorithm>
#include <iostream>
#include <limits>
#include <list>
#include <numeric>
#include <tuple>
#include "TSPHeuristic.h"

using namespace std;

/*
TSPSolution TSPHeuristic::cheapestInsertion() const {
    tuple<size_t,size_t,double> cheapest {0, 0, numeric_limits<double>::max()};
    for (size_t i=0; i<costs.size(); ++i)
        for (size_t j=0; j<costs.size(); ++j)
            if (i!=j && costs[i][j]<get<2>(cheapest))
                cheapest={i, j, costs[i][j]};
    list<size_t> tour {get<0>(cheapest), get<1>(cheapest)};
    list<size_t> pending;
    for (size_t i=0; i<costs.size(); ++i)
        if (i!=tour.front() && i!=tour.back())
            pending.push_back(i);
    double totcost=costs[tour.front()][tour.back()]
            +costs[tour.back()][tour.front()];
    while (!pending.empty()) {
        tuple<list<size_t>::iterator, list<size_t>::iterator, double> best
                {pending.end(), tour.end(), numeric_limits<double>::max()};
        for (auto it_p=pending.begin(); it_p!=pending.end(); ++it_p)
            for (auto it_t=tour.begin(); it_t!=tour.end(); ++it_t) {
                size_t i=it_t==tour.begin()?tour.back():*prev(it_t);
                double cost=costs[i][*it_p]+costs[*it_p][*it_t]-costs[i][*it_t];
                if (cost<get<2>(best))
                    best={it_p, it_t, cost};
            }
        tour.insert(get<1>(best), *get<0>(best));
        pending.erase(get<0>(best));
        totcost+=get<2>(best);
    }
    return {vector<size_t>(tour.begin(), tour.end()), totcost};
}
*/

/*
TSPSolution TSPHeuristic::farthestInsertion() const {
    tuple<size_t, size_t, double> farthest {0, 0, -1};
    for (size_t i=0; i<costs.size(); ++i)
        for (size_t j=0; j<costs.size(); ++j)
            if (i!=j && costs[i][j]>get<2>(farthest))
                farthest={i, j, costs[i][j]};
    list<size_t> tour {get<0>(farthest), get<1>(farthest)};
    list<size_t> pending;
    for (size_t i=0; i<costs.size(); ++i)
        if (i!=tour.front() && i!=tour.back())
            pending.push_back(i);
    double totcost=costs[tour.front()][tour.back()]
            +costs[tour.back()][tour.front()];
    while (!pending.empty()) {
        // find farthest node
        tuple<list<size_t>::iterator, double> far {pending.end(), -1};
        for (auto it_p=pending.begin(); it_p!=pending.end(); ++it_p) {
            double dist=numeric_limits<double>::max();
            for (const auto& i : tour)
                if (costs[*it_p][i]<dist)
                    dist=costs[*it_p][i];
            if (dist>get<1>(far))
                far={it_p, dist};
        }
        size_t p=*get<0>(far);        // farthest node
        // insert farthest node in the tour
        tuple<list<size_t>::iterator, double> best
                {tour.end(), numeric_limits<double>::max()};
        for (auto it_t=tour.begin(); it_t!=tour.end(); ++it_t) {
            size_t i=it_t==tour.begin()?tour.back():*prev(it_t);
            double cost=costs[i][p]+costs[p][*it_t]-costs[i][*it_t];
            if (cost<get<1>(best))
                best={it_t, cost};
        }
        tour.insert(get<0>(best), p);
        pending.erase(get<0>(far));
        totcost+=get<1>(best);
    }
    return {vector<size_t>(tour.begin(), tour.end()), totcost};
}
*/

vector<TSPSolution> TSPHeuristic::pool(size_t n, size_t guide, bool fix) {
    vector<TSPSolution> pool_;
    pool_.reserve(n);
    for (size_t i=0; i<n; ++i)
        pool_.push_back(randomInsertion(guide, fix));
    return pool_;
}

TSPSolution TSPHeuristic::randomInsertion(size_t guide, bool fix) {
    if (guide==1 || guide>costs.size())
        cout<<"warning: invalid guide value"<<endl;
    vector<size_t> pending(costs.size()-guide);
    iota(pending.begin(), pending.end(), guide);
    shuffle(pending.begin(), pending.end(), g);
    list<size_t> tour;
    double totcost=0;
    if (guide==0) {
        tour={pending.end()[-1], pending.end()[-2]};
        pending.resize(pending.size()-2);
        totcost=costs[tour.front()][tour.back()]
                +costs[tour.back()][tour.front()];
    } else {
        for (size_t i=0; i<guide; ++i)
            tour.push_back(i);
        for (size_t i=0; i<guide; ++i)
            totcost+=costs[i==0?guide-1:i-1][i];
    }
    while (!pending.empty()) {
        size_t r=pending.back();           // random node
        pending.pop_back();
        tuple<list<size_t>::iterator, double> best
                {tour.end(), numeric_limits<double>::max()};
        // fix arcs to/from station if there is a guide and 'fix' is set
        for (auto it_t = guide!=0 && fix ? next(tour.begin(),2) : tour.begin();
                it_t!=tour.end(); ++it_t) { // TODO: bounds checking on 'next'
            size_t i=it_t==tour.begin()?tour.back():*prev(it_t);
            double cost=costs[i][r]+costs[r][*it_t]-costs[i][*it_t];
            if (cost<get<1>(best))
                best={it_t, cost};
        }
        tour.insert(get<0>(best), r);
        totcost+=get<1>(best);
    }
    return {vector<size_t>(tour.begin(), tour.end()), totcost};
}

/*
TSPSolution TSPHeuristic::randomInsertion() {
    vector<size_t> pending(costs.size());
    iota(pending.begin(), pending.end(), 0);
    shuffle(pending.begin(), pending.end(), g);
    list<size_t> tour {pending.end()[-1], pending.end()[-2]};
    pending.resize(pending.size()-2);
    double totcost=costs[tour.front()][tour.back()]
            +costs[tour.back()][tour.front()];
    while (!pending.empty()) {
        size_t r=pending.back();           // random node
        pending.pop_back();
        tuple<list<size_t>::iterator, double> best
                {tour.end(), numeric_limits<double>::max()};
        for (auto it_t=tour.begin(); it_t!=tour.end(); ++it_t) {
            size_t i=it_t==tour.begin()?tour.back():*prev(it_t);
            double cost=costs[i][r]+costs[r][*it_t]-costs[i][*it_t];
            if (cost<get<1>(best))
                best={it_t, cost};
        }
        tour.insert(get<0>(best), r);
        totcost+=get<1>(best);
    }
    return {vector<size_t>(tour.begin(), tour.end()), totcost};
}
*/

