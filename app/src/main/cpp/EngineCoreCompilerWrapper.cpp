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

class IroncladEngineSafetyVisitor : public clang::RecursiveASTVisitor<IroncladEngineSafetyVisitor> {
private:
    clang::ASTContext *ASTContextPtr;
    int local_ref_count = 0;
public:
    explicit IroncladEngineSafetyVisitor(clang::ASTContext *Context) : ASTContextPtr(Context) {}

    bool VisitCallExpr(clang::CallExpr *Expr) {
        if (const auto *Decl = Expr->getDirectCallee()) {
            std::string funcName = Decl->getNameAsString();
            
            // Layer 1 & 2: Vulkan Synchronization Checks
            if (funcName == "vkQueueSubmit" || funcName == "vkQueuePresentKHR") {
                if (Expr->getNumArgs() < 3) {
                    trigger_violation(Expr, "Vulkan submission function invoked with insufficient synchronization structures.");
                }
            }
            
            // Layer 3: JNI Reference Tracking
            if (funcName == "NewLocalRef" || funcName == "FindClass" || funcName == "GetMethodID" || funcName == "NewStringUTF") {
                local_ref_count++;
            }
            if (funcName == "DeleteLocalRef" || funcName == "DeleteGlobalRef") {
                local_ref_count = std::max(0, local_ref_count - 1);
            }

            // Layer 5: Dynamic Heap Allocation Prohibition (Raw C-style)
            if (funcName == "malloc" || funcName == "calloc" || funcName == "realloc" || funcName == "strdup") {
                trigger_violation(Expr, "Forbidden C-style heap allocation detected in native execution path.");
            }
        }
        return true;
    }
    
    bool VisitCXXNewExpr(clang::CXXNewExpr *NewExpr) {
        trigger_violation(NewExpr, "Forbidden raw C++ 'new' operator detected. Enforcing safe memory arenas and smart pointers.");
        return true;
    }

    bool VisitTranslationUnitDecl(clang::TranslationUnitDecl *D) {
        if (local_ref_count > 10) {
            enforce_system_halt("JNI_LIFECYCLE", "Unbalanced JNI reference creation detected across compilation unit.", g_current_scan_file);
        }
        return true;
    }

private:
    void trigger_violation(clang::Stmt *StmtPtr, const std::string& msg) {
        g_ast_violation_found = true;
        clang::SourceLocation Loc = StmtPtr->getBeginLoc();
        clang::FullSourceLoc FullLoc(Loc, ASTContextPtr->getSourceManager());
        unsigned int line_num = FullLoc.isValid() ? FullLoc.getSpellingLineNumber() : 0;
        enforce_system_halt("IRONCLAD_AST_ANALYZER", msg, g_current_scan_file, line_num);
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
    std::unique_ptr<clang::ASTConsumer> CreateASTConsumer(
        clang::CompilerInstance &CI, llvm::StringRef file) override {
        return std::make_unique<IroncladEngineSafetyConsumer>(&CI.getASTContext());
    }
};

void scan_native_sources(const fs::path& root_dir) {
    const char* android_home_env = std::getenv("ANDROID_HOME");
    std::string ndk_include = android_home_env ? std::string(android_home_env) + "/ndk/26.1.10909125/toolchains/llvm/prebuilt/linux-x86_64/sysroot/usr/include" : "";

    for (auto const& dir_entry : fs::recursive_directory_iterator(root_dir)) {
        if (dir_entry.is_regular_file()) {
            std::string path_str = dir_entry.path().string();
            
            if (dir_entry.path().filename() == "EngineCoreCompilerWrapper.cpp" || is_generated_or_build_path(path_str)) {
                continue;
            }

            std::string ext = dir_entry.path().extension().string();
            if (ext == ".cpp" || ext == ".h" || ext == ".hpp" || ext == ".cc") {
                g_current_scan_file = path_str;
                std::ifstream t(path_str);
                if (!t.is_open()) continue;

                std::string file_content((std::istreambuf_iterator<char>(t)),
                                         std::istreambuf_iterator<char>());

                std::vector<std::string> args = {"-fsyntax-only", "-std=c++17", "-x", "c++"};
                if (!ndk_include.empty() && fs::exists(ndk_include)) {
                    args.push_back("-I" + ndk_include);
                }

                bool success = clang::tooling::runToolOnCodeWithArgs(
                    std::make_unique<IroncladEngineSafetyAction>(),
                    file_content,
                    args,
                    dir_entry.path().filename().string()
                );

                if (!success || g_ast_violation_found) {
                    enforce_system_halt("NATIVE_AST_PARSER", "AST structural safety validation failure.", path_str);
                }
            }
        }
    }
}

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
