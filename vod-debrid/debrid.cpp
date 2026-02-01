// rewrote as old vod modules were dire
// prowlarr is used here
// add tpb indexer, use socks5 proxy

#include <iostream>
#include <fstream>
#include <cstdlib>
#include <string>
#include <cstdio> 

    int main(int argc, char* argv[]) {
    std::string query, apikey, dl;

    for (int i = 1; i < argc; i++) {
        std::string arg = argv[i];

        if (arg == "-query") {
            if (i + 1 < argc) query = argv[++i];
            else { std::cerr << "Missing value for -query\n"; return 1; }
        } 
        else if (arg == "-pr_apikey") {
            if (i + 1 < argc) pr_apikey = argv[++i];
            else { std::cerr << "Missing value for -apikey (prowlarr)\n"; return 1; }
        } 
        else if (arg == "-tb-apikey") {
            if (i + 1 < argc) tb-apikey = argv[++i];
            else { std::cerr << "Missing value for -apikey (torbox)\n"; return 1; }
        } 
        else if (arg == "-dl") {
            if (i + 1 < argc) dl = argv[++i];
            else { std::cerr << "Missing value for -dl (true/false)\n"; return 1; }
        } 
        else {
            std::cerr << "Unknown argument: " << arg << "\n";
            return 1;
        }
    }

    if (query.empty() || pr_apikey.empty() || tb_apikey.empty()) { 
        std::cerr << "Usage: debrid -query <QUERY> -tb-apikey <TORBOX API KEY> -pr_apikey <PROWLARR API KEY> -dl <TRUE/FALSE>\n";
        return 1;
    }

    std::cout << "query = " << query << "\n";
    std::cout << "pr_apikey = " << prapikey << "\n";
    std::cout << "tb-apikey = " << tbapikey << "\n";
    std::cout << "dl = " << dl << "\n";

    // Construct the command string with concatenated variables
    // We use the R"(...)" literal to keep the jq syntax readable
    std::string command = "curl -sG \"http://localhost:9696/api/v1/search\" "
                          "-H \"X-Api-Key: " + pr_apikey + "\" "
                          "--data-urlencode \"query=" + query + "\" | "
                          "jq -r --arg tr \"&tr=udp://tracker.opentrackr.org:1337/announce\" "
                          "'[.[] | select(.infoHash != null)] | sort_by(.size) | reverse | .[0] | \"magnet:?xt=urn:btih:\" + .infoHash + $tr'";

    //  Open the pipe to the shell command
    // "r" means we want to READ the output
    FILE* pipe = popen(command.c_str(), "r");
    if (!pipe) {
        std::cerr << "Failed to open pipe!" << std::endl;
        return 1;
    }

    // Read the output from the pipe into our C++ variable
    char buffer[128];
    std::string magnet_link = "";
    while (fgets(buffer, sizeof(buffer), pipe) != nullptr) {
        magnet_link += buffer;
    }

    // Close the pipe and get the exit status
    int status = pclose(pipe);

    
    if (status == 0 && !magnet_link.empty()) {
        if (!magnet_link.empty() && magnet_link.back() == '\n') {
            magnet_link.pop_back();
        }
        
        std::cout << "Magnet saved to variable: " << magnet_link << std::endl;
    } else {
        std::cout << "No magnet link was found or command failed." << std::endl;
    }

    return 0;
}

