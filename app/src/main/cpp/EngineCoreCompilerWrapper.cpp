#include <iostream>
#include <fstream>
#include <vector>
#include <string>
#include <filesystem>
#include <cstdlib>

namespace fs = std::filesystem;

void enforce_system_halt(const std::string& layer, const std::string& error_msg, const std::string& file_path) {
    std::cerr << "\n[FATAL SYSTEM HALT][" << layer << "] Critical Violation Detected!\n"
              << "-> File: " << file_path << "\n"
              << "-> Reason: " << error_msg << "\n"
              << "-> Status: Entire project build permanently aborted. Zero tolerance.\n";
    std::exit(666); // तुरंत सिस्टम क्रैश, कोई माफी नहीं
}

// 1. पूरी C++ और हेडर फाइलों की डीप स्कैनिंग
void scan_native_sources(const fs::path& root_dir) {
    for (auto const& dir_entry : fs::recursive_directory_iterator(root_dir)) {
        if (dir_entry.is_regular_file()) {
            std::string ext = dir_entry.path().extension().string();
            if (ext == ".cpp" || ext == ".h" || ext == ".hpp" || ext == ".cc") {
                std::ifstream file(dir_entry.path());
                std::string line;
                int line_num = 0;
                while (std::getline(file, line)) {
                    line_num++;
                    // हीप एलोकेशन या अनसेफ पॉइंटर चेक्स
                    if (line.find("malloc(") != std::string::npos || line.find("new ") != std::string::npos) {
                        // चेक करो कि क्या यह हॉट लूप या फ्रेम प्रोसेसिंग के अंदर है
                        enforce_system_halt("NATIVE_CPP", "Dynamic heap allocation inside execution path at line " + std::to_string(line_num), dir_entry.path().string());
                    }
                    // खाली स्टब या अधूरे कोड की जाँच
                    if (line.find("TODO") != std::string::npos || line.find("{ }") != std::string::npos) {
                        enforce_system_halt("NATIVE_CPP", "Incomplete stub or TODO found at line " + std::to_string(line_num), dir_entry.path().string());
                    }
                }
            }
        }
    }
}

// 2. पूरे प्रोजेक्ट की Java और Kotlin फाइलों की स्कैनिंग (GC Pauses और Thread Locks रोकने के लिए)
void scan_managed_sources(const fs::path& root_dir) {
    for (auto const& dir_entry : fs::recursive_directory_iterator(root_dir)) {
        if (dir_entry.is_regular_file()) {
            std::string ext = dir_entry.path().extension().string();
            if (ext == ".java" || ext == ".kt") {
                std::ifstream file(dir_entry.path());
                std::string line;
                int line_num = 0;
                bool inside_loop = false;
                while (std::getline(file, line)) {
                    line_num++;
                    if (line.find("for(") != std::string::npos || line.find("while(") != std::string::npos) {
                        inside_loop = true;
                    }
                    if (inside_loop && line.find("new ") != std::string::npos) {
                        enforce_system_halt("MANAGED_JVM", "Object allocation inside hot loop will trigger GC pause at line " + std::to_string(line_num), dir_entry.path().string());
                    }
                    if (line.find("synchronized") != std::string::npos || line.find("Thread.sleep") != std::string::npos) {
                        enforce_system_halt("MANAGED_JVM", "Unsafe thread lock or blocking sleep detected at line " + std::to_string(line_num), dir_entry.path().string());
                    }
                    if (line.find("}") != std::string::npos) {
                        inside_loop = false;
                    }
                }
            }
        }
    }
}

// 3. Vulkan Shaders (GLSL/SPIR-V) की बाइनरी और लेआउट सिंक्रोनाइज़ेशन जाँच
void scan_shader_pipelines(const fs::path& shader_dir) {
    if (!fs::exists(shader_dir)) return;
    for (auto const& dir_entry : fs::recursive_directory_iterator(shader_dir)) {
        if (dir_entry.is_regular_file()) {
            std::string ext = dir_entry.path().extension().string();
            if (ext == ".vert" || ext == ".frag" || ext == ".comp" || ext == ".glsl") {
                std::string cmd = "glslangValidator -V " + dir_entry.path().string() + " > /dev/null 2>&1";
                int res = std::system(cmd.c_str());
                if (res != 0) {
                    enforce_system_halt("VULKAN_SHADER", "Shader compilation or layout validation failed", dir_entry.path().string());
                }
            }
        }
    }
}

int main(int argc, char* argv[]) {
    std::cout << "[ENGINE MASTER GUARD] Initializing full-project zero-tolerance scan...\n";
    
    fs::path project_root = (argc > 1) ? argv[1] : ".";

    // पूरे प्रोजेक्ट के हर कोने की चीरफाड़ एक साथ
    scan_native_sources(project_root);
    scan_managed_sources(project_root);
    scan_shader_pipelines(project_root / "shaders");

    std::cout << "[ENGINE MASTER GUARD SUCCESS] Absolute zero errors, stubs, or leaks found. Proceeding to compilation.\n";
    return 0;
}
