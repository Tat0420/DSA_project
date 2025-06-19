#include <iostream>
#include <sstream>
#include <unordered_map>
#include <unordered_set>

using namespace std;

struct Commit {
    string hash;
    string parent;
    string parent2; // for merge
    string message;
    time_t timestamp;
    unordered_map<string, string> files; // filename -> blob hash
};

unordered_map<string, Commit> commits;
unordered_map<string, string> branches;
string head = "main";

string sha1(const string &content) {
    hash<string> hasher;
    return to_string(hasher(content));
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