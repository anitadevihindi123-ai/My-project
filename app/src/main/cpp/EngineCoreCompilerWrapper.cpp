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
    std::exit(666); 
}

// 1. स्मार्ट स्कोप और मल्टी-लाइन कॉन्टेक्स्ट-अवेयर C++ स्कैनर
void scan_native_sources(const fs::path& root_dir) {
    for (auto const& dir_entry : fs::recursive_directory_iterator(root_dir)) {
        if (dir_entry.is_regular_file()) {
            // स्कैनर खुद अपनी फाइल को स्कैन न करे, वरना यह खुद को ही क्रैश कर देगा
            if (dir_entry.path().filename() == "EngineCoreCompilerWrapper.cpp") {
                continue;
            }

            std::string ext = dir_entry.path().extension().string();
            if (ext == ".cpp" || ext == ".h" || ext == ".hpp" || ext == ".cc") {
                std::ifstream file(dir_entry.path());
                std::string line;
                int line_num = 0;
                
                int brace_depth = 0;
                std::string current_function = "";
                bool inside_hot_loop = false;

                while (std::getline(file, line)) {
                    line_num++;

                    // ब्रेस डेप्थ और स्कोप ट्रैक करो
                    for (char c : line) {
                        if (c == '{') brace_depth++;
                        if (c == '}') {
                            brace_depth--;
                            if (brace_depth <= 0) {
                                current_function = "";
                                inside_hot_loop = false;
                            }
                        }
                    }

                    // मल्टी-लाइन या सिंगल-लाइन फंक्शन सिग्नेचर को कलेक्ट करें
                    if (line.find("void ") != std::string::npos || line.find("int ") != std::string::npos || 
                        line.find("JNIEXPORT") != std::string::npos || line.find("extern \"C\"") != std::string::npos ||
                        line.find("nativeInit") != std::string::npos || line.find("onCreate") != std::string::npos) {
                        current_function += " " + line;
                    }

                    // हॉट लूप / फ्रेम लूप की पहचान
                    if (line.find("while(") != std::string::npos || line.find("for(") != std::string::npos || 
                        line.find("render") != std::string::npos || line.find("update") != std::string::npos) {
                        inside_hot_loop = true;
                    }

                    // चेक करो क्या यह स्टार्टअप/इनिशियलाइजेशन फंक्शन है
                    bool is_init_func = (current_function.find("Init") != std::string::npos || 
                                           current_function.find("constructor") != std::string::npos ||
                                           current_function.find("onCreate") != std::string::npos);

                    // हीप एलोकेशन चेक: स्टार्टअप पर एलाऊ है, लेकिन हॉट लूप या रनटाइम पाथ पर सख्त मना है
                    if (line.find("malloc(") != std::string::npos || line.find("new ") != std::string::npos) {
                        if (!is_init_func || inside_hot_loop) {
                            enforce_system_halt("NATIVE_CPP", "Illegal dynamic heap allocation inside run-time/hot path at line " + std::to_string(line_num), dir_entry.path().string());
                        }
                    }

                    // थ्रेड रेस कंडीशन या अनसेफ सिंक्रोनाइज़ेशन चेक
                    if (line.find("std::thread") != std::string::npos && line.find("detach") != std::string::npos) {
                        enforce_system_halt("NATIVE_CPP", "Unsafe detached thread detected—risk of race condition/dangling pointer at line " + std::to_string(line_num), dir_entry.path().string());
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

// 2. पूरे प्रोजेक्ट की Java और Kotlin फाइलों की स्कैनिंग
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

// 3. Vulkan Shaders की जाँच
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
    std::cout << "[ENGINE MASTER GUARD] Initializing full-project multi-line scope-aware scan...\n";
    
    fs::path project_root = (argc > 1) ? argv[1] : ".";

    scan_native_sources(project_root);
    scan_managed_sources(project_root);
    scan_shader_pipelines(project_root / "shaders");

    std::cout << "[ENGINE MASTER GUARD SUCCESS] Absolute zero errors found. Proceeding to compilation.\n";
    return 0;
}
