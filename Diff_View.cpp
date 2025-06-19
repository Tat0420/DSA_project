#include <iostream>
#include <fstream>
#include <unordered_map>
#include <filesystem>

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

string readFile(const string &path) {
    ifstream ifs(path);
    stringstream buffer;
    buffer << ifs.rdbuf();
    return buffer.str();
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