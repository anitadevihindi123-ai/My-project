import os
import re

# प्रोजेक्ट के सभी संभावित लूपहोल्स, एरर्स और अधूरी चीज़ों को पकड़ने के लिए मास्टर रूल्स
COMPREHENSIVE_RULES = [
    {
        "category": "JNI / Native Library Loading Error Risk",
        "regex": r"(System\.loadLibrary|dlopen|dlsym)",
        "desc": "Check if native library name matches exact .so file name and handle UnsatisfiedLinkError exceptions properly."
    },
    {
        "category": "Incomplete Code / Placeholder / Stub Risk",
        "regex": r"(TODO|FIXME|throw\s+new\s+UnsupportedOperationException|NotImplementedError|pass\s*//)",
        "desc": "Incomplete implementation found. This stub/placeholder will cause crashes or missing logic at runtime."
    },
    {
        "category": "Vulkan VRAM Memory Allocation & Leak Risk",
        "regex": r"(vkAllocateMemory|vkCreateBuffer|vkCreateImage|vkCreateImageView)",
        "desc": "Vulkan resource allocated. Ensure a corresponding free/destroy function exists in the cleanup lifecycle."
    },
    {
        "category": "Vulkan Command Buffer / Performance Bottleneck",
        "regex": r"(vkBeginCommandBuffer|vkQueueSubmit|vkResetCommandBuffer)",
        "desc": "Performance Warning: Avoid recreating command buffers per frame inside the core rendering/compute loop."
    },
    {
        "category": "Tensor & Multi-Threading Data Corruption Risk",
        "regex": r"(ncnn::Mat|memcpy|vkMapMemory)",
        "desc": "Multi-threading/Tensor access detected. Ensure thread synchronization (std::mutex) is used to avoid race conditions."
    },
    {
        "category": "Android Manifest & Component Declaration Flaw",
        "regex": r"(<uses-permission|<activity|<service|<application)",
        "desc": "Verify permissions, hardware acceleration, and exported component flags are correctly configured."
    }
]

# जिन एक्सटेंशन और फोल्डर्स को स्कैन से बाहर रखना है (NCNN मॉडल्स और सिस्टम फोल्डर्स)
EXCLUDED_EXTENSIONS = {".param", ".bin", ".weights", ".tflite", ".onnx", ".png", ".jpg", ".jpeg", ".so", ".zip", ".keystore", ".gradle"}
EXCLUDED_DIRS = {".git", "build", ".github", ".gradle", "assets", "native-libs"}

def scan_entire_file_line_by_line(file_path):
    file_ext = os.path.splitext(file_path)[1].lower()
    file_name = os.path.basename(file_path)

    if file_ext in EXCLUDED_EXTENSIONS:
        return False

    try:
        with open(file_path, 'r', encoding='utf-8', errors='ignore') as f:
            lines = f.readlines()
    except Exception:
        return False
        
    issues_found = False

    # 🔍 लाइन 1 से लेकर आखिरी लाइन तक पूरा स्कैन
    for line_num, line in enumerate(lines, 1):
        stripped = line.strip()
        
        # कमेंट्स या खाली लाइनों को छोड़ दें
        if not stripped or stripped.startswith("//") or stripped.startswith("/*") or stripped.startswith("*") or stripped.startswith("<!--") or stripped.startswith("#"):
            continue

        for rule in COMPREHENSIVE_RULES:
            if re.search(rule["regex"], line):
                print(f"🔴 [FULL SCAN ALERT] File: {file_path}")
                print(f"   🔢 Line Number     : {line_num}")
                print(f"   ⚠️ Vulnerability   : {rule['category']}")
                print(f"   💻 Flagged Code    : {stripped}")
                print(f"   💡 Manual Fix Guide: {rule['desc']}")
                print("-" * 75)
                issues_found = True

    return issues_found

def main():
    print("🚀 Starting Full-Project Line-by-Line Comprehensive Auditor...")
    total_files_scanned = 0
    total_issues_files = 0
    
    for root, dirs, files in os.walk("."):
        # सिस्टम फोल्डर और 'assets' (NCNN मॉडल्स) को सुरक्षित रखना
        dirs[:] = [d for d in dirs if d not in EXCLUDED_DIRS]
        
        for file in files:
            full_path = os.path.join(root, file)
            total_files_scanned += 1
            if scan_entire_file_line_by_line(full_path):
                total_issues_files += 1
                
    print(f"\n📂 Scan Completed: Scanned {total_files_scanned} files across the entire repository.")
    if total_issues_files > 0:
        print(f"❌ AUDIT FAILED: Found structural flaws, memory risks, or incomplete code in {total_issues_files} files.")
        print("🛠️ Review every line number above and apply manual fixes with total precision.")
        exit(1)
    else:
        print("\n✨ SUCCESS: Full-project line-by-line audit found zero vulnerabilities or loopholes!")

if __name__ == "__main__":
    main()
