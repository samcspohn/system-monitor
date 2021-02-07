#include <fstream>
#include <iostream>
#include <filesystem>
#include <map>
#include <list>
#include <queue>
#include <vector>
#include <string>
#include <sstream>
#include <unordered_map>
#include <unordered_set>

// #include <ranges>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <thread>
#include <numeric>
#include <algorithm>
#include <chrono>
#include <GL/glew.h>
#include <SDL2/SDL.h>

#include "imgui.h"
#include "imgui_impl_sdl.h"
#include "imgui_impl_opengl3.h"
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
typedef unsigned long long ull;
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

struct vec2
{
  float x;
  float y;
  vec2() {}
  vec2(float _x, float _y) : x(_x), y(_y) {}
  vec2 operator*(const vec2 &v)
  {
    return vec2(this->x * v.x, this->y * v.y);
  }
  vec2 operator*(const float &f)
  {
    return {this->x * f, this->y * f};
  }
  vec2 operator+(const vec2 &v)
  {
    return {this->x + v.x, this->y + v.y};
  }
};
struct graph
{
  int size;
  deque<float> data;
  graph(int s) : size(s)
  {
    for (int i = 0; i < size; i++)
      data.push_back(0.f);
  }
  void push(float f)
  {
    data.push_back(f);
    // if(data.size() > size){
    data.pop_front();
    // }
  }

  void render(vec2 pos, vec2 _size)
  {

    vector<float> arr = vector<float>(data.begin(),data.end());
    ImGui::SetCursorPos(ImVec2(pos.x * 600,pos.y * 500));
    ImGui::PlotHistogram("", arr.data(), arr.size(), 0, NULL, 0.0f, 100.0f, ImVec2(_size.x * 1000 / 2, _size.y * 720 / 2));
    // glBegin(GL_QUADS);
    // glColor3f(0.0f, 1.0f, 0.0f);
    // for (int i = 0; i < size - 1; i++)
    // {
    //   vec2 v1 = vec2((float)i / (float)(size - 1), data[i] / 100.f) * _size + pos;
    //   vec2 v2 = vec2((float)(i + 1) / (float)(size - 1), data[i + 1] / 100.f) * _size + pos;
    //   glVertex2f(v1.x, pos.y);
    //   glVertex2f(v1.x, v1.y);
    //   glVertex2f(v2.x, v2.y);
    //   glVertex2f(v2.x, pos.y);
    // }
    // glEnd();

    // glBegin(GL_LINE_LOOP);
    // glColor3f(.9f, .9f, .9f);
    // glVertex2f(pos.x, pos.y);
    // glVertex2f(pos.x, pos.y + _size.y);
    // glVertex2f(pos.x + _size.x, pos.y + _size.y);
    // glVertex2f(pos.x + _size.x, pos.y);
    // glEnd();
  }
};

int main(int argc, char **argv)
{
  graph avgCpu(20);
  vector<graph> avgCpus = vector<graph>(thread::hardware_concurrency(), graph(20));
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

  SDL_Init(SDL_INIT_VIDEO | SDL_INIT_TIMER | SDL_INIT_GAMECONTROLLER); // Initialize SDL2

  // GL 3.0 + GLSL 130
  const char *glsl_version = "#version 130";
  SDL_GL_SetAttribute(SDL_GL_CONTEXT_FLAGS, 0);
  SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_CORE);
  SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 3);
  SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 0);

  // Create window with graphics context
  SDL_GL_SetAttribute(SDL_GL_DOUBLEBUFFER, 1);
  SDL_GL_SetAttribute(SDL_GL_DEPTH_SIZE, 24);
  SDL_GL_SetAttribute(SDL_GL_STENCIL_SIZE, 8);
  SDL_WindowFlags window_flags = (SDL_WindowFlags)(SDL_WINDOW_OPENGL | SDL_WINDOW_RESIZABLE | SDL_WINDOW_ALLOW_HIGHDPI);
  SDL_Window *window = SDL_CreateWindow("Dear ImGui SDL2+OpenGL3 example", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, 1280, 720, window_flags);
  SDL_GLContext gl_context = SDL_GL_CreateContext(window);
  SDL_GL_MakeCurrent(window, gl_context);
  SDL_GL_SetSwapInterval(1); // Enable vsync

  bool err = glewInit() != GLEW_OK;
  if (err)
  {
    fprintf(stderr, "Failed to initialize OpenGL loader!\n");
    return 1;
  }

  SDL_bool done = SDL_FALSE;

  // Setup Dear ImGui context
  IMGUI_CHECKVERSION();
  ImGui::CreateContext();
  ImGuiIO &io = ImGui::GetIO();
  (void)io;
  //io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;     // Enable Keyboard Controls
  //io.ConfigFlags |= ImGuiConfigFlags_NavEnableGamepad;      // Enable Gamepad Controls

  // Setup Dear ImGui style
  ImGui::StyleColorsDark();
  //ImGui::StyleColorsClassic();

  // Setup Platform/Renderer bindings
  ImGui_ImplSDL2_InitForOpenGL(window, gl_context);
  ImGui_ImplOpenGL3_Init(glsl_version);

  int lastTime = 1000 * -interval, currentTime;

  while (!done)
  {
    currentTime = SDL_GetTicks();
    //////////////////////////// collect //////////////////////
    if (currentTime > lastTime + 1000 * interval)
    {
      lastTime = currentTime;

      t.start();
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
      toggle = !toggle;

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
      for (int i = 2; i < net_lines.size(); i++)
      {
        {
          int topRate = netStats[net_lines[i][0]].down.top;
          ull prevDown = netStats[net_lines[i][0]].down.total;
          ull currDown = stoull(net_lines[i][(int)net_recieve::bytes]);
          netStats[net_lines[i][0]].down.rate = (currDown - prevDown) / interval;
          netStats[net_lines[i][0]].down.total = currDown;
          if (netStats[net_lines[i][0]].down.rate > topRate)
            netStats[net_lines[i][0]].down.top = netStats[net_lines[i][0]].down.rate;
        }
        {
          int topRate = netStats[net_lines[i][0]].up.top;
          ull prevUp = netStats[net_lines[i][0]].up.total;
          ull currUp = stoull(net_lines[i][(int)net_transmit::bytes]);
          netStats[net_lines[i][0]].up.rate = (currUp - prevUp) / interval;
          netStats[net_lines[i][0]].up.total = currUp;
          if (netStats[net_lines[i][0]].up.rate > topRate)
            netStats[net_lines[i][0]].up.top = netStats[net_lines[i][0]].up.rate;
        }
      }

      //////////////////////////// render //////////////////////
      system("clear");
      std::cout << "collection: " << t.stop() << " ms" << endl;
      std::cout << (float)(curr[0].usage - prev[0].usage) / thread::hardware_concurrency() << "\t";
      vector<int> usage;

      for (int i = 1; i < curr.size(); i++)
      {
        usage.emplace_back(curr[i].usage - prev[i].usage);
        avgCpus[i - 1].push(usage.back());
      }
      float avgUsage = (float)accumulate(usage.begin(), usage.end(), 0) / (float)usage.size();

      avgCpu.push(avgUsage);
      cout << avgUsage << "\t";
      for (int i : usage)
      {
        std::cout << i << "\t";
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
    }

    SDL_Event event;
    while (SDL_PollEvent(&event))
    {
      ImGui_ImplSDL2_ProcessEvent(&event);
      switch (event.type)
      {
      case SDL_QUIT:
        /* Quit */
        done = SDL_TRUE;
        break;
      }
    }

    // this_thread::sleep_for(interval * 1s);
    glClearColor(0.3, .3, .3, 1.0);
    glClear(GL_COLOR_BUFFER_BIT);


    IM_ASSERT(ImGui::GetCurrentContext() != NULL && "Missing dear imgui context. Refer to examples app!"); // Exceptionally add an extra assert here for people confused with initial dear imgui setup

    // int count = 0;
    // Start the Dear ImGui frame
    // cout << "here" << count++ << endl;
    ImGui_ImplOpenGL3_NewFrame();
    // cout << "here" << count++ << endl;
    ImGui_ImplSDL2_NewFrame(window);
    // cout << "here" << count++ << endl;
    ImGui::NewFrame();
    // cout << "here" << count++ << endl;

    ImGui::Begin("cpus");
    // avgCpu.render(vec2(-.5,0),vec2(0.1,0.1));
    int avgCpusItr = 0;
    vec2 cpusPos(0.04f, 0.04f);
    vec2 cpusSize(1.2f, 0.8);
    int count = 0;
    for (int x = 0; x < 8; x++)
    {
      for (int y = 0; y < 8; y++)
      {
        avgCpus[avgCpusItr].render(vec2(cpusPos.x + x * cpusSize.x / 8, cpusPos.y + y * cpusSize.y / 8), vec2(cpusSize.x / 8, cpusSize.y / 8));
        avgCpusItr++;
      }
    }
    ImGui::End();
        // cout << "here: " << count++ << endl;

    {
      static float f = 0.0f;
      static int counter = 0;
      // ImGui::SetNextWindowPos(ImVec2(30, 30), ImGuiCond_Always);
      // ImGui::SetNextWindowSize(ImVec2(200, 100), ImGuiCond_Always);

      ImGui::Begin("Hello, world!"); // Create a window called "Hello, world!" and append into it.

      // ImGui::Columns(4, NULL, false);
      auto itr = procs_list.begin();
      for (int i = 0; i < 20; i++)
      {
        // std::cout << (*itr)->pid << " " << (*itr)->name << " : " << (*itr)->cmdline.substr(0, 128) << " -- " << (int)((*itr)->usage * 1000) / 1000.0 << endl;

        ImGui::Text(to_string((*itr)->pid).c_str());
        // ImGui::NextColumn();
        ImGui::SameLine(60);
        ImGui::Text((*itr)->name.c_str());
        // ImGui::NextColumn();
        ImGui::SameLine(150);
        ImGui::Text((*itr)->cmdline.substr(0, 100).c_str());
        // ImGui::NextColumn();
        ImGui::SameLine(1000);
        ImGui::Text("%.2f", (int)((*itr)->usage * 1000) / 1000.0);
        // ImGui::NextColumn();
        // ImGui::SameLine(20);
        // ImGui::NewLine();
        itr++;
      }
      // ImGui::Columns(1);

      // ImGui::Text("This is some useful text."); // Display some text (you can use a format strings too)

      // ImGui::SliderFloat("float", &f, 0.0f, 1.0f); // Edit 1 float using a slider from 0.0f to 1.0f

      // if (ImGui::Button("Button")) // Buttons return true when clicked (most widgets return true when edited/activated)
      //   counter++;
      // ImGui::SameLine();
      // ImGui::Text("counter = %d", counter);

      // ImGui::Text("Application average %.3f ms/frame (%.1f FPS)", 1000.0f / ImGui::GetIO().Framerate, ImGui::GetIO().Framerate);
      ImGui::End();
    }

    // Rendering
    ImGui::Render();
    glViewport(0, 0, (int)io.DisplaySize.x, (int)io.DisplaySize.y);
    ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());

    SDL_GL_SwapWindow(window);
    // SDL_Delay(interval * 1000);
  }

  // Cleanup gui
  ImGui_ImplOpenGL3_Shutdown();
  ImGui_ImplSDL2_Shutdown();
  ImGui::DestroyContext();

  /* Delete our opengl context, destroy our window, and shutdown SDL */
  SDL_GL_DeleteContext(gl_context);
  SDL_DestroyWindow(window);
  SDL_Quit();

  std::cout << "######################" << endl;

  // for(auto &i : procs){
  //     std::cout << i[0] << " : " << i[1] << " : " << i.back() << endl;
  // }
}