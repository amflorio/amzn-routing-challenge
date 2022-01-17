#ifndef routingpattern_h
#define routingpattern_h

#include <string>
#include <unordered_map>
#include <boost/serialization/unordered_map.hpp>

class RoutingPattern {
    friend class boost::serialization::access;
    private:
        typedef std::unordered_map<std::string, std::unordered_map<std::string,
                std::unordered_map<std::string, size_t>>> Pattern;
        Pattern macro;
        Pattern micro;
        Pattern nano;
        std::unordered_map<std::string, size_t> entriesmacro, entriesmicro,
                entriesnano;
        std::unordered_map<std::string, size_t> exitsmacro, exitsmicro,
                exitsnano;
        template<class Archive> void serialize(Archive& ar,
                const unsigned int version) {
            ar & macro;
            ar & micro;
            ar & nano;
            ar & entriesmacro;
            ar & entriesmicro;
            ar & entriesnano;
            ar & exitsmacro;
            ar & exitsmicro;
            ar & exitsnano;
        }
    public:
        void addEntryMacro(const std::string& z) {
            entriesmacro[z]++;
        }
        void addEntryMicro(const std::string& z) {
            entriesmicro[z]++;
        }
        void addEntryNano(const std::string& z) {
            entriesnano[z]++;
        }
        void addExitMacro(const std::string& z) {
            exitsmacro[z]++;
        }
        void addExitMicro(const std::string& z) {
            exitsmicro[z]++;
        }
        void addExitNano(const std::string& z) {
            exitsnano[z]++;
        }
        void addMacro(const std::string& station, const std::string& z1,
                const std::string& z2) {macro[station][z1][z2]++;}
        void addMicro(const std::string& station, const std::string& z1,
                const std::string& z2) {micro[station][z1][z2]++;}
        void addNano(const std::string& station, const std::string& z1,
                const std::string& z2) {nano[station][z1][z2]++;}
        size_t countEntriesMacro(const std::string& z) const {
            return entriesmacro.count(z)==0 ? 0 : entriesmacro.at(z);
        }
        size_t countEntriesMicro(const std::string& z) const {
            return entriesmicro.count(z)==0 ? 0 : entriesmicro.at(z);
        }
        size_t countEntriesNano(const std::string& z) const {
            return entriesnano.count(z)==0 ? 0 : entriesnano.at(z);
        }
        size_t countExitsMacro(const std::string& z) const {
            return exitsmacro.count(z)==0 ? 0 : exitsmacro.at(z);
        }
        size_t countExitsMicro(const std::string& z) const {
            return exitsmicro.count(z)==0 ? 0 : exitsmicro.at(z);
        }
        size_t countExitsNano(const std::string& z) const {
            return exitsnano.count(z)==0 ? 0 : exitsnano.at(z);
        }
        size_t countMacro(const std::string& station, const std::string& z1,
                const std::string& z2) const {
            if (macro.count(station)==1     // need this to keep `const'ness
                    && macro.at(station).count(z1)==1
                    && macro.at(station).at(z1).count(z2)==1)
                return macro.at(station).at(z1).at(z2);
            return 0;
        }
        size_t countMicro(const std::string& station, const std::string& z1,
                const std::string& z2) const {
            if (micro.count(station)==1
                    && micro.at(station).count(z1)==1
                    && micro.at(station).at(z1).count(z2)==1)
                return micro.at(station).at(z1).at(z2);
            return 0;
        }
        size_t countNano(const std::string& station, const std::string& z1,
                const std::string& z2) const {
            if (nano.count(station)==1
                    && nano.at(station).count(z1)==1
                    && nano.at(station).at(z1).count(z2)==1)
                return nano.at(station).at(z1).at(z2);
            return 0;
        }
        void print() const;
};

#endif

