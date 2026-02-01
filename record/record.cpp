#include <iostream>
    int main(int argc, char* argv[]) {
    std::string stream, out, time;

    for (int i = 1; i < argc; i++) {
        std::string arg = argv[i];

        if (arg == "-stream") {
            if (i + 1 < argc) stream = argv[++i];
            else { std::cerr << "Missing value for -stream\n"; return 1; }
        } 
        else if (arg == "-out") {
            if (i + 1 < argc) out = argv[++i];
            else { std::cerr << "Missing value for -out\n"; return 1; }
        } 
        else if (arg == "-time") {
            if (i + 1 < argc) time = argv[++i];
            else { std::cerr << "Missing value for -time\n"; return 1; }
        } 
        else {
            std::cerr << "Unknown argument: " << arg << "\n";
            return 1;
        }
    }

    if (stream.empty() || out.empty()) {
        std::cerr << "Usage: record -stream <URL> -out <OUTFILE> -time <SECONDS>\n";
        return 1;
    }

    std::cout << "stream = " << stream << "\n";
    std::cout << "out = " << out << "\n";
    std::cout << "time = " << time << "\n";

    int rec = system(("ffmpeg -i " + stream + " -c copy " + "-t " + time + " " + out).c_str());
}

