#ifndef jsonparser_h
#define jsonparser_h

#include <string>
#include "rapidjson/document.h"

class JSONParser {
    public:
        static rapidjson::Document parse(const std::string& jsonfile);
        static void save(const rapidjson::Document& dom,
                const std::string& jsonfile);
};

#endif

