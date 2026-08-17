import os
import re

# मास्टर चेकिंग और परफॉरमेंस ऑप्टिमाइज़ेशन रूल्स
CHECKS = [
    {
        "target": ["AndroidManifest.xml"],
        "name": "Missing or Loose Android Permission/Component",
        "regex": r"(<uses-permission|<activity|<service)",
        "desc": "Check if hardware acceleration or required hardware features are explicitly declared."
    },
    {
        "target": [".cpp", ".h", ".cc"],
        "name": "Vulkan Memory / Buffer Leak Risk (vkAllocateMemory)",
        "regex": r"(vkAllocateMemory|vkCreateBuffer|vkCreateImage)",
        "desc": "Vulkan memory allocated. Ensure corresponding vkFreeMemory or vkDestroyBuffer is called to prevent VRAM leaks on GPU."
    },
    {
        "target": [".cpp", ".h", ".cc"],
        "name": "Vulkan Command Buffer Reset/Submit Bottleneck",
        "regex": r"(vkBeginCommandBuffer|vkQueueSubmit)",
        "desc": "Performance Tip: Avoid recreating command buffers every frame. Reuse them from a command pool to maximize GPU throughput."
    },
    {
        "target": [".java", ".kt"],
        "name": "JNI Native Call Performance / Memory Risk",
        "regex": r"(System\.loadLibrary|native\s+void|native\s+long)",
        "desc": "Ensure heavy tensor/Vulkan structures passed via JNI use Direct ByteBuffers to avoid garbage collector overhead."
    }
]

def analyze_file(file_path):
    with open(file_path, 'r', encoding='utf-8', errors='ignore') as f:
        lines = f.readlines()
        
    issues_found = False
    file_ext = os.path.splitext(file_path)[1]
    file_name = os.path.basename(file_path)

    for line_num, line in enumerate(lines, 1):
        for check in CHECKS:
            # चेक करें कि क्या यह फाइल इस रूल के टारगेट में है
            if file_ext in check["target"] or file_name in check["target"]:
                if re.search(check["regex"], line):
                    # कमेंट्स को इग्नोर करें
                    if line.strip().startswith("//") or line.strip().startswith("/*"):
                        continue
                        
                    print(f"🔴 [AUDIT ALERT] File: {file_path}")
                    print(f"   🔢 Line Number : {line_num}")
                    print(f"   ⚠️ Issue/Area  : {check['name']}")
                    print(f"   💻 Code Line   : {line.strip()}")
                    print(f"   💡 Pro-Tip/Fix : {check['desc']}")
                    print("-" * 65)
                    issues_found = True
                    
    return issues_found

def main():
    print("🚀 Starting Master Vulkan, JNI & Manifest Deep Audit...")
    total_issues = 0
    
    for root, dirs, files in os.walk("."):
        if ".git" in root or "build" in root or ".github" in root or ".gradle" in root:
            continue
        for file in files:
            full_path = os.path.join(root, file)
            if analyze_file(full_path):
                total_issues += 1
                
    if total_issues > 0:
        print(f"\n❌ AUDIT FAILED: Found potential vulnerabilities, leaks or optimization flaws in {total_issues} files.")
        print("💡 Fix the highlighted lines above to make your Vulkan/NCNN pipeline bulletproof.")
        exit(1)
    else:
        print("\n✨ SUCCESS: Project architecture, Manifest, and Vulkan memory hooks look completely solid!")

if __name__ == "__main__":
    main()
