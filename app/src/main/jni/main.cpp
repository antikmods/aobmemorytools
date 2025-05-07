// Extreme Turbo Fast Memory Scanner + Patcher with True Device Power Scaling
// Dev By - @aantik_mods

#define _GNU_SOURCE
#include <unistd.h>
#include <fcntl.h>
#include <dirent.h>
#include <sys/uio.h>
#include <sys/syscall.h>
#include <sys/sysinfo.h>
#include <thread>
#include <vector>
#include <string>
#include <fstream>
#include <sstream>
#include <iostream>
#include <mutex>
#include <atomic>

#ifndef __NR_process_vm_readv
#define __NR_process_vm_readv 310
#endif
#ifndef __NR_process_vm_writev
#define __NR_process_vm_writev 311
#endif

ssize_t process_vm_readv(pid_t pid,
const struct iovec *local_iov,
unsigned long liovcnt,
const struct iovec *remote_iov,
unsigned long riovcnt,
unsigned long flags) {
return syscall(__NR_process_vm_readv, pid, local_iov, liovcnt, remote_iov, riovcnt, flags);
}

ssize_t process_vm_writev(pid_t pid,
const struct iovec *local_iov,
unsigned long liovcnt,
const struct iovec *remote_iov,
unsigned long riovcnt,
unsigned long flags) {
return syscall(__NR_process_vm_writev, pid, local_iov, liovcnt, remote_iov, riovcnt, flags);
}

struct Region {
  uintptr_t start;
  uintptr_t end;
};

struct Patch {
  uintptr_t address;
  std::vector<uint8_t> bytes;
};

std::vector<int> parsePattern(const std::string& patternStr) {
std::istringstream iss(patternStr);
std::string byte;
std::vector<int> pattern;
while (iss >> byte) {
if (byte == "??") pattern.push_back(-1);
else pattern.push_back(std::stoi(byte, nullptr, 16));
}
return pattern;
}

bool match(const uint8_t* data, const std::vector<int>& pattern) {
for (size_t i = 0; i < pattern.size(); ++i) {
if (pattern[i] != -1 && data[i] != pattern[i])
return false;
}
return true;
}

std::vector<Region> getMemoryRegions(pid_t pid) {
std::vector<Region> regions;
std::ifstream maps("/proc/" + std::to_string(pid) + "/maps");
std::string line;
while (std::getline(maps, line)) {
uintptr_t start, end;
char perms[5];
if (sscanf(line.c_str(), "%lx-%lx %4s", &start, &end, perms) == 3) {
if (perms[0] == 'r') {
regions.push_back({start, end});
}
}
}
return regions;
}

pid_t getPID(const std::string& package) {
DIR* dir = opendir("/proc");
struct dirent* entry;
while ((entry = readdir(dir))) {
if (!isdigit(*entry->d_name)) continue;
std::ifstream cmd("/proc/" + std::string(entry->d_name) + "/cmdline");
std::string proc;
std::getline(cmd, proc, '\0');
if (proc == package) {
closedir(dir);
return atoi(entry->d_name);
}
}
closedir(dir);
return -1;
}

void scanRegion(pid_t pid, Region region, const std::vector<int>& search, const std::vector<int>& replace, std::vector<Patch>& patches) {
    
	
	size_t size = region.end - region.start;
    const size_t chunkSize = 8 * 1024 * 1024;
    uintptr_t addr = region.start;
    while (addr < region.end) {
    size_t toRead = std::min(chunkSize, region.end - addr);
    std::vector<uint8_t> buffer(toRead);
     
	struct iovec local = { buffer.data(), toRead };
    struct iovec remote = { (void*)addr, toRead };

   if (process_vm_readv(pid, &local, 1, &remote, 1, 0) <= 0) {
   addr += toRead;
   continue;
   }

        for (size_t i = 0; i <= buffer.size() - search.size(); ++i) {
            
			if (match(&buffer[i], search)) {
                
				Patch p;
             
				p.address = addr + i;
            
				for (size_t j = 0; j < replace.size(); ++j) {
                if (replace[j] == -1) p.bytes.push_back(buffer[i+j]);
                else p.bytes.push_back((uint8_t)replace[j]);
                }
                patches.push_back(p);
            }
        }

        addr += toRead;
    }
}

void writePatches(pid_t pid, const std::vector<Patch>& patches) {
const size_t batchSize = 50;
for (size_t i = 0; i < patches.size(); i += batchSize) {
size_t end = std::min(patches.size(), i + batchSize);
std::vector<struct iovec> locals, remotes;
for (size_t j = i; j < end; ++j) {
locals.push_back({ (void*)patches[j].bytes.data(), patches[j].bytes.size() });
remotes.push_back({ (void*)patches[j].address, patches[j].bytes.size() });
}
process_vm_writev(pid, locals.data(), locals.size(), remotes.data(), remotes.size(), 0);
}
}

int main(int argc, char* argv[]) {
    if (argc != 4) {
    std::cerr << "./memory <package> <search> <replace>\n";
    return 1;
    }

    std::string package = argv[1];
    auto search = parsePattern(argv[2]);
    auto replace = parsePattern(argv[3]);

    pid_t pid = getPID(package);
    if (pid == -1) {
    //std::cerr << "process not found :)" << std::endl;
    return 1;
    }

    auto regions = getMemoryRegions(pid);
    // --- Autonomic Thread Decider Haha Some Nop Device Not Handle Big Cores ---
    int cores = sysconf(_SC_NPROCESSORS_ONLN); 
	// Use sysconf Karnel
    int threads = std::max(4, cores * 2);
    // ----------------------------------
    std::vector<std::thread> workers;
    std::vector<std::vector<Patch>> allPatches(threads);
    std::atomic<size_t> index(0);

    for (int t = 0; t < threads; ++t) {
    workers.emplace_back([&, t]() {
    while (true) {
    size_t i = index++;
    if (i >= regions.size()) break;
    scanRegion(pid, regions[i], search, replace, allPatches[t]);
    }
    });
    }

    for (auto& th : workers) th.join();
    std::vector<Patch> finalPatches;
    for (auto& vec : allPatches) {
    finalPatches.insert(finalPatches.end(), vec.begin(), vec.end());
    }
    writePatches(pid, finalPatches);
    std::cout << "Patching completed" << finalPatches.size() << "Success" << std::endl;
    return 0;
}
