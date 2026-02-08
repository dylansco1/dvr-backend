#include <mpv/client.h>
#include <iostream>
#include <cstring>
#include <iomanip>

class MPVPlayer {
private:
    mpv_handle* mpv;

public:
    MPVPlayer() {
        mpv = mpv_create();
        if (!mpv) {
            throw std::runtime_error("Failed to create MPV instance");
        }

        // Better options for streaming
        mpv_set_option_string(mpv, "vo", "gpu");
        mpv_set_option_string(mpv, "keep-open", "yes");
        mpv_set_option_string(mpv, "terminal", "yes");
        mpv_set_option_string(mpv, "msg-level", "all=status");
        
        // Network/streaming options
        mpv_set_option_string(mpv, "cache", "yes");
        mpv_set_option_string(mpv, "demuxer-max-bytes", "512M");
        mpv_set_option_string(mpv, "demuxer-max-back-bytes", "256M");
        
        if (mpv_initialize(mpv) < 0) {
            throw std::runtime_error("Failed to initialize MPV");
        }
    }

    ~MPVPlayer() {
        if (mpv) {
            mpv_terminate_destroy(mpv);
        }
    }

    void loadFile(const char* filename) {
        const char* cmd[] = {"loadfile", filename, nullptr};
        mpv_command(mpv, cmd);
    }

    void waitForEvent() {
        std::cout << "Starting playback...\n" << std::endl;
        
        while (true) {
            mpv_event* event = mpv_wait_event(mpv, 1.0);
            
            switch (event->event_id) {
                case MPV_EVENT_NONE:
                    break;
                    
                case MPV_EVENT_START_FILE:
                    std::cout << "▶ Loading file..." << std::endl;
                    break;
                    
                case MPV_EVENT_FILE_LOADED:
                    std::cout << "✓ File loaded successfully" << std::endl;
                    printInfo();
                    break;
                    
                case MPV_EVENT_PLAYBACK_RESTART:
                    std::cout << "▶ Playback started" << std::endl;
                    break;
                    
                case MPV_EVENT_END_FILE: {
                    mpv_event_end_file* ef = (mpv_event_end_file*)event->data;
                    if (ef->reason == MPV_END_FILE_REASON_EOF) {
                        std::cout << "\n■ Playback finished" << std::endl;
                    } else if (ef->reason == MPV_END_FILE_REASON_ERROR) {
                        std::cout << "\n✗ Playback error: " << ef->error << std::endl;
                    }
                    return;
                }
                
                case MPV_EVENT_SHUTDOWN:
                    std::cout << "\n■ Shutdown" << std::endl;
                    return;
                    
                case MPV_EVENT_LOG_MESSAGE: {
                    mpv_event_log_message* msg = (mpv_event_log_message*)event->data;
                    std::cout << "[" << msg->prefix << "] " << msg->text;
                    break;
                }
                
                case MPV_EVENT_PROPERTY_CHANGE: {
                    mpv_event_property* prop = (mpv_event_property*)event->data;
                    if (strcmp(prop->name, "time-pos") == 0 && prop->format == MPV_FORMAT_DOUBLE) {
                        double time = *(double*)prop->data;
                        std::cout << "\r⏱  Position: " << formatTime(time) << std::flush;
                    }
                    break;
                }
                
                default:
                    // Uncomment to see all events:
                    // std::cout << "Event: " << mpv_event_name(event->event_id) << std::endl;
                    break;
            }
        }
    }

    void printInfo() {
        std::cout << "\nStream Information:" << std::endl;
        std::cout << "===================" << std::endl;
        
        // Get media title
        char* title = nullptr;
        mpv_get_property(mpv, "media-title", MPV_FORMAT_STRING, &title);
        if (title) {
            std::cout << "Title: " << title << std::endl;
            mpv_free(title);
        }
        
        // Get duration
        double duration = 0;
        if (mpv_get_property(mpv, "duration", MPV_FORMAT_DOUBLE, &duration) >= 0) {
            std::cout << "Duration: " << formatTime(duration) << std::endl;
        } else {
            std::cout << "Duration: Live stream" << std::endl;
        }
        
        // Get video format
        char* vformat = nullptr;
        mpv_get_property(mpv, "video-format", MPV_FORMAT_STRING, &vformat);
        if (vformat) {
            std::cout << "Video Format: " << vformat << std::endl;
            mpv_free(vformat);
        }
        
        // Get dimensions
        int64_t width = 0, height = 0;
        mpv_get_property(mpv, "width", MPV_FORMAT_INT64, &width);
        mpv_get_property(mpv, "height", MPV_FORMAT_INT64, &height);
        if (width && height) {
            std::cout << "Resolution: " << width << "x" << height << std::endl;
        }
        
        // Get audio codec
        char* acodec = nullptr;
        mpv_get_property(mpv, "audio-codec-name", MPV_FORMAT_STRING, &acodec);
        if (acodec) {
            std::cout << "Audio Codec: " << acodec << std::endl;
            mpv_free(acodec);
        }
        
        std::cout << "===================" << std::endl;
        std::cout << "\nPress Ctrl+C to stop\n" << std::endl;
    }

    std::string formatTime(double seconds) {
        int hours = (int)seconds / 3600;
        int minutes = ((int)seconds % 3600) / 60;
        int secs = (int)seconds % 60;
        
        char buffer[32];
        if (hours > 0) {
            snprintf(buffer, sizeof(buffer), "%02d:%02d:%02d", hours, minutes, secs);
        } else {
            snprintf(buffer, sizeof(buffer), "%02d:%02d", minutes, secs);
        }
        return std::string(buffer);
    }

    void observeProperty(const char* name) {
        mpv_observe_property(mpv, 0, name, MPV_FORMAT_DOUBLE);
    }
};

int main(int argc, char* argv[]) {
    if (argc < 2) {
        std::cout << "Usage: " << argv[0] << " <url-or-file>" << std::endl;
        std::cout << "\nExamples:" << std::endl;
        std::cout << "  " << argv[0] << " video.mp4" << std::endl;
        std::cout << "  " << argv[0] << " http://example.com/stream.m3u8" << std::endl;
        return 1;
    }

    try {
        MPVPlayer player;
        player.loadFile(argv[1]);
        player.observeProperty("time-pos");
        player.waitForEvent();
    } catch (const std::exception& e) {
        std::cerr << "Error: " << e.what() << std::endl;
        return 1;
    }

    return 0;
}