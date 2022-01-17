#include "rapidjson/error/en.h"
#include "rapidjson/filereadstream.h"
#include "rapidjson/filewritestream.h"
#include "rapidjson/writer.h"
#include "JSONParser.h"

using namespace std;
using namespace rapidjson;

Document JSONParser::parse(const string& jsonfile) {
    FILE *fp=fopen(jsonfile.c_str(), "r");
    char buffer[65536];
    FileReadStream is(fp, buffer, sizeof(buffer));
    Document dom;
    ParseResult ok=dom.ParseStream<kParseNanAndInfFlag>(is);
    if (!ok) {
        fprintf(stderr, "JSON parse error: %s (%u)",
                GetParseError_En(ok.Code()),
                static_cast<unsigned int>(ok.Offset()));
        exit(EXIT_FAILURE);
    }
    fclose(fp);
    return dom;
}

void JSONParser::save(const Document& dom, const string& jsonfile) {
    FILE* fp=fopen(jsonfile.c_str(), "w");
    char buffer[65536];
    FileWriteStream os(fp, buffer, sizeof(buffer));
    Writer<FileWriteStream, UTF8<>, UTF8<>, CrtAllocator, kWriteNanAndInfFlag>
            writer(os);
    dom.Accept(writer);
    fclose(fp);
}

