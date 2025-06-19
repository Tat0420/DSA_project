#include <iostream>
#include <fstream>
#include <sstream>
#include <unordered_map>
#include <unordered_set>
#include <filesystem>
#include <vector>
#include <ctime>

namespace fs = std::filesystem;
using namespace std;

namespace minigit {

struct Blob {
    string hash;
    string content;
};

struct Commit {
    string hash;
    string parent;
    string parent2; // for merge
    string message;
    time_t timestamp;
    unordered_map<string, string> files; // filename -> blob hash
};

unordered_map<string, string> stagingArea;
unordered_map<string, Commit> commits;
unordered_map<string, string> branches;
string head = "main";

string sha1(const string &content) {
    hash<string> hasher;
    return to_string(hasher(content));
}

void writeFile(const string &path, const string &content) {
    ofstream ofs(path);
    ofs << content;
}

string readFile(const string &path) {
    ifstream ifs(path);
    stringstream buffer;
    buffer << ifs.rdbuf();
    return buffer.str();
}

void init() {
    fs::create_directory(".minigit");
    fs::create_directory(".minigit/objects");
    fs::create_directory(".minigit/refs");
    writeFile(".minigit/HEAD", "main");
    branches["main"] = "";
    cout << "Initialized empty MiniGit repository" << endl;
}

void add(const string &filename) {
    string content = readFile(filename);
    string hash = sha1(content);
    stagingArea[filename] = hash;
    writeFile(".minigit/objects/" + hash, content);
    cout << "Staged " << filename << endl;
}

void commit(const string &msg) {
    Commit c;
    c.timestamp = time(nullptr);
    c.message = msg;
    c.parent = branches[head];
    c.parent2 = "";
    
    if (!c.parent.empty())
        c.files = commits[c.parent].files;

    for (auto &[file, hash] : stagingArea) {
        c.files[file] = hash;
    }

    stringstream ss;
    ss << msg << c.timestamp;
    for (auto &[k, v] : c.files) ss << k << v;
    c.hash = sha1(ss.str());

    commits[c.hash] = c;
    branches[head] = c.hash;
    stagingArea.clear();

    writeFile(".minigit/commits_" + c.hash + ".txt", msg);
    cout << "Committed as " << c.hash << endl;
}

void log() {
    string current = branches[head];
    while (!current.empty()) {
        Commit &c = commits[current];
        cout << "Commit: " << current << endl;
        cout << "Message: " << c.message << endl;
        cout << "Time: " << ctime(&c.timestamp);
        cout << "-----------" << endl;
        current = c.parent;
    }
}

void branch(const string &name) {
    branches[name] = branches[head];
    cout << "Created branch " << name << endl;
}

void checkout(const string &target) {
    string commitHash = branches.count(target) ? branches[target] : target;
    if (!commits.count(commitHash)) {
        cout << "Invalid target" << endl;
        return;
    }

    Commit &c = commits[commitHash];
    for (auto &[file, hash] : c.files) {
        string content = readFile(".minigit/objects/" + hash);
        writeFile(file, content);
    }

    if (branches.count(target)) head = target;
    else writeFile(".minigit/HEAD", commitHash);

    cout << "Checked out to " << target << endl;
}

string findLCA(string a, string b) {
    unordered_set<string> ancestors;
    while (!a.empty()) {
        ancestors.insert(a);
        a = commits[a].parent;
    }
    while (!b.empty()) {
        if (ancestors.count(b)) return b;
        b = commits[b].parent;
    }
    return "";
}

void merge(const string &branchName) {
    if (!branches.count(branchName)) {
        cout << "Branch not found" << endl;
        return;
    }

    string base = findLCA(branches[head], branches[branchName]);
    string current = branches[head];
    string other = branches[branchName];

    Commit merged;
    merged.parent = current;
    merged.parent2 = other;
    merged.timestamp = time(nullptr);
    merged.message = "Merged branch " + branchName;

    unordered_map<string, string> &baseFiles = commits[base].files;
    unordered_map<string, string> &currFiles = commits[current].files;
    unordered_map<string, string> &otherFiles = commits[other].files;

    unordered_set<string> allFiles;
    for (auto &[f, _] : baseFiles) allFiles.insert(f);
    for (auto &[f, _] : currFiles) allFiles.insert(f);
    for (auto &[f, _] : otherFiles) allFiles.insert(f);

    for (const auto &f : allFiles) {
        string baseH = baseFiles[f], currH = currFiles[f], otherH = otherFiles[f];
        if (currH == otherH) merged.files[f] = currH;
        else if (baseH == currH) merged.files[f] = otherH;
        else if (baseH == otherH) merged.files[f] = currH;
        else {
            cout << "CONFLICT: both modified " << f << endl;
            merged.files[f] = currH; // or mark conflict
        }
    }

    stringstream ss;
    ss << merged.message << merged.timestamp;
    for (auto &[k, v] : merged.files) ss << k << v;
    merged.hash = sha1(ss.str());
    commits[merged.hash] = merged;
    branches[head] = merged.hash;

    cout << "Merged into " << head << " as commit " << merged.hash << endl;
}

void diff(const string &a, const string &b) {
    if (!commits.count(a) || !commits.count(b)) {
        cout << "Invalid commit hash" << endl;
        return;
    }
    auto &ca = commits[a].files;
    auto &cb = commits[b].files;
    for (auto &[file, h1] : ca) {
        if (cb.count(file)) {
            string h2 = cb[file];
            if (h1 != h2) {
                string c1 = readFile(".minigit/objects/" + h1);
                string c2 = readFile(".minigit/objects/" + h2);
                cout << "Diff in " << file << ":\n" << c1 << "\n---\n" << c2 << endl;
            }
        }
    }
}

} // namespace minigit

int main() {
    using namespace minigit;
    string cmd;
    while (true) {
        cout << "MiniGit> ";
        cin >> cmd;
        if (cmd == "init") init();
        else if (cmd == "add") {
            string f; cin >> f; add(f);
        }
        else if (cmd == "commit") {
            string msg; getline(cin >> ws, msg); commit(msg);
        }
        else if (cmd == "log") log();
        else if (cmd == "branch") {
            string name; cin >> name; branch(name);
        }
        else if (cmd == "checkout") {
            string t; cin >> t; checkout(t);
        }
        else if (cmd == "merge") {
            string b; cin >> b; merge(b);
        }
        else if (cmd == "diff") {
            string a, b; cin >> a >> b; diff(a, b);
        }
        else if (cmd == "exit") break;
    }
    return 0;
}
