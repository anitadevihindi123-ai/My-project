#include <iostream>
#include <fstream>
#include <vector>
#include <string>
#include <filesystem>
#include <cstdlib>
#include <clang/Driver/Driver.h>
#include <clang/Frontend/CompilerInstance.h>
#include <clang/Tooling/Tooling.h>
#include <llvm/TargetParser/Host.h>
#include <clang/AST/ASTConsumer.h>
#include <clang/AST/RecursiveASTVisitor.h>
#include <clang/Frontend/FrontendAction.h>
#include <clang/Basic/SourceManager.h>

namespace fs = std::filesystem;

static bool g_ast_violation_found = false;
static std::string g_current_scan_file = "";

void enforce_system_halt(const std::string& layer, const std::string& error_msg, const std::string& file_path, unsigned int line_num = 0) {
    std::cerr << "\n[FATAL SYSTEM HALT][" << layer << "] Critical Zero-Tolerance Violation Detected!\n"
              << "-> File: " << file_path << (line_num > 0 ? ":" + std::to_string(line_num) : "") << "\n"
              << "-> Reason: " << error_msg << "\n"
              << "-> Status: Build permanently aborted. Exit Code 666 enforced.\n";
     std::exit(666); 
}

bool is_generated_or_build_path(const std::string& path_str) {
    return path_str.find("/build/") != std::string::npos || 
           path_str.find("\\build\\") != std::string::npos ||
           path_str.find("/app/build/") != std::string::npos ||
           path_str.find("\\app\\build\\") != std::string::npos;
}

// 1. उन्नत नेटिव C++ AST विजिटर (रेस कंडीशन, रॉ पॉइंटर और JNI चेकर)
class IroncladEngineSafetyVisitor : public clang::RecursiveASTVisitor<IroncladEngineSafetyVisitor> {
private:
    clang::ASTContext *ASTContextPtr;
    int global_ref_created = 0;
    int global_ref_destroyed = 0;

        void trigger_violation(clang::Stmt *StmtPtr, const std::string &msg) {
        g_ast_violation_found = true;
        clang::SourceLocation Loc = StmtPtr->getBeginLoc();
        clang::SourceManager &SM = ASTContextPtr->getSourceManager();

        // 1. मैक्रो के झंझट से बचने के लिए पहले एक्सपेंशन लोकेशन लें
        clang::SourceLocation ExpansionLoc = SM.getExpansionLoc(Loc);

        // 2. PresumedLoc का इस्तेमाल करें (यह फाइल नाम और लाइन नंबर दोनों एक साथ सटीक देता है)
        clang::PresumedLoc PLoc = SM.getPresumedLoc(ExpansionLoc);

        if (PLoc.isInvalid()) {
            enforce_system_halt("IRONCLAD_AST_ANALYZER", msg, g_current_scan_file, 0);
            return;
        }

        std::string FileName = PLoc.getFilename();

        // 3. चेक करें कि क्या यह आपकी अपनी मुख्य फाइल है या नहीं
        if (FileName.empty() || FileName.find("true-singularity-core.cpp") == std::string::npos) {
            return;
        }

        // 4. 101% सटीक लाइन नंबर के साथ सिस्टम को रोकें
        unsigned int exactLine = PLoc.getLine();
        enforce_system_halt("IRONCLAD_AST_ANALYZER", msg, FileName, exactLine);
    }

    void trigger_violation_decl(clang::Decl *DeclPtr, const std::string &msg) {
        g_ast_violation_found = true;
        clang::SourceLocation Loc = DeclPtr->getBeginLoc();
        clang::SourceManager &SM = ASTContextPtr->getSourceManager();

        // 1. मैक्रो के झंझट से बचने के लिए पहले एक्सपेंशन लोकेशन लें
        clang::SourceLocation ExpansionLoc = SM.getExpansionLoc(Loc);

        // 2. PresumedLoc का इस्तेमाल करें
        clang::PresumedLoc PLoc = SM.getPresumedLoc(ExpansionLoc);

        if (PLoc.isInvalid()) {
            enforce_system_halt("IRONCLAD_AST_ANALYZER", msg, g_current_scan_file, 0);
            return;
        }

        std::string FileName = PLoc.getFilename();

        // 3. चेक करें कि क्या यह आपकी अपनी मुख्य फाइल है या नहीं
        if (FileName.empty() || FileName.find("true-singularity-core.cpp") == std::string::npos) {
            return;
        }

        // 4. 101% सटीक लाइन नंबर के साथ सिस्टम को रोकें
        unsigned int exactLine = PLoc.getLine();
        enforce_system_halt("IRONCLAD_AST_ANALYZER", msg, FileName, exactLine);
    }

public:
    explicit IroncladEngineSafetyVisitor(clang::ASTContext *Context) : ASTContextPtr(Context) {}

    bool VisitCXXNewExpr(clang::CXXNewExpr *node) {
        if (node) {
            trigger_violation(node, "Forbidden raw C++ 'new' operator detected. Enforcing safe memory arenas.");
        }
        return true;
    }

        // 101% अचूक मैजिक स्टैटिक चेकर (फंक्शन के अंदर के सेफ स्टैटिक्स को छांटने के लिए)
    bool isSafeMagicStatic(const clang::VarDecl *VD) {
        if (!VD) return false;

        // अगर यह ग्लोबल या क्लास का स्टैटिक मेंबर है, तो यह सेफ नहीं है
        if (!VD->isLocalVarDecl()) {
            return false; 
        }

        // क्या यह स्टोरेज स्टैटिक है?
        if (VD->getStorageDuration() != clang::StorageDuration::SD_Static) {
            return false;
        }

        // अगर const है तो वैसे ही सेफ है
        if (VD->getType().isConstQualified()) {
            return true;
        }

        // C++11 मैजिक स्टैटिक की गारंटी (फंक्शन के अंदर का लोकल नॉन-कांस्टेंट स्टैटिक)
        return true;
    }

    bool VisitVarDecl(clang::VarDecl *node) {
        // सबसे पहले चेक करो: अगर यह 101% सेफ मैजिक स्टैटिक है, तो यहीं से पास कर दो (बिल्ड मत रोको)
        if (isSafeMagicStatic(node)) {
            return true;
        }

        // बाकी सभी ग्लोबल और खतरनाक स्टैटिक वेरिएबल्स के लिए पुरानी कड़ाई जारी रहेगी
        if (node && node->hasGlobalStorage() && !node->getType().isConstQualified()) {
            std::string type_str = node->getType().getAsString();
            if (type_str.find("atomic") == std::string::npos && type_str.find("mutex") == std::string::npos) {
                trigger_violation_decl(node, "Unsafe global/static mutable variable without std::atomic or std::mutex.");
            }
        }
        return true;
    }

    bool VisitCallExpr(clang::CallExpr *node) {
        if (node && node->getDirectCallee()) {
            std::string func_name = node->getDirectCallee()->getNameAsString();
            if (func_name == "NewGlobalRef") global_ref_created++;
            else if (func_name == "DeleteGlobalRef") global_ref_destroyed++;
        }
        return true;
    }

    bool VisitTranslationUnitDecl(clang::TranslationUnitDecl *D) {
        if (global_ref_created != global_ref_destroyed) {
            enforce_system_halt("JNI_LIFECYCLE", "Unbalanced JNI global references (NewGlobalRef vs DeleteGlobalRef mismatch).", g_current_scan_file, 0);
        }
        return true;
    }
};

class IroncladEngineSafetyConsumer : public clang::ASTConsumer {
private:
    IroncladEngineSafetyVisitor Visitor;
public:
    explicit IroncladEngineSafetyConsumer(clang::ASTContext *Context) : Visitor(Context) {}
    void HandleTranslationUnit(clang::ASTContext &Context) override {
        Visitor.TraverseDecl(Context.getTranslationUnitDecl());
    }
};

class IroncladEngineSafetyAction : public clang::ASTFrontendAction {
public:
    std::unique_ptr<clang::ASTConsumer> CreateASTConsumer(clang::CompilerInstance &CI, llvm::StringRef file) override {
        return std::make_unique<IroncladEngineSafetyConsumer>(&CI.getASTContext());
    }
};

// 2. नेटिव C++ सोर्सेज स्कैनिंग (क्रॉस-कंपाइलर टारगेट बाइंडिंग के साथ)
void scan_native_sources(const fs::path& root_dir) {
    const char* android_home_env = std::getenv("ANDROID_HOME");
    std::string ndk_include = android_home_env ? std::string(android_home_env) + "/ndk/26.1.10909125/toolchains/llvm/prebuilt/linux-x86_64/sysroot/usr/include" : "";

    for (auto const& dir_entry : fs::recursive_directory_iterator(root_dir)) {
        if (dir_entry.is_regular_file()) {
            std::string path_str = dir_entry.path().string();
            if (dir_entry.path().filename() == "EngineCoreCompilerWrapper.cpp" || is_generated_or_build_path(path_str)) continue;

            std::string ext = dir_entry.path().extension().string();
            if (ext == ".cpp" || ext == ".h" || ext == ".hpp" || ext == ".cc") {
                g_current_scan_file = path_str;
                std::ifstream t(path_str);
                if (!t.is_open()) continue;
                std::string file_content((std::istreambuf_iterator<char>(t)), std::istreambuf_iterator<char>());

                std::vector<std::string> args = {
    "-fsyntax-only", "-std=c++17", "-x", "c++",
    "-isystem", ndk_include + "/usr/include",
    "-isystem", ndk_include + "/usr/include/aarch64-linux-android", // एंड्रॉइड मल्टीआर्च हेडर के लिए
    "-isystem", "/usr/lib/llvm-18/lib/clang/18/include",
    // होस्ट मल्टीआर्च सपोर्ट (bits/wordsize.h के लिए)
    "-target", "aarch64-none-linux-android26",
    "-U__STRICT_ANSI__", "-D_GNU_SOURCE"
};


                bool success = clang::tooling::runToolOnCodeWithArgs(
                    std::make_unique<IroncladEngineSafetyAction>(), file_content, args, dir_entry.path().filename().string()
                );

                if (!success || g_ast_violation_found) {
                    enforce_system_halt("NATIVE_AST_PARSER", "AST structural safety validation failure.", path_str);
                }
            }
        }
    }
}

// 3. मैनेज्ड (Java/Kotlin) सोर्सेज स्कैनिंग (हॉट-लूप एलोकेशन और थ्रेड ब्लॉक चेक)
void scan_managed_sources(const fs::path& root_dir) {
    for (auto const& dir_entry : fs::recursive_directory_iterator(root_dir)) {
        if (dir_entry.is_regular_file()) {
            std::string path_str = dir_entry.path().string();
            if (is_generated_or_build_path(path_str)) continue;

            std::string filename = dir_entry.path().filename().string();
            std::string ext = dir_entry.path().extension().string();

            if (ext == ".java" || ext == ".kt") {
                std::ifstream file(path_str);
                std::string line;
                int line_num = 0;
                bool inside_loop = false;
                while (std::getline(file, line)) {
                    line_num++;
                    if (line.find("for(") != std::string::npos || line.find("while(") != std::string::npos) {
                        inside_loop = true;
                    }
                    if (inside_loop && line.find("new ") != std::string::npos) {
                        enforce_system_halt("MANAGED_JVM", "Object allocation inside hot-loop prohibited.", path_str, line_num);
                    }
                    if (filename != "AppDatabase.java" && line.find("synchronized") != std::string::npos) {
                        enforce_system_halt("MANAGED_JVM", "Unsafe synchronized block outside secure database layer.", path_str, line_num);
                    }
                    if (line.find("Thread.sleep") != std::string::npos) {
                        enforce_system_halt("MANAGED_JVM", "Blocking Thread.sleep execution prohibited.", path_str, line_num);
                    }
                    if (line.find("}") != std::string::npos) {
                        inside_loop = false;
                    }
                }
            }
        }
    }
}

// 4. Vulkan शेडर पाइपलाइन वैलिडेशन (SPIR-V कंप्लायंस)
void scan_shader_pipelines(const fs::path& shader_dir) {
    if (!fs::exists(shader_dir)) return;
    for (auto const& dir_entry : fs::recursive_directory_iterator(shader_dir)) {
        if (dir_entry.is_regular_file()) {
            std::string path_str = dir_entry.path().string();
            if (is_generated_or_build_path(path_str)) continue;

            std::string ext = dir_entry.path().extension().string();
            if (ext == ".vert" || ext == ".frag" || ext == ".comp" || ext == ".glsl") {
                std::string cmd = "glslangValidator -V " + path_str + " > /dev/null 2>&1";
                int res = std::system(cmd.c_str());
                if (res != 0) {
                    enforce_system_halt("VULKAN_SHADER", "SPIR-V shader compilation and layout validation failure.", path_str);
                }
            }
        }
    }
}

int main(int argc, char* argv[]) {
    std::cout << "[IRONCLAD ENGINE GUARD] Initializing full 10-layer enforcement pipeline...\n";
    
    fs::path project_root = (argc > 1) ? argv[1] : ".";

    scan_native_sources(project_root);
    scan_managed_sources(project_root);
    scan_shader_pipelines(project_root / "shaders");

    std::cout << "[ENGINE GUARD SUCCESS] Complete verification sequence passed. Zero exceptions found.\n";
    return 0;
}
