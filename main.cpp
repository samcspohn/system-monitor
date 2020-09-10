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
#include <map>
#include <unordered_map>
#include <unordered_set>
#include <thread>
#include <list>
#include <algorithm>
#include <chrono>
namespace fs = std::filesystem;

using namespace std;
using namespace chrono;

vector<string> split_str(string &s, char delim = ' ')
{
  stringstream ss(s);
  string r;
  vector<string> ret;
  while (getline(ss, r, ' '))
  {
    if (r != "")
      ret.push_back(r);
  }
  return ret;
}
vector<string> getStats(string s)
{
  ifstream f(s + "/stat");
  vector<string> ret;
  string r;
  while (getline(f, r, ' '))
  {
    ret.push_back(r);
  }
  return ret;
}

string getCmdLine(string p)
{
  // std::cout << p + "/cmdline" << endl;
  ifstream f = ifstream("/proc/" + p + "/cmdline");
  string s;
  vector<string> v;
  while (getline(f, s, '\0'))
  {
    // std::cout << s << endl;
    v.push_back(s);
  }
  s = "";
  for (string &r : v)
  {
    s += r + " ";
  }
  return s;
}

class timer
{
  high_resolution_clock::time_point tstart;

public:
  void start();
  float stop();
};

void timer::start()
{
  tstart = high_resolution_clock::now();
}
float timer::stop()
{
  return (float)duration_cast<microseconds>(high_resolution_clock::now() - tstart).count() / 1000.0;
}

enum proc_enum
{
  pid = 0,
  comm,
  state,
  ppid,
  pgrp,
  session,
  tty_nr,
  tpgid,
  flags,
  minflt,
  cminflt,
  majflt,
  cmajflt,
  utime,
  stime,
  cutime,
  cstime,
  priority,
  _nice,
  num_threads,
  itrealvalue,
  starttime,
  vsize,
  rss,
  rsslim,
  startcode,
  endcode,
  startstack,
  kstkesp,
  kstkeip,
  signal,
  blocked,
  sigignore,
  sigcatch,
  wchan,
  nswap,
  cnswap,
  exit_signal,
  processor,
  rt_priority,
  policy,
  delayacct_blkio_ticks,
  guest_time,
  cguest_time,
  start_data,
  end_data,
  start_brk,
  arg_start,
  arg_end,
  env_start,
  env_end,
  exit_code
};
enum cpu_usage_enum
{
  user = 1
};

enum class net_recieve
{
  bytes = 1,
  packets,
  errs,
  drop,
  fifo,
  frame,
  compressed,
  multicast
};
enum class net_transmit
{
  bytes = (int)net_recieve::multicast + 1,
  packets,
  errs,
  drop,
  fifo,
  colls,
  carrier,
  compressed
};

struct cpu_stat
{
  int usage;
};
typedef unsigned long long ull;
struct proc_stat
{
  bool toggle;
  string cmdline;
  string name;
  float usage;

  int pid;
  string comm;
  char state;
  int ppid;
  int pgrp;
  int session;
  int tty_nr;
  int tpgid;
  ull utime;
  ull stime;
  ull cutime;
  ull cstime;
  ull priority;
  int nice;
  ull num_threads;
  ull starttime;
  unsigned int vsize;
  ull rss;
  unsigned int rsslim;
  int processor;

  int rt_priority;

  //   (41) policy  %u  (since Linux 2.5.19)
  //             Scheduling policy (see sched_setscheduler(2)).
  //             Decode using the SCHED_* constants in linux/sched.h.

  //             The format for this field was %lu before Linux
  //             2.6.22.

  //   (42) delayacct_blkio_ticks  %llu  (since Linux 2.6.18)
  //             Aggregated block I/O delays, measured in clock ticks
  //             (centiseconds).

  ull guest_time;
  ull cguest_time;
};

struct net
{
  int rate;
  ull total;
  int top;
};
struct net_stat
{
  net down;
  net up;
};

string bytePrint(ull bytes)
{
  char suffixes[] = {'B', 'B', 'K', 'M', 'G', 'T', 'P'};
  ull temp = bytes;
  ull rem = bytes;
  int x = 0;
  while (temp != 0)
  {
    rem = temp;
    temp = rem / 1024;
    x++;
  }
  return to_string(rem) + suffixes[x];
}
int main(int argc, char **argv)
{

  vector<cpu_stat> curr(thread::hardware_concurrency() + 1);
  vector<cpu_stat> prev(thread::hardware_concurrency() + 1);

  unordered_map<int, proc_stat *> procs;
  unordered_set<int> running_procs;

  vector<proc_stat *> procs_list;

  map<string, net_stat> netStats;

  map<string, size_t> memStats;

  float interval = 2;
  timer t;
  bool toggle = 0;
  while (true)
  {

    t.start();
    //////////////////////////// collect //////////////////////

    /////// cpu
    ifstream f = ifstream("/proc/stat");
    string s;
    vector<vector<string>> lines;
    while (getline(f, s) && lines.size() < thread::hardware_concurrency() + 1)
    {
      if (s.find("cpu") != -1)
        lines.push_back(split_str(s));
    }
    for (int i = 0; i < lines.size(); i++)
    {
      string usage = lines[i][1];
      curr[i].usage = stoi(usage) / interval;
    }
    f.close();
    //////// memory

    f = ifstream("/proc/meminfo");
    lines.clear();
    int numLines = 0;
    while (getline(f, s) && numLines++ < 6)
    {
      vector<string> v = split_str(s);
      memStats[v[0]] = stoull(v[1]);
    }
    memStats["Used:"] = memStats["MemTotal:"] - memStats["MemAvailable:"];

    /////// processes
    running_procs.clear();
    for (const auto &entry : fs::directory_iterator("/proc"))
    {
      if (entry.path().stem().string()[0] >= '0' && entry.path().stem().string()[0] <= '9')
      {
        // std::cout << entry.path() << endl;
        vector<string> v = getStats(entry.path());
        if (v.size() > 0)
        {
          string cmd = getCmdLine(v[0]);
          // v.push_back(cmd);
          auto ps_itr = procs.find(stoi(v[proc_enum::pid]));
          proc_stat *ps;
          if (ps_itr != procs.end())
          {
            ps = ps_itr->second;
            proc_stat prev_ps = *(ps_itr->second);

            ps->pid = stoi(v[proc_enum::pid]);
            ps->cmdline = cmd;
            ps->name = v[1];
            ps->utime = stoi(v[proc_enum::utime]);
            ps->stime = stoi(v[proc_enum::stime]);
            ps->cutime = stoi(v[proc_enum::cutime]);
            ps->toggle != ps->toggle;
            // usage = ps->stime - prev_p.at(ps->pid)->stime + ps->utime - prev_p.at(ps->pid)->utime;

            ps->usage = ((ps->utime - prev_ps.utime) + (ps->stime - prev_ps.stime)) / interval / thread::hardware_concurrency();
          }
          else
          {
            ps = new proc_stat();
            ps->pid = stoi(v[proc_enum::pid]);
            procs[ps->pid] = ps;
            ps->toggle = toggle;
            ps->cmdline = cmd;
            ps->name = v[1];
            ps->utime = stoi(v[proc_enum::utime]);
            ps->cutime = stoi(v[proc_enum::cutime]);
            ps->stime = stoi(v[proc_enum::stime]);
          }
          running_procs.emplace(ps->pid);
        }
      }
    }
    procs_list.clear();
    for (auto i = procs.begin(); i != procs.end();)
    {
      if (running_procs.find(i->second->pid) == running_procs.end())
      {
        int pid = i->second->pid;
        delete procs[pid];
        i = procs.erase(i);
      }
      else
      {
        procs_list.push_back(i->second);
        i++;
      }
    }
    toggle != toggle;

    sort(procs_list.begin(), procs_list.end(), [](const auto &lhs, const auto &rhs) {
      return (int)(lhs->usage * 1000) > (int)(rhs->usage * 1000);
    });

    ////// networks

    ifstream net("/proc/net/dev");
    vector<vector<string>> net_lines;

    string net_line;
    while (getline(net, net_line, '\n'))
    {
      net_lines.push_back(split_str(net_line));
    }
    // for (auto &i : net_lines)
    // {
    //   for (auto &j : i)
    //   {
    //     std::cout << j << ",";
    //   }
    //   std::cout << endl;
    // }
    for (int i = 2; i < net_lines.size(); i++)
    {
      {
        int prevRate = netStats[net_lines[i][0]].down.rate;
        ull prevDown = netStats[net_lines[i][0]].down.total;
        ull currDown = stoull(net_lines[i][(int)net_recieve::bytes]);
        netStats[net_lines[i][0]].down.rate = (currDown - prevDown) / interval;
        netStats[net_lines[i][0]].down.total = currDown;
        if (netStats[net_lines[i][0]].down.rate > prevRate)
          netStats[net_lines[i][0]].down.top = netStats[net_lines[i][0]].down.rate;
      }
      {
        int prevRate = netStats[net_lines[i][0]].up.rate;
        ull prevUp = netStats[net_lines[i][0]].up.total;
        ull currUp = stoull(net_lines[i][(int)net_transmit::bytes]);
        netStats[net_lines[i][0]].up.rate = (currUp - prevUp) / interval;
        netStats[net_lines[i][0]].up.total = currUp;
        if (netStats[net_lines[i][0]].up.rate > prevRate)
          netStats[net_lines[i][0]].up.top = netStats[net_lines[i][0]].up.rate;
      }
    }

    //////////////////////////// render //////////////////////
    system("clear");
    std::cout << "collection: " << t.stop() << " ms" << endl;
    std::cout << (float)(curr[0].usage - prev[0].usage) / thread::hardware_concurrency() << "\t";
    for (int i = 1; i < curr.size(); i++)
    {
      std::cout << curr[i].usage - prev[i].usage << "\t";
    }
    std::cout << endl
              << endl;
    for (auto &i : memStats)
    {
      cout << i.first << "\t" << bytePrint(i.second * 1024) << endl;
    }
    std::cout << endl;
    auto itr = procs_list.begin();
    for (int i = 0; i < 20; i++)
    {
      std::cout << (*itr)->pid << " " << (*itr)->name << " : " << (*itr)->cmdline.substr(0, 128) << " -- " << (int)((*itr)->usage * 1000) / 1000.0 << endl;
      itr++;
    }
    std::cout << endl
              << endl;
    for (auto &i : netStats)
    {
      std::cout << i.first << ": " << bytePrint(i.second.down.rate) << " : " << bytePrint(i.second.down.top) << " : " << bytePrint(i.second.down.total) << endl;
      std::cout << "\t" << bytePrint(i.second.up.rate) << " : " << bytePrint(i.second.up.top) << " : " << bytePrint(i.second.up.total) << endl;
    }

    curr.swap(prev);

    this_thread::sleep_for(interval * 1s);
  }
  std::cout << "######################" << endl;

  // for(auto &i : procs){
  //     std::cout << i[0] << " : " << i[1] << " : " << i.back() << endl;
  // }
}