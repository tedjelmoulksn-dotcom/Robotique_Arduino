#include <iostream>
#include <thread>
#include <string>
#include <chrono>
#include <ctime>
#include <iomanip>
#include <vector>
#include <fstream>
#include <sstream>
#include <memory>

#include <camera/camera.h>
#include <camera/device_discovery.h>
#include <camera/photography_settings.h>

#ifdef _WIN32
#include <io.h>
#include <sys/stat.h>
#define ACCESS_FUNC _access
#define F_OK 0
#define STAT_FUNC _stat
#else
#include <unistd.h>
#include <time.h>
#include <sys/stat.h>
#define ACCESS_FUNC access
#define STAT_FUNC stat
#endif

std::string formatWallClockNow() {
    auto now = std::chrono::system_clock::now();
    auto tt = std::chrono::system_clock::to_time_t(now);

    std::tm tm{};
#ifdef _WIN32
    localtime_s(&tm, &tt);
#else
    localtime_r(&tt, &tm);
#endif

    auto ms = std::chrono::duration_cast<std::chrono::milliseconds>(
        now.time_since_epoch()) % 1000;

    std::ostringstream oss;
    oss << std::put_time(&tm, "%Y-%m-%d %H:%M:%S")
        << "." << std::setw(3) << std::setfill('0') << ms.count();
    return oss.str();
}

std::string getCurrentTime() {
    const auto now = std::chrono::system_clock::now();
    const std::time_t t = std::chrono::system_clock::to_time_t(now);
    std::tm tm{};
#ifdef _WIN32
    localtime_s(&tm, &t);
#else
    localtime_r(&t, &tm);
#endif
    char buffer[80];
    std::strftime(buffer, sizeof(buffer), "%Y%m%d_%H%M%S", &tm);
    return std::string(buffer);
}

bool fileExists(const std::string& file_path) {
    return ACCESS_FUNC(file_path.c_str(), F_OK) == 0;
}

std::string getFileName(const std::string& path) {
    size_t lastSlash = path.find_last_of("/\\");
    if (lastSlash == std::string::npos) {
        return path;
    }
    return path.substr(lastSlash + 1);
}

int64_t getFileSize(const std::string& file_path) {
    struct STAT_FUNC stat_buf;
    if (STAT_FUNC(file_path.c_str(), &stat_buf) == 0) {
        return stat_buf.st_size;
    }
    return -1;
}

std::string formatBytes(int64_t bytes) {
    const int64_t GB = 1024LL * 1024LL * 1024LL;
    const int64_t MB = 1024LL * 1024LL;
    const int64_t KB = 1024LL;

    if (bytes >= GB) {
        double gb = static_cast<double>(bytes) / GB;
        char buffer[32];
        snprintf(buffer, sizeof(buffer), "%.2f GB", gb);
        return std::string(buffer);
    } else if (bytes >= MB) {
        double mb = static_cast<double>(bytes) / MB;
        char buffer[32];
        snprintf(buffer, sizeof(buffer), "%.2f MB", mb);
        return std::string(buffer);
    } else if (bytes >= KB) {
        double kb = static_cast<double>(bytes) / KB;
        char buffer[32];
        snprintf(buffer, sizeof(buffer), "%.2f KB", kb);
        return std::string(buffer);
    } else {
        return std::to_string(bytes) + " bytes";
    }
}

struct SequenceOptions {
    int nb_images = 0;
    std::chrono::milliseconds total_duration{0};
    std::string save_directory = "./";
    bool best_quality = false;
};

struct SequenceSummary {
    std::string wall_operation_start;
    std::string wall_capture_start;
    std::string wall_capture_end;
    std::string wall_download_start;
    std::string wall_download_end;
    std::string wall_operation_end;

    long long capture_phase_duration_ms = -1;
    long long download_phase_duration_ms = -1;
    long long total_operation_duration_ms = -1;

    int planned_images = 0;
    int captured_images = 0;
    int downloaded_images = 0;
};

struct ShotTiming {
    int index = 0;

    // horodatages lisibles
    std::string wall_command_sent;
    std::string wall_sdk_return;
    std::string wall_file_visible;
    std::string wall_download_start;
    std::string wall_download_end;

    // temps monotones pour calculs
    std::chrono::steady_clock::time_point t_command;
    std::chrono::steady_clock::time_point t_takephoto_return;
    std::chrono::steady_clock::time_point t_file_visible;
    std::chrono::steady_clock::time_point t_download_start;
    std::chrono::steady_clock::time_point t_download_end;

    std::string photo_url;
    std::string local_path;

    bool takephoto_success = false;
    bool file_visible_success = false;
    bool download_success = false;

    bool sdk_trigger_available = false;
    long long sdk_trigger_ms = -1;

    int64_t downloaded_size = 0;

    long long command_to_return_ms = -1;
    long long command_to_visible_ms = -1;
    long long command_to_local_saved_ms = -1;
    long long download_duration_ms = -1;
};

class CameraController {
private:
    std::shared_ptr<ins_camera::Camera> camera_;
    bool is_connected_;

private:
    bool preparePhotoSequence(bool best_quality) {
        if (!is_connected_ || !camera_) {
            std::cerr << "Error: Camera not connected." << std::endl;
            return false;
        }

        if (!camera_->IsConnected()) {
            std::cerr << "Error: Camera connection lost." << std::endl;
            is_connected_ = false;
            return false;
        }

        ins_camera::StorageStatus storage{};
        if (camera_->GetStorageState(storage)) {
            if (storage.state != ins_camera::STOR_CS_PASS) {
                std::cerr << "Error: Camera storage not ready. State = " << storage.state << std::endl;
                return false;
            }

            std::cout << "Storage OK. Free space: " << formatBytes(storage.free_space)
                      << " / Total: " << formatBytes(storage.total_space) << std::endl;
        } else {
            std::cerr << "Warning: Could not verify storage state." << std::endl;
        }

        std::cout << "Setting photo mode to PHOTO_SINGLE..." << std::endl;
        if (!camera_->SetPhotoSubMode(ins_camera::SubPhotoMode::PHOTO_SINGLE)) {
            std::cerr << "Error: Failed to set PHOTO_SINGLE mode." << std::endl;
            return false;
        }

        ins_camera::PhotoSize photo_size =
            best_quality
                ? ins_camera::PhotoSize::Size_11968_5984
                : ins_camera::PhotoSize::Size_5952_2976;

        std::cout << "Setting photo size to "
                  << (best_quality ? "maximum quality" : "fast mode")
                  << "..." << std::endl;

        if (!camera_->SetPhotoSize(ins_camera::CameraFunctionMode::FUNCTION_MODE_NORMAL_IMAGE, photo_size)) {
            std::cerr << "Error: Failed to set photo size." << std::endl;
            return false;
        }

        std::this_thread::sleep_for(std::chrono::milliseconds(200));
        return true;
    }

    bool waitUntilFileVisible(const std::string& expected_url,
                              std::chrono::milliseconds timeout,
                              std::chrono::milliseconds poll_period,
                              std::chrono::steady_clock::time_point& visible_time) {
        auto start = std::chrono::steady_clock::now();

        while (std::chrono::steady_clock::now() - start < timeout) {
            auto files = camera_->GetCameraFilesList();

            for (const auto& f : files) {
                if (f == expected_url) {
                    visible_time = std::chrono::steady_clock::now();
                    return true;
                }
            }

            std::this_thread::sleep_for(poll_period);
        }

        return false;
    }

    ShotTiming captureOnePhotoNoDownload(int index) {
        ShotTiming shot;
        shot.index = index;

        if (!is_connected_ || !camera_ || !camera_->IsConnected()) {
            std::cerr << "Error: Camera not connected during sequence." << std::endl;
            is_connected_ = false;
            return shot;
        }

        shot.wall_command_sent = formatWallClockNow();
        shot.t_command = std::chrono::steady_clock::now();

        const auto url = camera_->TakePhoto();

        shot.t_takephoto_return = std::chrono::steady_clock::now();
        shot.wall_sdk_return = formatWallClockNow();

        shot.command_to_return_ms =
            std::chrono::duration_cast<std::chrono::milliseconds>(
                shot.t_takephoto_return - shot.t_command).count();

        if (!url.Empty() && url.IsSingleOrigin()) {
            shot.photo_url = url.GetSingleOrigin();
            shot.takephoto_success = true;
        } else {
            auto files = camera_->GetCameraFilesList();
            if (!files.empty()) {
                shot.photo_url = files.back();
                shot.takephoto_success = true;
            } else {
                shot.takephoto_success = false;
                return shot;
            }
        }

        std::chrono::steady_clock::time_point visible_time;
        if (waitUntilFileVisible(
                shot.photo_url,
                std::chrono::milliseconds(4000),
                std::chrono::milliseconds(50),
                visible_time)) {
            shot.t_file_visible = visible_time;
            shot.wall_file_visible = formatWallClockNow();
            shot.file_visible_success = true;

            shot.command_to_visible_ms =
                std::chrono::duration_cast<std::chrono::milliseconds>(
                    shot.t_file_visible - shot.t_command).count();
        }

        // Le SDK ne renvoie pas ici de vrai timestamp shutter explicite
        shot.sdk_trigger_available = false;
        shot.sdk_trigger_ms = -1;

        return shot;
    }

    bool downloadSingleFile(ShotTiming& shot, const std::string& save_directory) {
        if (shot.photo_url.empty()) {
            std::cerr << "Error: Empty photo URL for shot #" << shot.index << std::endl;
            return false;
        }

        std::string save_path = save_directory;
        if (!save_path.empty() && save_path.back() != '/' && save_path.back() != '\\') {
            save_path += "/";
        }

        if (!fileExists(save_path)) {
            std::cerr << "Error: Save directory does not exist: " << save_path << std::endl;
            return false;
        }

        std::string file_name = getFileName(shot.photo_url);
        if (file_name.empty()) {
            file_name = "sequence_" + std::to_string(shot.index) + "_" + getCurrentTime() + ".jpg";
        }

        shot.local_path = save_path + file_name;

        std::cout << "Downloading shot #" << shot.index
                  << " to: " << shot.local_path << std::endl;

        int64_t total_size_known = 0;
        int64_t last_progress = -1;

        shot.wall_download_start = formatWallClockNow();
        shot.t_download_start = std::chrono::steady_clock::now();

        bool ok = camera_->DownloadCameraFile(
            shot.photo_url,
            shot.local_path,
            [&](int64_t current, int64_t total_size) {
                total_size_known = total_size;

                int64_t progress = 0;
                if (total_size > 0) {
                    progress = static_cast<int64_t>(
                        (static_cast<double>(current) * 100.0) / static_cast<double>(total_size));
                    if (current >= total_size) {
                        progress = 100;
                    }
                }

                if (progress != last_progress) {
                    if (total_size > 0) {
                        std::cout << "\r  Progress: " << progress << "% ("
                                  << formatBytes(current) << " / " << formatBytes(total_size) << ")"
                                  << std::flush;
                    } else {
                        std::cout << "\r  Downloaded: " << formatBytes(current) << std::flush;
                    }
                    last_progress = progress;
                }
            });

        shot.t_download_end = std::chrono::steady_clock::now();
        shot.wall_download_end = formatWallClockNow();
        std::cout << std::endl;

        if (!ok) {
            std::cerr << "Error: Download failed for shot #" << shot.index << std::endl;
            shot.download_success = false;
            return false;
        }

        int64_t file_size = getFileSize(shot.local_path);
        if (file_size <= 0) {
            std::cerr << "Error: Downloaded file invalid for shot #" << shot.index << std::endl;
            shot.download_success = false;
            return false;
        }

        shot.downloaded_size = file_size;
        shot.download_success = true;

        shot.download_duration_ms =
            std::chrono::duration_cast<std::chrono::milliseconds>(
                shot.t_download_end - shot.t_download_start).count();

        shot.command_to_local_saved_ms =
            std::chrono::duration_cast<std::chrono::milliseconds>(
                shot.t_download_end - shot.t_command).count();

        std::cout << "Downloaded shot #" << shot.index
                  << " (" << formatBytes(file_size) << ")" << std::endl;

        return true;
    }

public:
    CameraController() : is_connected_(false) {}

    ~CameraController() {
        disconnect();
    }

    bool discoverAndConnect() {
        std::cout << "Discovering Insta360 cameras..." << std::endl;

        ins_camera::SetLogLevel(ins_camera::LogLevel::ERR);
        ins_camera::DeviceDiscovery discovery;
        auto device_list = discovery.GetAvailableDevices();

        if (device_list.empty()) {
            std::cerr << "Error: No Insta360 camera found." << std::endl;
            std::cerr << "Please ensure:" << std::endl;
            std::cerr << "  1. Camera is powered on" << std::endl;
            std::cerr << "  2. Camera is connected via USB or WiFi" << std::endl;
            return false;
        }

        std::cout << "Found " << device_list.size() << " camera(s):" << std::endl;
        for (size_t i = 0; i < device_list.size(); i++) {
            const auto& device = device_list[i];
            std::cout << "  [" << i << "] " << device.camera_name
                      << " (SN: " << device.serial_number
                      << ", FW: " << device.fw_version << ")" << std::endl;
        }

        const auto& selected_device = device_list[0];
        std::cout << "\nConnecting to: " << selected_device.camera_name
                  << " (SN: " << selected_device.serial_number << ")..." << std::endl;

        camera_ = std::make_shared<ins_camera::Camera>(selected_device.info);

        if (!camera_->Open()) {
            std::cerr << "Error: Failed to open camera connection." << std::endl;
            discovery.FreeDeviceDescriptors(device_list);
            return false;
        }

        time_t now = time(nullptr);
        std::tm tm{};
#ifdef _WIN32
        localtime_s(&tm, &now);
        time_t time_seconds = _mkgmtime(&tm);
#else
        localtime_r(&now, &tm);
        time_t time_seconds = timegm(&tm);
#endif
        camera_->SyncLocalTimeToCamera(time_seconds);

        is_connected_ = true;
        std::cout << "Successfully connected to camera!" << std::endl;

        discovery.FreeDeviceDescriptors(device_list);
        return true;
    }

    void disconnect() {
        if (camera_ && is_connected_) {
            camera_->Close();
            is_connected_ = false;
            std::cout << "Disconnected from camera." << std::endl;
        }
    }

    bool takePhoto(const std::string& save_directory = "./") {
        if (!is_connected_ || !camera_) {
            std::cerr << "Error: Camera not connected." << std::endl;
            return false;
        }

        if (!camera_->IsConnected()) {
            std::cerr << "Error: Camera connection lost." << std::endl;
            is_connected_ = false;
            return false;
        }

        ins_camera::StorageStatus storage{};
        if (camera_->GetStorageState(storage)) {
            if (storage.state != ins_camera::STOR_CS_PASS) {
                std::cerr << "Error: Camera storage not ready. State = " << storage.state << std::endl;
                return false;
            }

            std::cout << "Storage OK. Free space: " << formatBytes(storage.free_space)
                      << " / Total: " << formatBytes(storage.total_space) << std::endl;
        } else {
            std::cerr << "Warning: Could not verify storage state, continuing anyway..." << std::endl;
        }

        std::cout << "Setting photo mode..." << std::endl;
        bool ret = camera_->SetPhotoSubMode(ins_camera::SubPhotoMode::PHOTO_HDR);
        if (!ret) {
            std::cerr << "Warning: Failed to set photo mode, continuing anyway..." << std::endl;
        }

        std::cout << "Setting photo size..." << std::endl;
        ins_camera::PhotoSize photo_size = ins_camera::PhotoSize::Size_11968_5984;
        ret = camera_->SetPhotoSize(ins_camera::CameraFunctionMode::FUNCTION_MODE_NORMAL_IMAGE, photo_size);
        if (!ret) {
            std::cerr << "Warning: Failed to set photo size, continuing anyway..." << std::endl;
        }

        std::this_thread::sleep_for(std::chrono::seconds(1));

        std::cout << "Taking photo..." << std::endl;
        const auto url = camera_->TakePhoto();
        std::string photo_url;

        if (url.Empty() || !url.IsSingleOrigin()) {
            std::cerr << "Warning: TakePhoto() returned no usable URL or timed out." << std::endl;
            std::cerr << "Trying fallback via camera file list..." << std::endl;

            std::this_thread::sleep_for(std::chrono::seconds(1));
            auto files = camera_->GetCameraFilesList();

            if (files.empty()) {
                std::cerr << "Error: No files found on camera after TakePhoto timeout." << std::endl;
                return false;
            }

            photo_url = files.back();
            std::cout << "Fallback selected file: " << photo_url << std::endl;
        } else {
            photo_url = url.GetSingleOrigin();
            std::cout << "Photo captured! URL: " << photo_url << std::endl;
        }

        if (!save_directory.empty()) {
            std::string save_path = save_directory;
            if (save_path.back() != '/' && save_path.back() != '\\') {
                save_path += "/";
            }

            if (!fileExists(save_path)) {
                std::cerr << "Warning: Save directory does not exist: " << save_path << std::endl;
                std::cerr << "Photo URL saved on camera: " << photo_url << std::endl;
                return true;
            }

            std::string file_name = getFileName(photo_url);
            if (file_name.empty()) {
                file_name = "photo_" + getCurrentTime() + ".jpg";
            }

            std::string full_path = save_path + file_name;
            std::cout << "Downloading photo to: " << full_path << std::endl;

            int64_t last_progress = -1;
            int64_t last_current = -1;
            auto last_update_time = std::chrono::steady_clock::now();
            int64_t total_size_known = 0;

            bool download_success = camera_->DownloadCameraFile(
                photo_url,
                full_path,
                [&](int64_t current, int64_t total_size) {
                    total_size_known = total_size;
                    auto now = std::chrono::steady_clock::now();
                    auto elapsed = std::chrono::duration_cast<std::chrono::seconds>(now - last_update_time).count();

                    double progress_double = total_size > 0
                        ? (static_cast<double>(current) * 100.0 / static_cast<double>(total_size))
                        : 0.0;
                    int64_t progress = static_cast<int64_t>(progress_double);

                    if (current >= total_size && total_size > 0) {
                        progress = 100;
                    }

                    if (progress != last_progress ||
                        (current != last_current && total_size > 0 && current >= total_size * 0.97)) {
                        if (total_size > 0) {
                            std::cout << "\rDownload progress: " << progress << "% ("
                                      << formatBytes(current) << " / " << formatBytes(total_size) << ")"
                                      << std::flush;
                        } else {
                            std::cout << "\rDownload progress: " << formatBytes(current)
                                      << " downloaded" << std::flush;
                        }

                        last_progress = progress;
                        last_current = current;
                        last_update_time = now;
                    }

                    if (elapsed > 30 && current == last_current && current < total_size) {
                        std::cout << "\nWarning: Download appears stalled at " << progress << "%" << std::endl;
                        std::cout << "Continuing to wait..." << std::flush;
                    }
                });

            if (download_success) {
                if (total_size_known > 0) {
                    std::cout << "\rDownload progress: 100% (" << formatBytes(total_size_known)
                              << " / " << formatBytes(total_size_known) << ")" << std::flush;
                } else {
                    std::cout << "\rDownload progress: 100%" << std::flush;
                }
            }
            std::cout << std::endl;

            if (download_success) {
                int64_t file_size = getFileSize(full_path);
                if (file_size < 0) {
                    std::cerr << "Error: Download reported success but file does not exist: " << full_path << std::endl;
                    return false;
                }
                if (file_size == 0) {
                    std::cerr << "Error: Download reported success but file is empty: " << full_path << std::endl;
                    return false;
                }
                if (total_size_known > 0 && file_size != total_size_known) {
                    std::cerr << "Warning: File size mismatch. Expected: " << formatBytes(total_size_known)
                              << ", Got: " << formatBytes(file_size) << std::endl;
                }

                std::cout << "Photo successfully downloaded to: " << full_path
                          << " (" << formatBytes(file_size) << ")" << std::endl;
                return true;
            } else {
                std::cerr << "Error: Failed to download photo." << std::endl;
                std::cerr << "Photo URL on camera: " << photo_url << std::endl;
                return false;
            }
        }

        return true;
    }

    bool sequence(int nb_images,
                  int duration_seconds,
                  const std::string& save_directory = "./",
                  bool best_quality = false) {
        if (!is_connected_ || !camera_) {
            std::cerr << "Error: Camera not connected." << std::endl;
            return false;
        }

        if (nb_images <= 0 || duration_seconds <= 0) {
            std::cerr << "Error: nb_images and duration_seconds must be > 0." << std::endl;
            return false;
        }

        std::string save_path = save_directory;
        if (!save_path.empty() && save_path.back() != '/' && save_path.back() != '\\') {
            save_path += "/";
        }

        if (!fileExists(save_path)) {
            std::cerr << "Error: Save directory does not exist: " << save_path << std::endl;
            return false;
        }

        if (!preparePhotoSequence(best_quality)) {
            return false;
        }

        SequenceSummary summary;
        summary.planned_images = nb_images;
        summary.wall_operation_start = formatWallClockNow();

        auto op_start = std::chrono::steady_clock::now();

        std::cout << "\n=== Starting sequence ===" << std::endl;
        std::cout << "Images: " << nb_images << std::endl;
        std::cout << "Duration: " << duration_seconds << " s" << std::endl;
        std::cout << "Profile: " << (best_quality ? "quality" : "speed") << std::endl;
        std::cout << "Download mode: deferred (after capture)" << std::endl;

        std::vector<ShotTiming> shots;
        shots.reserve(static_cast<size_t>(nb_images));

        auto total_duration = std::chrono::milliseconds(duration_seconds * 1000);
        auto period = total_duration / nb_images;
        auto t0 = std::chrono::steady_clock::now();

        summary.wall_capture_start = formatWallClockNow();
        auto capture_start = std::chrono::steady_clock::now();

        for (int i = 0; i < nb_images; ++i) {
            auto target_time = t0 + i * period;
            std::this_thread::sleep_until(target_time);

            std::cout << "\n[Capture " << (i + 1) << "/" << nb_images << "]" << std::endl;
            ShotTiming shot = captureOnePhotoNoDownload(i);

            if (shot.takephoto_success) {
                std::cout << "  Photo URL: " << shot.photo_url << std::endl;
                std::cout << "  Command -> SDK return: " << shot.command_to_return_ms << " ms" << std::endl;

                if (shot.file_visible_success) {
                    std::cout << "  Command -> file visible on camera: "
                              << shot.command_to_visible_ms << " ms" << std::endl;
                } else {
                    std::cout << "  File visibility timing: unavailable" << std::endl;
                }

                summary.captured_images++;
            } else {
                std::cout << "  Capture failed" << std::endl;
            }

            shots.push_back(std::move(shot));
        }

        auto capture_end = std::chrono::steady_clock::now();
        summary.wall_capture_end = formatWallClockNow();
        summary.capture_phase_duration_ms =
            std::chrono::duration_cast<std::chrono::milliseconds>(
                capture_end - capture_start).count();

        std::cout << "\n=== Capture phase complete ===" << std::endl;
        std::cout << "Now downloading files..." << std::endl;

        summary.wall_download_start = formatWallClockNow();
        auto download_start = std::chrono::steady_clock::now();

        int download_success_count = 0;

        for (auto& shot : shots) {
            if (!shot.takephoto_success || shot.photo_url.empty()) {
                continue;
            }

            if (downloadSingleFile(shot, save_directory)) {
                download_success_count++;
                summary.downloaded_images++;
            }
        }

        auto download_end = std::chrono::steady_clock::now();
        summary.wall_download_end = formatWallClockNow();
        summary.download_phase_duration_ms =
            std::chrono::duration_cast<std::chrono::milliseconds>(
                download_end - download_start).count();

        std::string csv_path = save_path + "sequence_detail_" + getCurrentTime() + ".csv";
        std::ofstream csv(csv_path);

        if (csv.is_open()) {
            csv << "index,"
                   "wall_command_sent,wall_sdk_return,wall_file_visible,wall_download_start,wall_download_end,"
                   "takephoto_success,file_visible_success,download_success,"
                   "photo_url,local_path,"
                   "sdk_trigger_available,sdk_trigger_ms,"
                   "command_to_return_ms,command_to_visible_ms,download_duration_ms,command_to_local_saved_ms,"
                   "downloaded_size_bytes\n";

            for (const auto& shot : shots) {
                csv << shot.index << ","
                    << "\"" << shot.wall_command_sent << "\","
                    << "\"" << shot.wall_sdk_return << "\","
                    << "\"" << shot.wall_file_visible << "\","
                    << "\"" << shot.wall_download_start << "\","
                    << "\"" << shot.wall_download_end << "\","
                    << (shot.takephoto_success ? 1 : 0) << ","
                    << (shot.file_visible_success ? 1 : 0) << ","
                    << (shot.download_success ? 1 : 0) << ","
                    << "\"" << shot.photo_url << "\","
                    << "\"" << shot.local_path << "\","
                    << (shot.sdk_trigger_available ? 1 : 0) << ","
                    << shot.sdk_trigger_ms << ","
                    << shot.command_to_return_ms << ","
                    << shot.command_to_visible_ms << ","
                    << shot.download_duration_ms << ","
                    << shot.command_to_local_saved_ms << ","
                    << shot.downloaded_size
                    << "\n";
            }

            csv.close();
            std::cout << "Timing CSV written to: " << csv_path << std::endl;
        } else {
            std::cerr << "Warning: Could not write CSV file: " << csv_path << std::endl;
        }

        auto op_end = std::chrono::steady_clock::now();
        summary.wall_operation_end = formatWallClockNow();
        summary.total_operation_duration_ms =
            std::chrono::duration_cast<std::chrono::milliseconds>(
                op_end - op_start).count();

        std::string summary_path = save_path + "sequence_summary_" + getCurrentTime() + ".csv";
        std::ofstream sum(summary_path);

        if (sum.is_open()) {
            sum << "wall_operation_start,wall_capture_start,wall_capture_end,wall_download_start,wall_download_end,wall_operation_end,"
                   "planned_images,captured_images,downloaded_images,"
                   "capture_phase_duration_ms,download_phase_duration_ms,total_operation_duration_ms\n";

            sum << "\"" << summary.wall_operation_start << "\","
                << "\"" << summary.wall_capture_start << "\","
                << "\"" << summary.wall_capture_end << "\","
                << "\"" << summary.wall_download_start << "\","
                << "\"" << summary.wall_download_end << "\","
                << "\"" << summary.wall_operation_end << "\","
                << summary.planned_images << ","
                << summary.captured_images << ","
                << summary.downloaded_images << ","
                << summary.capture_phase_duration_ms << ","
                << summary.download_phase_duration_ms << ","
                << summary.total_operation_duration_ms << "\n";

            sum.close();
            std::cout << "Summary CSV written to: " << summary_path << std::endl;
        } else {
            std::cerr << "Warning: Could not write summary CSV file: " << summary_path << std::endl;
        }

        int capture_success_count = 0;
        long long sum_return_ms = 0;
        int count_return_ms = 0;
        long long sum_visible_ms = 0;
        int count_visible_ms = 0;
        long long sum_local_saved_ms = 0;
        int count_local_saved_ms = 0;

        for (const auto& shot : shots) {
            if (shot.takephoto_success) {
                capture_success_count++;
            }
            if (shot.command_to_return_ms >= 0) {
                sum_return_ms += shot.command_to_return_ms;
                count_return_ms++;
            }
            if (shot.command_to_visible_ms >= 0) {
                sum_visible_ms += shot.command_to_visible_ms;
                count_visible_ms++;
            }
            if (shot.command_to_local_saved_ms >= 0) {
                sum_local_saved_ms += shot.command_to_local_saved_ms;
                count_local_saved_ms++;
            }
        }

        std::cout << "\n=== Sequence summary ===" << std::endl;
        std::cout << "Capture success: " << capture_success_count << "/" << nb_images << std::endl;
        std::cout << "Download success: " << download_success_count << "/" << nb_images << std::endl;

        if (count_return_ms > 0) {
            std::cout << "Average command -> SDK return: "
                      << (sum_return_ms / count_return_ms) << " ms" << std::endl;
        }

        if (count_visible_ms > 0) {
            std::cout << "Average command -> file visible on camera: "
                      << (sum_visible_ms / count_visible_ms) << " ms" << std::endl;
        }

        if (count_local_saved_ms > 0) {
            std::cout << "Average command -> file saved locally: "
                      << (sum_local_saved_ms / count_local_saved_ms) << " ms" << std::endl;
        }

        return capture_success_count > 0;
    }

    bool shutdownCamera() {
        if (!is_connected_ || !camera_) {
            std::cerr << "Error: Camera not connected." << std::endl;
            return false;
        }

        std::cout << "Shutting down camera..." << std::endl;
        bool ret = camera_->ShutdownCamera();

        if (ret) {
            std::cout << "Camera shutdown command sent successfully." << std::endl;
            is_connected_ = false;
            return true;
        } else {
            std::cerr << "Error: Failed to shutdown camera." << std::endl;
            return false;
        }
    }

    bool getBatteryStatus() {
        if (!is_connected_ || !camera_) {
            std::cerr << "Error: Camera not connected." << std::endl;
            return false;
        }

        ins_camera::BatteryStatus status{};
        bool ret = camera_->GetBatteryStatus(status);

        if (!ret) {
            std::cerr << "Error: Failed to get battery status." << std::endl;
            return false;
        }

        std::cout << "Battery Status:" << std::endl;
        std::cout << "  Power Type: " << (status.power_type == ins_camera::PowerType::BATTERY ? "Battery" : "Adapter") << std::endl;
        std::cout << "  Battery Level: " << status.battery_level << "%" << std::endl;
        std::cout << "  Battery Scale: " << status.battery_scale << std::endl;

        return true;
    }

    bool getStorageStatus() {
        if (!is_connected_ || !camera_) {
            std::cerr << "Error: Camera not connected." << std::endl;
            return false;
        }

        ins_camera::StorageStatus status{};
        bool ret = camera_->GetStorageState(status);

        if (!ret) {
            std::cerr << "Error: Failed to get storage status." << std::endl;
            return false;
        }

        auto localFormatBytes = [](uint64_t bytes) -> std::string {
            const uint64_t GB = 1024ULL * 1024ULL * 1024ULL;
            const uint64_t MB = 1024ULL * 1024ULL;

            if (bytes >= GB) {
                double gb = static_cast<double>(bytes) / GB;
                char buffer[32];
                snprintf(buffer, sizeof(buffer), "%.2f GB", gb);
                return std::string(buffer);
            } else if (bytes >= MB) {
                double mb = static_cast<double>(bytes) / MB;
                char buffer[32];
                snprintf(buffer, sizeof(buffer), "%.2f MB", mb);
                return std::string(buffer);
            } else {
                return std::to_string(bytes) + " bytes";
            }
        };

        std::string state_text;
        switch (status.state) {
            case ins_camera::STOR_CS_PASS:
                state_text = "OK";
                break;
            case ins_camera::STOR_CS_NOCARD:
                state_text = "No Card";
                break;
            case ins_camera::STOR_CS_NOSPACE:
                state_text = "No Space";
                break;
            case ins_camera::STOR_CS_INVALID_FORMAT:
                state_text = "Invalid Format";
                break;
            case ins_camera::STOR_CS_WPCARD:
                state_text = "Write Protected";
                break;
            case ins_camera::STOR_CS_OTHER_ERROR:
                state_text = "Other Error";
                break;
            default:
                state_text = "Unknown";
                break;
        }

        uint64_t used_space = status.total_space - status.free_space;
        double used_percentage = status.total_space > 0
            ? (static_cast<double>(used_space) / status.total_space) * 100.0
            : 0.0;

        std::cout << "Storage Status:" << std::endl;
        std::cout << "  State: " << state_text << std::endl;
        std::cout << "  Total Space: " << localFormatBytes(status.total_space) << std::endl;
        std::cout << "  Free Space: " << localFormatBytes(status.free_space) << std::endl;
        std::cout << "  Used Space: " << localFormatBytes(used_space) << " ("
                  << std::fixed << std::setprecision(1) << used_percentage << "%)" << std::endl;

        return true;
    }

    bool setVideoMode() {
        if (!is_connected_ || !camera_) {
            std::cerr << "Error: Camera not connected." << std::endl;
            return false;
        }

        std::cout << "Setting video mode..." << std::endl;
        bool ret = camera_->SetVideoSubMode(ins_camera::SubVideoMode::VIDEO_NORMAL);

        if (!ret) {
            std::cerr << "Error: Failed to set video mode." << std::endl;
            return false;
        }

        std::cout << "Video mode set successfully." << std::endl;
        return true;
    }

    bool startRecording() {
        if (!is_connected_ || !camera_) {
            std::cerr << "Error: Camera not connected." << std::endl;
            return false;
        }

        if (!camera_->IsConnected()) {
            std::cerr << "Error: Camera connection lost." << std::endl;
            is_connected_ = false;
            return false;
        }

        std::cout << "Setting video mode..." << std::endl;
        bool ret = camera_->SetVideoSubMode(ins_camera::SubVideoMode::VIDEO_NORMAL);
        if (!ret) {
            std::cerr << "Warning: Failed to set video mode, continuing anyway..." << std::endl;
        }

        std::cout << "Starting recording..." << std::endl;
        ret = camera_->StartRecording();

        if (!ret) {
            std::cerr << "Error: Failed to start recording." << std::endl;
            return false;
        }

        std::cout << "Recording started successfully!" << std::endl;
        return true;
    }

    bool stopRecording(const std::string& save_directory = "./") {
        if (!is_connected_ || !camera_) {
            std::cerr << "Error: Camera not connected." << std::endl;
            return false;
        }

        if (!camera_->IsConnected()) {
            std::cerr << "Error: Camera connection lost." << std::endl;
            is_connected_ = false;
            return false;
        }

        std::cout << "Stopping recording..." << std::endl;
        const auto url = camera_->StopRecording();

        if (url.Empty()) {
            std::cerr << "Error: Failed to stop recording or no video was recorded." << std::endl;
            return false;
        }

        std::cout << "Recording stopped successfully!" << std::endl;

        std::string save_path = save_directory;
        if (!save_path.empty() && save_path.back() != '/' && save_path.back() != '\\') {
            save_path += "/";
        }

        if (!save_directory.empty() && !fileExists(save_path)) {
            std::cerr << "Warning: Save directory does not exist: " << save_path << std::endl;
            std::cerr << "Video URL(s) saved on camera:" << std::endl;
            if (url.IsSingleOrigin()) {
                std::cout << "  " << url.GetSingleOrigin() << std::endl;
            } else {
                const auto& origins = url.OriginUrls();
                for (size_t i = 0; i < origins.size(); i++) {
                    std::cout << "  [" << i << "] " << origins[i] << std::endl;
                }
            }
            return true;
        }

        if (!save_directory.empty()) {
            bool all_success = true;

            if (url.IsSingleOrigin()) {
                const std::string video_url = url.GetSingleOrigin();
                std::cout << "Video URL: " << video_url << std::endl;

                std::string file_name = getFileName(video_url);
                if (file_name.empty()) {
                    file_name = "video_" + getCurrentTime() + ".mp4";
                }

                std::string full_path = save_path + file_name;
                std::cout << "Downloading video to: " << full_path << std::endl;

                int64_t last_progress = -1;
                int64_t last_current = -1;
                auto last_update_time = std::chrono::steady_clock::now();
                int64_t total_size_known = 0;

                bool download_success = camera_->DownloadCameraFile(video_url, full_path,
                    [&](int64_t current, int64_t total_size) {
                        total_size_known = total_size;
                        auto now = std::chrono::steady_clock::now();
                        auto elapsed = std::chrono::duration_cast<std::chrono::seconds>(now - last_update_time).count();

                        double progress_double = total_size > 0 ? (static_cast<double>(current) * 100.0 / static_cast<double>(total_size)) : 0.0;
                        int64_t progress = static_cast<int64_t>(progress_double);

                        if (current >= total_size && total_size > 0) {
                            progress = 100;
                        }

                        if (progress != last_progress || (current != last_current && total_size > 0 && current >= total_size * 0.97)) {
                            if (total_size > 0) {
                                std::cout << "\rDownload progress: " << progress << "% ("
                                         << formatBytes(current) << " / " << formatBytes(total_size) << ")" << std::flush;
                            } else {
                                std::cout << "\rDownload progress: " << formatBytes(current) << " downloaded" << std::flush;
                            }
                            last_progress = progress;
                            last_current = current;
                            last_update_time = now;
                        }

                        if (elapsed > 30 && current == last_current && current < total_size) {
                            std::cout << "\nWarning: Download appears stalled at " << progress << "%" << std::endl;
                            std::cout << "Continuing to wait..." << std::flush;
                        }
                    });

                if (download_success) {
                    if (total_size_known > 0) {
                        std::cout << "\rDownload progress: 100% (" << formatBytes(total_size_known)
                                 << " / " << formatBytes(total_size_known) << ")" << std::flush;
                    } else {
                        std::cout << "\rDownload progress: 100%" << std::flush;
                    }
                }
                std::cout << std::endl;

                if (download_success) {
                    int64_t file_size = getFileSize(full_path);
                    if (file_size < 0) {
                        std::cerr << "Error: Download reported success but file does not exist: " << full_path << std::endl;
                        all_success = false;
                    } else if (file_size == 0) {
                        std::cerr << "Error: Download reported success but file is empty: " << full_path << std::endl;
                        all_success = false;
                    } else {
                        if (total_size_known > 0 && file_size != total_size_known) {
                            std::cerr << "Warning: File size mismatch. Expected: " << formatBytes(total_size_known)
                                     << ", Got: " << formatBytes(file_size) << std::endl;
                        }
                        std::cout << "Video successfully downloaded to: " << full_path
                                 << " (" << formatBytes(file_size) << ")" << std::endl;
                    }
                } else {
                    std::cerr << "Error: Failed to download video." << std::endl;
                    std::cerr << "Video URL on camera: " << video_url << std::endl;
                    all_success = false;
                }
            } else {
                const auto& origins = url.OriginUrls();
                std::cout << "Video URLs (" << origins.size() << "):" << std::endl;

                for (size_t i = 0; i < origins.size(); i++) {
                    const std::string& video_url = origins[i];
                    std::cout << "\n[" << (i + 1) << "/" << origins.size() << "] " << video_url << std::endl;

                    std::string file_name = getFileName(video_url);
                    if (file_name.empty()) {
                        file_name = "video_" + getCurrentTime() + "_" + std::to_string(i) + ".mp4";
                    }

                    std::string full_path = save_path + file_name;
                    std::cout << "Downloading to: " << full_path << std::endl;

                    int64_t last_progress = -1;
                    int64_t last_current = -1;
                    auto last_update_time = std::chrono::steady_clock::now();
                    int64_t total_size_known = 0;

                    bool download_success = camera_->DownloadCameraFile(video_url, full_path,
                        [&](int64_t current, int64_t total_size) {
                            total_size_known = total_size;
                            auto now = std::chrono::steady_clock::now();
                            auto elapsed = std::chrono::duration_cast<std::chrono::seconds>(now - last_update_time).count();

                            double progress_double = total_size > 0 ? (static_cast<double>(current) * 100.0 / static_cast<double>(total_size)) : 0.0;
                            int64_t progress = static_cast<int64_t>(progress_double);

                            if (current >= total_size && total_size > 0) {
                                progress = 100;
                            }

                            if (progress != last_progress || (current != last_current && total_size > 0 && current >= total_size * 0.97)) {
                                if (total_size > 0) {
                                    std::cout << "\rDownload progress: " << progress << "% ("
                                             << formatBytes(current) << " / " << formatBytes(total_size) << ")" << std::flush;
                                } else {
                                    std::cout << "\rDownload progress: " << formatBytes(current) << " downloaded" << std::flush;
                                }
                                last_progress = progress;
                                last_current = current;
                                last_update_time = now;
                            }

                            if (elapsed > 30 && current == last_current && current < total_size) {
                                std::cout << "\nWarning: Download appears stalled at " << progress << "%" << std::endl;
                                std::cout << "Continuing to wait..." << std::flush;
                            }
                        });

                    if (download_success) {
                        if (total_size_known > 0) {
                            std::cout << "\rDownload progress: 100% (" << formatBytes(total_size_known)
                                     << " / " << formatBytes(total_size_known) << ")" << std::flush;
                        } else {
                            std::cout << "\rDownload progress: 100%" << std::flush;
                        }
                    }
                    std::cout << std::endl;

                    if (download_success) {
                        int64_t file_size = getFileSize(full_path);
                        if (file_size < 0) {
                            std::cerr << "Error: Download reported success but file does not exist: " << full_path << std::endl;
                            all_success = false;
                        } else if (file_size == 0) {
                            std::cerr << "Error: Download reported success but file is empty: " << full_path << std::endl;
                            all_success = false;
                        } else {
                            if (total_size_known > 0 && file_size != total_size_known) {
                                std::cerr << "Warning: File size mismatch. Expected: " << formatBytes(total_size_known)
                                         << ", Got: " << formatBytes(file_size) << std::endl;
                            }
                            std::cout << "Successfully downloaded: " << full_path
                                     << " (" << formatBytes(file_size) << ")" << std::endl;
                        }
                    } else {
                        std::cerr << "Error: Failed to download video." << std::endl;
                        std::cerr << "Video URL on camera: " << video_url << std::endl;
                        all_success = false;
                    }
                }
            }

            return all_success;
        } else {
            if (url.IsSingleOrigin()) {
                std::cout << "Video URL: " << url.GetSingleOrigin() << std::endl;
            } else {
                const auto& origins = url.OriginUrls();
                std::cout << "Video URLs (" << origins.size() << "):" << std::endl;
                for (size_t i = 0; i < origins.size(); i++) {
                    std::cout << "  [" << i << "] " << origins[i] << std::endl;
                }
            }
        }

        return true;
    }

    bool copyStorage(const std::string& save_directory = "./") {
        if (!is_connected_ || !camera_) {
            std::cerr << "Error: Camera not connected." << std::endl;
            return false;
        }

        if (!camera_->IsConnected()) {
            std::cerr << "Error: Camera connection lost." << std::endl;
            is_connected_ = false;
            return false;
        }

        std::cout << "Getting list of files from camera..." << std::endl;
        std::vector<std::string> file_list = camera_->GetCameraFilesList();

        if (file_list.empty()) {
            std::cout << "No files found on camera storage." << std::endl;
            return true;
        }

        std::cout << "Found " << file_list.size() << " file(s) on camera." << std::endl;

        std::string save_path = save_directory;
        if (save_path.back() != '/' && save_path.back() != '\\') {
            save_path += "/";
        }

        if (!fileExists(save_path)) {
            std::cerr << "Error: Save directory does not exist: " << save_path << std::endl;
            return false;
        }

        int success_count = 0;
        int fail_count = 0;

        for (size_t i = 0; i < file_list.size(); i++) {
            const std::string& file_url = file_list[i];
            std::string file_name = getFileName(file_url);

            if (file_name.empty()) {
                file_name = "file_" + getCurrentTime() + "_" + std::to_string(i);
            }

            std::string full_path = save_path + file_name;
            std::cout << "\n[" << (i + 1) << "/" << file_list.size() << "] Downloading: " << file_name << std::endl;

            int64_t last_progress = -1;
            int64_t last_current = -1;
            auto last_update_time = std::chrono::steady_clock::now();
            int64_t total_size_known = 0;

            bool download_success = camera_->DownloadCameraFile(file_url, full_path,
                [&](int64_t current, int64_t total_size) {
                    total_size_known = total_size;
                    auto now = std::chrono::steady_clock::now();
                    auto elapsed = std::chrono::duration_cast<std::chrono::seconds>(now - last_update_time).count();

                    double progress_double = total_size > 0 ? (static_cast<double>(current) * 100.0 / static_cast<double>(total_size)) : 0.0;
                    int64_t progress = static_cast<int64_t>(progress_double);

                    if (current >= total_size && total_size > 0) {
                        progress = 100;
                    }

                    if (progress != last_progress || (current != last_current && total_size > 0 && current >= total_size * 0.97)) {
                        if (total_size > 0) {
                            std::cout << "\rDownload progress: " << progress << "% ("
                                     << formatBytes(current) << " / " << formatBytes(total_size) << ")" << std::flush;
                        } else {
                            std::cout << "\rDownload progress: " << formatBytes(current) << " downloaded" << std::flush;
                        }
                        last_progress = progress;
                        last_current = current;
                        last_update_time = now;
                    }

                    if (elapsed > 30 && current == last_current && current < total_size) {
                        std::cout << "\nWarning: Download appears stalled at " << progress << "%" << std::endl;
                        std::cout << "Continuing to wait..." << std::flush;
                    }
                });

            if (download_success) {
                if (total_size_known > 0) {
                    std::cout << "\rDownload progress: 100% (" << formatBytes(total_size_known)
                             << " / " << formatBytes(total_size_known) << ")" << std::flush;
                } else {
                    std::cout << "\rDownload progress: 100%" << std::flush;
                }
            }
            std::cout << std::endl;

            if (download_success) {
                int64_t file_size = getFileSize(full_path);
                if (file_size < 0) {
                    std::cerr << "Error: Download reported success but file does not exist: " << full_path << std::endl;
                    fail_count++;
                    continue;
                }
                if (file_size == 0) {
                    std::cerr << "Error: Download reported success but file is empty: " << full_path << std::endl;
                    fail_count++;
                    continue;
                }
                if (total_size_known > 0 && file_size != total_size_known) {
                    std::cerr << "Warning: File size mismatch. Expected: " << formatBytes(total_size_known)
                             << ", Got: " << formatBytes(file_size) << std::endl;
                }
                std::cout << "Successfully downloaded: " << full_path
                         << " (" << formatBytes(file_size) << ")" << std::endl;

                std::cout << "Deleting from camera: " << file_url << std::endl;
                bool delete_success = camera_->DeleteCameraFile(file_url);

                if (delete_success) {
                    std::cout << "Successfully deleted from camera." << std::endl;
                    success_count++;
                } else {
                    std::cerr << "Warning: Failed to delete file from camera: " << file_url << std::endl;
                    std::cerr << "File was downloaded but remains on camera." << std::endl;
                    success_count++;
                }
            } else {
                std::cerr << "Error: Failed to download: " << file_url << std::endl;
                fail_count++;
            }
        }

        std::cout << "\n=== Copy Summary ===" << std::endl;
        std::cout << "Successfully copied: " << success_count << " file(s)" << std::endl;
        if (fail_count > 0) {
            std::cout << "Failed: " << fail_count << " file(s)" << std::endl;
        }
        std::cout << "Total: " << file_list.size() << " file(s)" << std::endl;

        return fail_count == 0;
    }

    bool isConnected() const {
        return is_connected_ && camera_ && camera_->IsConnected();
    }
};

void printUsage(const char* program_name) {
    std::cout << "Insta360 Camera Control for Raspberry Pi" << std::endl;
    std::cout << "Usage: " << program_name << " <command> [options]" << std::endl;
    std::cout << std::endl;
    std::cout << "Commands:" << std::endl;
    std::cout << "  connect                                  - Connect to camera" << std::endl;
    std::cout << "  photo [save_dir]                         - Take a photo (optionally save to directory)" << std::endl;
    std::cout << "  sequence <n> <duration_sec> [dir] [speed|quality] - Take multiple photos over a duration" << std::endl;
    std::cout << "  shutdown                                 - Power off the camera" << std::endl;
    std::cout << "  battery                                  - Get battery status" << std::endl;
    std::cout << "  storage                                  - Get storage capacity status" << std::endl;
    std::cout << "  video-mode                               - Switch camera to video mode" << std::endl;
    std::cout << "  record-start                             - Start recording video (keeps connection open)" << std::endl;
    std::cout << "  record-stop [dir]                        - Stop recording video (optionally save to directory)" << std::endl;
    std::cout << "  copy-storage [dir]                       - Copy all files from camera storage to directory (deletes from camera after copying)" << std::endl;
    std::cout << "  interactive                              - Interactive mode" << std::endl;
    std::cout << std::endl;
    std::cout << "Examples:" << std::endl;
    std::cout << "  " << program_name << " copy-storage ./videos" << std::endl;
    std::cout << "  " << program_name << " photo" << std::endl;
    std::cout << "  " << program_name << " photo ./photos" << std::endl;
    std::cout << "  " << program_name << " sequence 20 10 ./photos speed" << std::endl;
    std::cout << "  " << program_name << " sequence 10 15 ./photos quality" << std::endl;
    std::cout << "  " << program_name << " record-stop" << std::endl;
    std::cout << "  " << program_name << " record-stop ./videos" << std::endl;
    std::cout << "  " << program_name << " shutdown" << std::endl;
    std::cout << "  " << program_name << " interactive" << std::endl;
}

int main(int argc, char* argv[]) {
    if (argc < 2) {
        printUsage(argv[0]);
        return 1;
    }

    std::string command = argv[1];
    CameraController controller;

    if (command == "connect") {
        if (!controller.discoverAndConnect()) {
            return 1;
        }
        std::cout << "Camera connected. Use 'photo', 'sequence', 'shutdown', 'battery', 'storage', or video commands." << std::endl;
        return 0;
    }

    if (!controller.discoverAndConnect()) {
        return 1;
    }

    if (command == "photo") {
        std::string save_dir = (argc > 2) ? argv[2] : "./";
        bool success = controller.takePhoto(save_dir);
        controller.disconnect();
        return success ? 0 : 1;
    }
    else if (command == "sequence") {
        if (argc < 4) {
            std::cerr << "Usage: " << argv[0]
                      << " sequence <nb_images> <duration_sec> [save_dir] [speed|quality]" << std::endl;
            controller.disconnect();
            return 1;
        }

        int nb_images = std::stoi(argv[2]);
        int duration_sec = std::stoi(argv[3]);
        std::string save_dir = (argc > 4) ? argv[4] : "./";
        std::string profile = (argc > 5) ? argv[5] : "speed";

        bool best_quality = (profile == "quality");

        bool success = controller.sequence(nb_images, duration_sec, save_dir, best_quality);
        controller.disconnect();
        return success ? 0 : 1;
    }
    else if (command == "shutdown") {
        bool success = controller.shutdownCamera();
        controller.disconnect();
        return success ? 0 : 1;
    }
    else if (command == "battery") {
        bool success = controller.getBatteryStatus();
        controller.disconnect();
        return success ? 0 : 1;
    }
    else if (command == "storage") {
        bool success = controller.getStorageStatus();
        controller.disconnect();
        return success ? 0 : 1;
    }
    else if (command == "video-mode") {
        bool success = controller.setVideoMode();
        controller.disconnect();
        return success ? 0 : 1;
    }
    else if (command == "record-start") {
        bool success = controller.startRecording();
        controller.disconnect();
        return success ? 0 : 1;
    }
    else if (command == "record-stop") {
        std::string save_dir = (argc > 2) ? argv[2] : "./";
        bool success = controller.stopRecording(save_dir);
        controller.disconnect();
        return success ? 0 : 1;
    }
    else if (command == "copy-storage") {
        std::string save_dir = (argc > 2) ? argv[2] : "./";
        bool success = controller.copyStorage(save_dir);
        controller.disconnect();
        return success ? 0 : 1;
    }
    else if (command == "interactive") {
        std::cout << "\n=== Interactive Mode ===" << std::endl;
        std::cout << "Commands: photo [dir], sequence <n> <duration_sec> [dir] [speed|quality], shutdown, battery, storage, record-start, record-stop [dir], quit" << std::endl;

        std::string line;
        while (true) {
            std::cout << "\n> ";
            std::getline(std::cin, line);

            if (line == "quit" || line == "exit") {
                break;
            }
            else if (line == "photo") {
                controller.takePhoto("./");
            }
            else if (line.substr(0, 5) == "photo") {
                std::string dir = line.length() > 6 ? line.substr(6) : "./";
                controller.takePhoto(dir);
            }
            else if (line == "shutdown") {
                if (controller.shutdownCamera()) {
                    break;
                }
            }
            else if (line == "battery") {
                controller.getBatteryStatus();
            }
            else if (line == "storage") {
                controller.getStorageStatus();
            }
            else if (line == "video-mode") {
                controller.setVideoMode();
            }
            else if (line == "record-start") {
                controller.startRecording();
            }
            else if (line == "record-stop") {
                controller.stopRecording("./");
            }
            else if (line.substr(0, 12) == "record-stop ") {
                std::string dir = line.length() > 12 ? line.substr(12) : "./";
                controller.stopRecording(dir);
            }
            else if (line.empty()) {
                continue;
            }
            else {
                std::cout << "Unknown command. Try: photo, sequence, shutdown, battery, storage, video-mode, record-start, record-stop, quit" << std::endl;
            }

            if (!controller.isConnected()) {
                std::cout << "Camera disconnected. Exiting..." << std::endl;
                break;
            }
        }

        controller.disconnect();
        return 0;
    }
    else {
        std::cerr << "Unknown command: " << command << std::endl;
        printUsage(argv[0]);
        return 1;
    }

    return 0;
}
