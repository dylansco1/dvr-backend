// rewrote as old vod modules were dire
// prowlarr is used here
// add knaben

#include <iostream>
#include <fstream>
#include <cstdlib>
#include <cstdio> 
#include <unistd.h>
#include <ctime>
#include <sys/wait.h>
#include <string>
#include <vector>

    static std::string shellEscape(const std::string& s)
    {
        std::string out = "'";
        for (char c : s)
        {
            if (c == '\'')
                out += "'\\''";   // end quote, escape quote, reopen
            else
                out += c;
        }
        out += "'";
        return out;
    }


    int main(int argc, char* argv[]) {
    std::string query, prapikey, tbapikey, tv, episode;

    for (int i = 1; i < argc; i++) {
        std::string arg = argv[i];

        if (arg == "-query") {
            if (i + 1 < argc) query = argv[++i];
            else { std::cerr << "Missing value for -query\n"; return 1; }
        }
        else if (arg == "-prapikey") {
            if (i + 1 < argc) prapikey = argv[++i];
            else { std::cerr << "Missing value for -prapikey\n"; return 1; }
        }
        else if (arg == "-tbapikey") {
            if (i + 1 < argc) tbapikey = argv[++i];
            else { std::cerr << "Missing value for -tbapikey\n"; return 1; }
        }
        else if (arg == "-tv") {
            if (i + 1 < argc) tv = argv[++i];
            else { std::cerr << "Missing value for -tv\n"; return 1; }
        }
        else if (arg == "-episode") {
            if (i + 1 < argc) episode = argv[++i];
            else { std::cerr << "Missing value for -episode\n"; return 1; }
        }
        else {
            std::cerr << "Unknown argument: " << arg << "\n";
            return 1;
        }
    }

    if (query.empty() || prapikey.empty() || tbapikey.empty() || tv.empty()) { 
        std::cerr << "Usage: debrid -query <QUERY> -tbapikey <TORBOX API KEY> -prapikey <PROWLARR API KEY> -tv <TRUE/FALSE> -episode (not required, has to look like S01E01)\n";
        return 1;
    }

    std::cout << "query = " << query << "\n";
    std::cout << "prapikey = " << prapikey << "\n";
    std::cout << "tbapikey = " << tbapikey << "\n";
    std:: cout << "tv = " << tv << "\n";

    std::string command;

    if (tv == "false") {
        command =
            "curl -sG \"http://localhost:9696/api/v1/search\" "
            "-H \"X-Api-Key: " + prapikey + "\" "
            "--data-urlencode \"query=" + query + "\" | "
            "jq -r --arg tr \"&tr=udp://tracker.opentrackr.org:1337/announce\" "
            "'[.[] | select(.infoHash != null)] "
            "| sort_by(.size) "
            "| reverse "
            "| .[0] "
            "| \"magnet:?xt=urn:btih:\" + .infoHash + $tr'";
    }
    else if (tv == "true") {
        command =
            "curl -sG \"http://localhost:9696/api/v1/search\" "
            "-H \"X-Api-Key: " + prapikey + "\" "
            "--data-urlencode \"query=" + query + " complete\" | "
            "jq -r --arg tr \"&tr=udp://tracker.opentrackr.org:1337/announce\" "
            "'[.[] | select(.infoHash != null)] "
            "| sort_by(.size) "
            "| reverse "
            "| .[0] "
            "| \"magnet:?xt=urn:btih:\" + .infoHash + $tr'";
    }
    else {
        std::cerr << "-tv must be true or false\n";
        return 1;
    }

    FILE* pipe = popen(command.c_str(), "r");
    if (!pipe) {
        std::cerr << "Failed to open pipe!" << std::endl;
        return 1;
    }

    char buffer[128];
    std::string magnet_link = "";
    while (fgets(buffer, sizeof(buffer), pipe) != nullptr) {
        magnet_link += buffer;
    }

    int status = pclose(pipe);

    
    if (status == 0 && !magnet_link.empty()) {
        if (!magnet_link.empty() && magnet_link.back() == '\n') {
            magnet_link.pop_back();
        }
        
        std::cout << "Magnet saved to variable: " << magnet_link << std::endl;
    } else {
        std::cout << "No magnet link was found or command failed." << std::endl;
    }


    std::vector<char*> args;

    args.push_back((char*)"curl");
    args.push_back((char*)"-s");
    args.push_back((char*)"-X");
    args.push_back((char*)"POST");
    args.push_back((char*)"--output");
    args.push_back((char*)"/tmp/torbox_post.json");
    args.push_back((char*)"https://api.torbox.app/v1/api/torrents/createtorrent");

    std::string auth = "Authorization: Bearer " + tbapikey;
    args.push_back((char*)"-H");
    args.push_back((char*)auth.c_str());

    std::string form = "magnet=" + magnet_link;
    args.push_back((char*)"-F");
    args.push_back((char*)form.c_str());

    args.push_back(nullptr);

    pid_t pid = fork();
    if (pid == 0) {
        execvp("curl", args.data());
        perror("execvp failed");
        _exit(1);
    } else {
        int wstatus;
        waitpid(pid, &wstatus, 0);
    }

    std::string torrent_id;
    FILE* idpipe = popen("jq -r '.data.torrent_id' /tmp/torbox_post.json", "r");
    char idbuf[128];
    if (fgets(idbuf, sizeof(idbuf), idpipe))
        torrent_id = idbuf;
    pclose(idpipe);

    if (!torrent_id.empty() && torrent_id.back() == '\n')
        torrent_id.pop_back();

    if (torrent_id.empty()) {
        std::cerr << "Failed to get torrent_id\n";
        return 1;
    }

    std::string file_id;

    if (!episode.empty()) {

        std::string mylist_cmd =
            "curl -s \"https://api.torbox.app/v1/api/torrents/mylist?id=" + torrent_id + "\" "
            "-H \"Authorization: Bearer " + tbapikey + "\" > /tmp/torbox_mylist.json";

        FILE* listpipe = popen(mylist_cmd.c_str(), "r");
        if (listpipe) pclose(listpipe);

        std::string jq_fileid =
            "jq -r '.data.files[] "
            "| select(.short_name | test(\"" + episode + "\"; \"i\")) "
            "| select(.mimetype | test(\"video\")) "
            "| .id' /tmp/torbox_mylist.json | head -n 1";

        FILE* fidpipe = popen(jq_fileid.c_str(), "r");

        char fbuf[128];
        if (fgets(fbuf, sizeof(fbuf), fidpipe))
            file_id = fbuf;

        pclose(fidpipe);

        if (!file_id.empty() && file_id.back() == '\n')
            file_id.pop_back();

        if (file_id.empty()) {
            std::cerr << "No matching episode file found\n";
            return 1;
        }

        std::cout << "Selected file_id: " << file_id << "\n";
    }

    std::string requestdl_cmd =
        "curl -s \"https://api.torbox.app/v1/api/torrents/requestdl"
        "?token=" + tbapikey +
        "&torrent_id=" + torrent_id;

    if (!file_id.empty())
        requestdl_cmd += "&file_id=" + file_id;

    requestdl_cmd += "\" > /tmp/torbox_requestdl.json";

    FILE* reqpipe = popen(requestdl_cmd.c_str(), "r");
    if (reqpipe) pclose(reqpipe);

    std::string media_link;
    FILE* dlpipe = popen("jq -r '.data' /tmp/torbox_requestdl.json", "r");

    char dlbuf[512];
    if (fgets(dlbuf, sizeof(dlbuf), dlpipe))
        media_link = dlbuf;

    pclose(dlpipe);

    if (!media_link.empty() && media_link.back() == '\n')
        media_link.pop_back();

    if (media_link.empty()) {
        std::cerr << "No download link returned\n";
        return 1;
    }

    std::cout << "Final media link: " << media_link << "\n";

    std::time_t timestamp = std::time(nullptr);
    std::cout << "Unix timestamp: " << timestamp << std::endl;

    std::ofstream file("debrid" + std::to_string(timestamp) + ".txt");

    if (!file.is_open()) {
        std::cerr << "Failed to open file\n";
        return 1;
    }

    file << media_link;

    file.close();
    return 0;
}