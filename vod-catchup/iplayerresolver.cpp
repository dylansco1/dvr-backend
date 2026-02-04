#include <iostream>
#include <fstream>
#include <sstream>
#include <string>
#include <vector>
#include <cstdlib>
#include <array>
#include <memory>

using namespace std;

string exec_command(const string& cmd) {
    array<char, 128> buffer;
    string result;
    unique_ptr<FILE, decltype(&pclose)> pipe(popen(cmd.c_str(), "r"), pclose);
    if (!pipe) {
        cerr << "Error: Failed to run command: " << cmd << endl;
        return "";
    }
    while (fgets(buffer.data(), buffer.size(), pipe.get()) != nullptr) {
        result += buffer.data();
    }
    return result;
}

string get_m3u8_url(const string& iplayer_link) {
    if (iplayer_link.empty()) {
        return "";
    }
    string cmd = "yt-dlp --get-url '" + iplayer_link + "' 2>/dev/null";
    cout << "  Fetching m3u8 URL for: " << iplayer_link << endl;
    string output = exec_command(cmd);
    
    size_t start = output.find_first_not_of(" \t\n\r");
    size_t end = output.find_last_not_of(" \t\n\r");
    if (start == string::npos || end == string::npos) {
        return "";
    }
    string m3u8_url = output.substr(start, end - start + 1);
    
    if (m3u8_url.find("http") == 0 && m3u8_url.find(".m3u8") != string::npos) {
        return m3u8_url;
    }
    return "";
}

string extract_json_value(const string& json, const string& key) {
    string search = "\"" + key + "\":";
    size_t pos = json.find(search);
    if (pos == string::npos) {
        return "";
    }
    pos += search.length();
    
    while (pos < json.length() && (json[pos] == ' ' || json[pos] == '\t' || json[pos] == '\n')) {
        pos++;
    }
    
    if (pos >= json.length()) {
        return "";
    }
    
    if (json[pos] == '"') {
        pos++;
        size_t end = pos;
        while (end < json.length() && json[end] != '"') {
            if (json[end] == '\\') {
                end++; 
            }
            end++;
        }
        return json.substr(pos, end - pos);
    }
    
    size_t end = pos;
    while (end < json.length() && json[end] != ',' && json[end] != '}' && json[end] != ']') {
        end++;
    }
    string value = json.substr(pos, end - pos);
    
    size_t first = value.find_first_not_of(" \t\n\r");
    size_t last = value.find_last_not_of(" \t\n\r");
    if (first == string::npos) {
        return "";
    }
    return value.substr(first, last - first + 1);
}

string extract_json_object(const string& json, const string& key) {
    string search = "\"" + key + "\":";
    size_t pos = json.find(search);
    if (pos == string::npos) {
        return "";
    }
    pos += search.length();
    
    while (pos < json.length() && (json[pos] == ' ' || json[pos] == '\t' || json[pos] == '\n')) {
        pos++;
    }
    
    if (pos >= json.length() || json[pos] != '{') {
        return "";
    }
    
    int brace_count = 0;
    size_t start = pos;
    while (pos < json.length()) {
        if (json[pos] == '{') {
            brace_count++;
        } else if (json[pos] == '}') {
            brace_count--;
            if (brace_count == 0) {
                return json.substr(start, pos - start + 1);
            }
        } else if (json[pos] == '"') {
            pos++;
            while (pos < json.length() && json[pos] != '"') {
                if (json[pos] == '\\') {
                    pos++;
                }
                pos++;
            }
        }
        pos++;
    }
    return "";
}

string extract_first_array_element(const string& json, const string& key) {
    string search = "\"" + key + "\":";
    size_t pos = json.find(search);
    if (pos == string::npos) {
        return "";
    }
    pos += search.length();
    
    while (pos < json.length() && (json[pos] == ' ' || json[pos] == '\t' || json[pos] == '\n')) {
        pos++;
    }
    
    if (pos >= json.length() || json[pos] != '[') {
        return "";
    }
    pos++;
    
    while (pos < json.length() && (json[pos] == ' ' || json[pos] == '\t' || json[pos] == '\n')) {
        pos++;
    }
    
    if (pos >= json.length() || json[pos] != '{') {
        return "";
    }
    
    int brace_count = 0;
    size_t start = pos;
    while (pos < json.length()) {
        if (json[pos] == '{') {
            brace_count++;
        } else if (json[pos] == '}') {
            brace_count--;
            if (brace_count == 0) {
                return json.substr(start, pos - start + 1);
            }
        } else if (json[pos] == '"') {
            pos++;
            while (pos < json.length() && json[pos] != '"') {
                if (json[pos] == '\\') {
                    pos++;
                }
                pos++;
            }
        }
        pos++;
    }
    return "";
}

string parse_duration(const string& iso_duration) {
    if (iso_duration.empty()) {
        return "";
    }
    
    size_t m_pos = iso_duration.find('M');
    if (m_pos != string::npos && m_pos > 2) {
        string minutes_str = iso_duration.substr(2, m_pos - 2);
        return minutes_str + " minutes";
    }
    return "";
}

string extract_remaining_time(const string& remaining_text) {
    if (remaining_text.empty()) {
        return "";
    }
    
    size_t for_pos = remaining_text.find("for");
    if (for_pos != string::npos) {
        string result = remaining_text.substr(for_pos + 3);
        size_t start = result.find_first_not_of(" \t");
        if (start != string::npos) {
            return result.substr(start);
        }
    }
    return remaining_text;
}

string xml_escape(const string& text) {
    string result;
    result.reserve(text.length());
    for (char c : text) {
        switch (c) {
            case '&':  result += "&amp;";  break;
            case '<':  result += "&lt;";   break;
            case '>':  result += "&gt;";   break;
            case '"':  result += "&quot;"; break;
            case '\'': result += "&apos;"; break;
            default:   result += c;        break;
        }
    }
    return result;
}

string convert_episode_to_xml(const string& episode_json, bool fetch_m3u8) {
    ostringstream xml;
    
    string version = extract_first_array_element(episode_json, "versions");
    
    string availability = extract_json_object(version, "availability");
    string duration_obj = extract_json_object(version, "duration");
    string synopses = extract_json_object(episode_json, "synopses");
    string remaining_obj = extract_json_object(availability, "remaining");
    
    string subtitle = extract_json_value(episode_json, "subtitle");
    string title = extract_json_value(episode_json, "title");
    string desc = extract_json_value(synopses, "medium");
    if (desc.empty()) {
        desc = extract_json_value(synopses, "small");
    }
    string duration_value = extract_json_value(duration_obj, "value");
    string first_broadcast = extract_json_value(version, "first_broadcast");
    string remaining_text = extract_json_value(remaining_obj, "text");
    string episode_id = extract_json_value(episode_json, "id");
    
    string duration = parse_duration(duration_value);
    string remaining = extract_remaining_time(remaining_text);
    string iplayer_link = "";
    if (!episode_id.empty()) {
        iplayer_link = "https://www.bbc.co.uk/programmes/" + episode_id;
    }
    
    string m3u8_url = "";
    if (fetch_m3u8 && !iplayer_link.empty()) {
        m3u8_url = get_m3u8_url(iplayer_link);
        if (m3u8_url.empty()) {
            cout << "  Warning: Could not fetch m3u8 URL" << endl;
        }
    }
    
    xml << "<nvsc>\n";
    xml << "\t<moviename></moviename>\n";
    xml << "\t<episode>" << xml_escape(subtitle) << "</episode>\n";
    xml << "\t<seriesname>" << xml_escape(title) << "</seriesname>\n";
    xml << "\t<desc>" << xml_escape(desc) << "</desc>\n";
    xml << "\t<duration>" << duration << "</duration>\n";
    xml << "\t<firstbroadcast>" << xml_escape(first_broadcast) << "</firstbroadcast>\n";
    xml << "\t<remainingtime>" << remaining << "</remainingtime>\n";
    xml << "\t<iplayerlink>" << iplayer_link << "</iplayerlink>\n";
    xml << "\t<m3u8url>" << xml_escape(m3u8_url) << "</m3u8url>\n";
    xml << "</nvsc>\n";
    
    return xml.str();
}

vector<string> extract_episodes(const string& json_content) {
    vector<string> episodes;
    
    string search = "\"elements\":";
    size_t pos = json_content.find(search);
    if (pos == string::npos) {
        return episodes;
    }
    pos += search.length();
    
    while (pos < json_content.length() && json_content[pos] != '[') {
        pos++;
    }
    pos++; 
    
    while (pos < json_content.length()) {
        while (pos < json_content.length() && 
               (json_content[pos] == ' ' || json_content[pos] == '\t' || 
                json_content[pos] == '\n' || json_content[pos] == '\r')) {
            pos++;
        }
        
        if (pos >= json_content.length() || json_content[pos] == ']') {
            break;
        }
        
        if (json_content[pos] == '{') {
            int brace_count = 0;
            size_t start = pos;
            while (pos < json_content.length()) {
                if (json_content[pos] == '{') {
                    brace_count++;
                } else if (json_content[pos] == '}') {
                    brace_count--;
                    if (brace_count == 0) {
                        episodes.push_back(json_content.substr(start, pos - start + 1));
                        pos++;
                        break;
                    }
                } else if (json_content[pos] == '"') {
                    pos++;
                    while (pos < json_content.length() && json_content[pos] != '"') {
                        if (json_content[pos] == '\\') {
                            pos++;
                        }
                        pos++;
                    }
                }
                pos++;
            }
        }
        
        if (pos < json_content.length() && json_content[pos] == ',') {
            pos++;
        }
    }
    
    return episodes;
}

void convert_json_to_xml(const string& json_file, const string& output_file = "", bool fetch_m3u8 = false) {
    ifstream file(json_file);
    if (!file.is_open()) {
        cerr << "Error: Could not open file " << json_file << endl;
        return;
    }

    stringstream buffer;
    buffer << file.rdbuf();
    string json_content = buffer.str();
    file.close();

    vector<string> episodes = extract_episodes(json_content);

    if (episodes.empty()) {
        cout << "No episodes found in JSON" << endl;
        return;
    }

    cout << "Found " << episodes.size() << " episode(s)" << endl;

    if (fetch_m3u8) {
        cout << "Fetching m3u8 URLs (this may take a while)..." << endl;
    }

    cout << endl;

    string output_dir = "";
    if (!output_file.empty() && episodes.size() > 1) {
        size_t slash_pos = output_file.find_last_of("/\\");
        if (slash_pos != string::npos) {
            output_dir = output_file.substr(0, slash_pos + 1);
        }
    }

    for (size_t i = 0; i < episodes.size(); i++) {
        cout << "Processing episode " << (i + 1) << "/" << episodes.size() << "..." << endl;

        string xml_string = convert_episode_to_xml(episodes[i], fetch_m3u8);

        string filename;
        
        if (!output_file.empty() && episodes.size() == 1) {
            filename = output_file;
        } else {
            
            string series_name = extract_json_value(episodes[i], "title");
            string episode_name = extract_json_value(episodes[i], "subtitle");

            string safe_series = series_name;
            for (char& c : safe_series) {
                if (c == ' ') c = '_';
                else if (c == '/' || c == '\\' || c == ':' || c == '*' ||
                    c == '?' || c == '"' || c == '<' || c == '>' || c == '|') {
                    c = '_';
                }
            }

            string safe_episode = episode_name;
            for (char& c : safe_episode) {
                if (c == ' ') c = '_';
                else if (c == '/' || c == '\\' || c == ':' || c == '*' ||
                    c == '?' || c == '"' || c == '<' || c == '>' || c == '|') {
                    c = '_';
                }
            }

            string base_dir;
            if (!output_dir.empty()) {
                base_dir = output_dir;
            } else {
                base_dir = "/opt/catchup/iplayer/";
            }

            if (!safe_series.empty() && !safe_episode.empty()) {
                filename = base_dir + safe_series + "_" + safe_episode + ".nvsc";
            } else if (!safe_series.empty()) {
                filename = base_dir + safe_series + "_" + to_string(i + 1) + ".nvsc";
            } else {
                string episode_id = extract_json_value(episodes[i], "id");
                if (episode_id.empty()) {
                    episode_id = "episode_" + to_string(i + 1);
                }
                filename = base_dir + episode_id + ".nvsc";
            }
        }

        ofstream out(filename);
        if (!out.is_open()) {
            cerr << "Error: Could not create output file " << filename << endl;
            continue;
        }

        out << xml_string;
        out.close();

        cout << "Created: " << filename << endl;
        cout << endl;
    }

    cout << "Successfully converted " << episodes.size() << " episode(s)" << endl;
}

void print_usage(const char* program_name) {
    cout << "BBC iPlayer JSON to XML Converter with yt-dlp Integration" << endl;
    cout << endl;
    cout << "Usage: " << program_name << " <json_file> [options]" << endl;
    cout << endl;
    cout << "Options:" << endl;
    cout << "  -o <output_file>    Specify output XML file" << endl;
    cout << "  -m, --fetch-m3u8    Fetch m3u8 URLs using yt-dlp" << endl;
    cout << "  -h, --help          Show this help message" << endl;
    cout << endl;
    cout << "Examples:" << endl;
    cout << "  " << program_name << " data.json" << endl;
    cout << "  " << program_name << " data.json -o output.xml" << endl;
    cout << "  " << program_name << " data.json -m" << endl;
    cout << "  " << program_name << " data.json -o output.xml -m" << endl;
    cout << endl;
    cout << "Note: The -m option requires yt-dlp to be installed and accessible in PATH" << endl;
}

int main(int argc, char* argv[]) {
    if (argc < 2) {
        print_usage(argv[0]);
        return 1;
    }

    // json_file meaning url
    string json_file = "";
    string output_file = "";
    bool fetch_m3u8 = false;

    for (int i = 1; i < argc; i++) {
        string arg = argv[i];
        if (arg == "-h" || arg == "--help") {
            print_usage(argv[0]);
            return 0;
        } else if (arg == "-m" || arg == "--fetch-m3u8") {
            fetch_m3u8 = true;
        } else if (arg == "-o") {
            if (i + 1 < argc) {
                output_file = argv[++i];
            } else {
                cerr << "Error: -o option requires an argument" << endl;
                return 1;
            }
        } else if (json_file.empty()) {
            json_file = arg;
        } else {
            cerr << "Error: Unknown argument '" << arg << "'" << endl;
            print_usage(argv[0]);
            return 1;
        }
    }

    if (json_file.empty()) {
        cerr << "Error: No JSON file specified" << endl;
        print_usage(argv[0]);
        return 1;
    }

    string cmd = "curl -s \"" + json_file + "\"";
    string output = exec_command(cmd);

    if (output.empty()) {
        cerr << "Error: curl returned empty output" << endl;
        return 1;
    }

    string temp_file = "/tmp/iplayer.json";
    ofstream tmp(temp_file);
    tmp << output;
    tmp.close();

    convert_json_to_xml(temp_file, output_file, fetch_m3u8);

    return 0;
}