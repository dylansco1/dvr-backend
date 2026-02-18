#include <iostream>
#include <fstream>
#include <sstream>
#include <string>
#include <regex>
#include <cstdlib>

int main(int argc, char* argv[])
{
    if (argc != 2) {
        std::cerr << "Usage: " << argv[0] << " <URL>\n";
        return 1;
    }

    const char* url = argv[1];

    std::system("bash -lc \"rm -rf /opt/dvrp/gbntemp\"");
    std::system("bash -lc \"mkdir -p /opt/dvrp/gbntemp\"");
    std::system((std::string("bash -lc \"curl -o /opt/dvrp/gbntemp/temphtm ") + url + "\"").c_str());

    std::string filePath = "/opt/dvrp/gbntemp/temphtm";

    std::ifstream file(filePath);
    if (!file.is_open()) {
        std::cerr << "Failed to open file! errno = " << errno << std::endl;
        return 1;
    }

    std::stringstream buffer;
    buffer << file.rdbuf();
    std::string html = buffer.str();

    std::regex scriptRe(R"(<script type="application/ld\+json">([\s\S]+?)</script>)");
    std::smatch match;

    if (!std::regex_search(html, match, scriptRe)) {
        std::cerr << "No <script type=\"application/ld+json\"> found.\n";
        return 1;
    }

    std::string jsonStr = match[1].str();

    std::regex uvidRe(R"(uvid%3D(\d+))");
    std::smatch uvidMatch;
    if (std::regex_search(jsonStr, uvidMatch, uvidRe)) {
        std::cout << "Found uvid: " << uvidMatch[1].str() << std::endl;
    } else {
        std::cout << "uvid not found in JSON." << std::endl;
    }

    std::regex dateRe(R"DELIM("url"\s*:\s*"https://www\.gbnews\.com/shows/[^/]+/([0-9]{4}-[0-9]{2}-[0-9]{2})")DELIM");
    std::smatch dateMatch;
    if (std::regex_search(jsonStr, dateMatch, dateRe)) {
        std::cout << "Found date: " << dateMatch[1].str() << std::endl;
    } else {
        std::cout << "Date not found in URL." << std::endl;
    }

    std::regex descRe(R"DELIM("description"\s*:\s*"([^"]*)")DELIM");
    std::smatch descMatch;

    if (std::regex_search(jsonStr, descMatch, descRe)) {
        std::cout << "Found description: " << descMatch[1].str() << std::endl;
    } else {
        std::cout << "Description not found." << std::endl;
    }

    std::regex nameRe(R"DELIM("name"\s*:\s*"([^"]*)")DELIM");
    std::smatch nameMatch;

    if (std::regex_search(jsonStr, nameMatch, nameRe)) {
        std::cout << "Found name: " << nameMatch[1].str() << std::endl;
    } else {
        std::cout << "Name not found." << std::endl;
    }

    std::string finalurl = "http://vod-gbnews-fastly.simplestreamcdn.com/gbnews/capture/gbnews/" + dateMatch[1].str() + "/" + uvidMatch[1].str() + ".ism/" + uvidMatch[1].str() + "-audio=192000-video=4500000.m3u8";
    std::cout << finalurl;

    return 0;
}
