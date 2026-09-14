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
    int global_ref_created = 0;
    int global_ref_destroyed = 0;

    void trigger_violation(clang::Stmt *StmtPtr, const std::string &msg) {
        g_ast_violation_found = true;
        clang::SourceLocation Loc = StmtPtr->getBeginLoc();
        clang::FullSourceLoc FullLoc(Loc, ASTContextPtr->getSourceManager());
        unsigned int line_num = FullLoc.isValid() ? FullLoc.getSpellingLineNumber() : 0;
        enforce_system_halt("IRONCLAD_AST_ANALYZER", msg, g_current_scan_file, line_num);
    }

    void trigger_violation_decl(clang::Decl *DeclPtr, const std::string &msg) {
        g_ast_violation_found = true;
        clang::SourceLocation Loc = DeclPtr->getBeginLoc();
        clang::FullSourceLoc FullLoc(Loc, ASTContextPtr->getSourceManager());
        unsigned int line_num = FullLoc.isValid() ? FullLoc.getSpellingLineNumber() : 0;
        enforce_system_halt("IRONCLAD_AST_ANALYZER", msg, g_current_scan_file, line_num);
    }

public:
    explicit IroncladEngineSafetyVisitor(clang::ASTContext *Context) : ASTContextPtr(Context) {}

    bool VisitCXXNewExpr(clang::CXXNewExpr *node) {
        if (node) {
            trigger_violation(node, "Forbidden raw C++ 'new' operator detected. Enforcing safe memory arenas.");
        }
        return true;
    }

    bool VisitVarDecl(clang::VarDecl *node) {
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
                    "-isystem", "/usr/lib/llvm-18/lib/clang/18/include",
                    "-target", "aarch64-none-linux-android26"
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

int main(int argc, char* argv[]) {
    std::cout << "[IRONCLAD ENGINE GUARD] Initializing full enforcement pipeline...\n";
    fs::path project_root = (argc > 1) ? argv[1] : ".";
    scan_native_sources(project_root);
    std::cout << "[ENGINE GUARD SUCCESS] Complete verification sequence passed. Zero exceptions found.\n";
    return 0;
}
