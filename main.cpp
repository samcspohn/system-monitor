#include <fstream>
#include <iostream>
#include <string>
#include <sstream>
#include <vector>
#include <filesystem>

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>

namespace fs = std::filesystem;

using namespace std;

vector<string> split_str(string& s){
    stringstream ss(s);
    string r;
    vector<string> ret;
    while(getline(ss,r,' ')){
        ret.push_back(r);
    }
    return ret;
}
vector<string> getStats(string s){
    ifstream f(s + "/stat");
    vector<string> ret;
    string r;
    while(getline(f,r,' ')){
        ret.push_back(r);
    }
    return ret;

}

string getCmdLine(string p){
    // cout << p + "/cmdline" << endl;
    ifstream f = ifstream("/proc/" + p + "/cmdline");
    string s;
    vector<string> v;
    while(getline(f,s,'\0')){
        // cout << s << endl;
        v.push_back(s);
    }
    s = "";
    for(string& r : v){
        s += r + " ";
    }
    return s;
}
int main(int argc, char** argv){
    ifstream f = ifstream("/proc/stat");

    string s;
    vector<string> lines;
    while(getline(f,s)){
        if(s.find("cpu") != -1)
            lines.push_back(s);
    }

    for(auto &i : lines){
        cout << i << endl;
    }

    cout << "######################" << endl;
    std::string path = "/proc";
    int count = 0;
    vector<vector<string>> procs;
    for (const auto & entry : fs::directory_iterator(path)){
        // cout << entry.path().stem().string() << endl;
        // if(entry.path().stem().string()[0] >= '0' && entry.path().stem().string()[0] <= '9')
        //     std::cout << count++ << entry.path() << std::endl;
        //     system(string("cat " + string(entry.path()) + "/stat").c_str());
        if(entry.path().stem().string()[0] >= '0' && entry.path().stem().string()[0] <= '9'){
            // cout << entry.path() << endl;
            procs.push_back(getStats(entry.path()));
            procs.back().push_back(getCmdLine(procs.back()[0]));
        }
    }

    for(auto &i : procs){
        cout << i[0] << " : " << i[1] << " : " << i.back() << endl;

        // if(i[1] == "(game)"){
        //     for(auto &j : i)
        //         cout << j;
        //     cout << endl;
        // }
    }
}