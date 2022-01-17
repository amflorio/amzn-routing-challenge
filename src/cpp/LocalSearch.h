#ifndef localsearch_h
#define localsearch_h

class Route;
class Sequence;
class LocalSearch {
    public:
        static bool myOpt(Sequence& seq, const Route& r);
};

#endif

