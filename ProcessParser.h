#ifndef PROCESSPARSER_H
#define PROCESSPARSER_H

#include "constants.h"
#include <algorithm>
#include <cerrno>
#include <chrono>
#include <cstring>
#include <dirent.h>
#include <fstream>
#include <iostream>
#include <iterator>
#include <math.h>
#include <sstream>
#include <stdexcept>
#include <stdio.h>
#include <stdlib.h>
#include <string>
#include <thread>
#include <time.h>
#include <unistd.h>
#include <vector>

#include "util.h"

using namespace std;

class ProcessParser {
private:
  std::ifstream stream;

public:
  static string getCmd(string pid);
  static vector<string> getPidList();
  static std::string getVmSize(string pid);
  static std::string getCpuPercent(string pid);
  static long int getSysUpTime();
  static std::string getProcUpTime(string pid);
  static string getProcUser(string pid);
  static vector<string> getSysCpuPercent(string coreNumber = "");
  static float getSysRamPercent();
  static string getSysKernelVersion();
  static int getNumberOfCores();
  static int getTotalThreads();
  static int getTotalNumberOfProcesses();
  static int getNumberOfRunningProcesses();
  static string getOSName();
  static float getSysActiveCpuTime(vector<string> values);
  static float getSysIdleCpuTime(vector<string> values);
  static std::string printCpuStats(std::vector<std::string> values1,
                                   std::vector<std::string> values2);
  static bool isPidExisting(string pid);
};

// TODO: Define all of the above functions below:

// Reading /proc/[PID]/status for memory status of specific process
string ProcessParser::getVmSize(string pid) {
  string line;
  // Declaring search attribute for file
  string name = "VmData";
  string value;
  float result;
  // Opening stream for specific file
  ifstream stream =
      Util::getStream((Path::basePath() + pid + Path::statusPath()));
  while (std::getline(stream, line)) {
    // Searching line by line
    if (line.compare(0, name.size(), name) == 0) {
      // slicing string line on ws for values using sstream
      istringstream buf(line);
      istream_iterator<string> beg(buf), end;
      // the string buf is sliced into pieces of strings (with space to seperate
      // them) by initializing the vector of sting
      vector<string> values(beg, end);
      // conversion kB -> GB
      // the 2nd piece is the number of vm
      result = (stof(values[1]) / float(1024 * 1024));
      break;
    }
  }
  return to_string(result);
}

// Reading /proc/[PID]/status for CPU status of specific process
string ProcessParser::getCpuPercent(string pid) {
  string line;
  string value;
  float result;
  ifstream stream =
      Util::getStream((Path::basePath() + pid + "/" + Path::statPath()));
  getline(stream, line);
  istringstream buf(line);
  istream_iterator<string> beg(buf), end;
  vector<string> values(beg, end); // done!
  // acquiring relevant times for calculation of active occupation of CPU for
  // selected process
  float utime = stof(ProcessParser::getProcUpTime(pid));
  float stime = stof(values[14]);
  float cutime = stof(values[15]);
  float cstime = stof(values[16]);
  float starttime = stof(values[21]);
  float uptime = ProcessParser::getSysUpTime();
  float freq = sysconf(_SC_CLK_TCK);
  float total_time = utime + stime + cutime + cstime;
  float seconds = uptime - (starttime / freq);
  result = 100.0 * ((total_time / freq) / seconds);
  return to_string(result);
}

// Reading /proc/[PID]/status for process up time of specific process
string ProcessParser::getProcUpTime(string pid) {
  string line;
  ifstream stream =
      Util::getStream((Path::basePath() + pid + "/" + Path::statPath()));
  getline(stream, line);
  istringstream buf(line);
  istream_iterator<string> beg(buf), end;
  vector<string> values(beg, end); // done!
  // Using sysconf to get clock ticks of the host machine
  float result = float(stof(values[13]) / sysconf(_SC_CLK_TCK));
  return to_string(result);
}

long int ProcessParser::getSysUpTime() {
  string line;
  ifstream stream = Util::getStream((Path::basePath() + Path::upTimePath()));
  getline(stream, line);
  istringstream buf(line);
  istream_iterator<string> beg(buf), end;
  vector<string> values(beg, end);
  return stoi(values[0]);
}

string ProcessParser::getProcUser(string pid) {
  string line;
  string name = "Uid:";
  string result = "";
  ifstream stream =
      Util::getStream((Path::basePath() + pid + Path::statusPath()));
  // Getting UID for user
  while (std::getline(stream, line)) {
    if (line.compare(0, name.size(), name) == 0) {
      istringstream buf(line);
      istream_iterator<string> beg(buf), end;
      vector<string> values(beg, end);
      result = values[1];
      break;
    }
  }
  stream = Util::getStream("/etc/passwd");
  name = ("x:" + result);
  // Searching for name of the user with selected UID
  while (std::getline(stream, line)) {
    if (line.find(name) != std::string::npos) {
      result = line.substr(0, line.find(":"));
      return result;
    }
  }
  return "";
}

vector<string> ProcessParser::getPidList() {
  DIR *dir;
  // Basically, we are scanning /proc dir for all directories with numbers as
  // their names If we get valid check we store dir names in vector as list of
  // machine pids
  vector<string> container;
  if (!(dir = opendir("/proc")))
    throw std::runtime_error(std::strerror(errno));

  while (dirent *dirp = readdir(dir)) {
    // is this a directory?
    if (dirp->d_type != DT_DIR)
      continue;
    // Is every character of the name a digit?
    if (all_of(dirp->d_name, dirp->d_name + std::strlen(dirp->d_name),
               [](char c) { return std::isdigit(c); })) {
      container.push_back(dirp->d_name);
    }
  }
  // Validating process of directory closing
  if (closedir(dir))
    throw std::runtime_error(std::strerror(errno));
  return container;
}

string ProcessParser::getCmd(string pid) {
  string line;
  ifstream stream = Util::getStream((Path::basePath() + pid + Path::cmdPath()));
  std::getline(stream, line);
  return line;
}

int ProcessParser::getNumberOfCores() {
  // Get the number of host cpu cores
  string line;
  string name = "cpu cores";
  ifstream stream = Util::getStream((Path::basePath() + "cpuinfo"));
  while (std::getline(stream, line)) {
    if (line.compare(0, name.size(), name) == 0) {
      istringstream buf(line);
      istream_iterator<string> beg(buf), end;
      vector<string> values(beg, end);
      return stoi(values[3]);
    }
  }
  return 0;
}

vector<string> ProcessParser::getSysCpuPercent(string coreNumber) {
  // It is possible to use this method for selection of data for overall cpu or
  // every core. when nothing is passed "cpu" line is read when, for example "0"
  // is passed  -> "cpu0" -> data for first core is read
  string line;
  string name = "cpu" + coreNumber;
  ifstream stream = Util::getStream((Path::basePath() + Path::statPath()));
  while (std::getline(stream, line)) {
    if (line.compare(0, name.size(), name) == 0) {
      istringstream buf(line);
      istream_iterator<string> beg(buf), end;
      vector<string> values(beg, end);
      // set of cpu data active and idle times;
      return values;
    }
  }
  return (vector<string>());
}
float ProcessParser::getSysActiveCpuTime(vector<string> values) {
  return (stof(values[S_USER]) + stof(values[S_NICE]) + stof(values[S_SYSTEM]) +
          stof(values[S_IRQ]) + stof(values[S_SOFTIRQ]) +
          stof(values[S_STEAL]) + stof(values[S_GUEST]) +
          stof(values[S_GUEST_NICE]));
}

float ProcessParser::getSysIdleCpuTime(vector<string> values) {
  return (stof(values[S_IDLE]) + stof(values[S_IOWAIT]));
}

string ProcessParser::printCpuStats(vector<string> values1,
                                    vector<string> values2) {
  /*
  Because CPU stats can be calculated only if you take measures in two different
  time, this function has two parameters: two vectors of relevant values. We use
  a formula to calculate overall activity of processor.
  */
  float activeTime =
      getSysActiveCpuTime(values2) - getSysActiveCpuTime(values1);
  float idleTime = getSysIdleCpuTime(values2) - getSysIdleCpuTime(values1);
  float totalTime = activeTime + idleTime;
  float result = 100.0 * (activeTime / totalTime);
  return to_string(result);
}

float ProcessParser::getSysRamPercent() {
  string line;
  string name1 = "MemAvailable:";
  string name2 = "MemFree:";
  string name3 = "Buffers:";

  string value;
  int result;
  ifstream stream = Util::getStream((Path::basePath() + Path::memInfoPath()));
  float total_mem = 0;
  float free_mem = 0;
  float buffers = 0;
  while (std::getline(stream, line)) {
    if (total_mem != 0 && free_mem != 0)
      break;
    if (line.compare(0, name1.size(), name1) == 0) {
      istringstream buf(line);
      istream_iterator<string> beg(buf), end;
      vector<string> values(beg, end);
      total_mem = stof(values[1]);
    }
    if (line.compare(0, name2.size(), name2) == 0) {
      istringstream buf(line);
      istream_iterator<string> beg(buf), end;
      vector<string> values(beg, end);
      free_mem = stof(values[1]);
    }
    if (line.compare(0, name3.size(), name3) == 0) {
      istringstream buf(line);
      istream_iterator<string> beg(buf), end;
      vector<string> values(beg, end);
      buffers = stof(values[1]);
    }
  }
  // calculating usage:
  return float(100.0 * (1 - (free_mem / (total_mem - buffers))));
}

string ProcessParser::getSysKernelVersion() {
  string line;
  string name = "Linux version ";
  ifstream stream = Util::getStream((Path::basePath() + Path::versionPath()));
  while (std::getline(stream, line)) {
    if (line.compare(0, name.size(), name) == 0) {
      istringstream buf(line);
      istream_iterator<string> beg(buf), end;
      vector<string> values(beg, end);
      return values[2];
    }
  }
  return "";
}

string ProcessParser::getOSName() {
  string line;
  string name = "PRETTY_NAME=";

  ifstream stream = Util::getStream(("/etc/os-release"));

  while (std::getline(stream, line)) {
    if (line.compare(0, name.size(), name) == 0) {
      std::size_t found = line.find("=");
      found++;
      string result = line.substr(found);
      result.erase(std::remove(result.begin(), result.end(), '"'),
                   result.end());
      return result;
    }
  }
  return "";
}

int ProcessParser::getTotalThreads() {
  string line;
  int result = 0;
  string name = "Threads:";
  vector<string> _list = ProcessParser::getPidList();
  for (int i = 0; i < _list.size(); i++) {
    string pid = _list[i];
    // getting every process and reading their number of their threads
    ifstream stream =
        Util::getStream((Path::basePath() + pid + Path::statusPath()));
    while (std::getline(stream, line)) {
      if (line.compare(0, name.size(), name) == 0) {
        istringstream buf(line);
        istream_iterator<string> beg(buf), end;
        vector<string> values(beg, end);
        result += stoi(values[1]);
        break;
      }
    }
    return result;
  }
}

int ProcessParser::getTotalNumberOfProcesses() {
  string line;
  int result = 0;
  string name = "processes";
  ifstream stream = Util::getStream((Path::basePath() + Path::statPath()));
  while (std::getline(stream, line)) {
    if (line.compare(0, name.size(), name) == 0) {
      istringstream buf(line);
      istream_iterator<string> beg(buf), end;
      vector<string> values(beg, end);
      result += stoi(values[1]);
      break;
    }
  }
  return result;
}

int ProcessParser::getNumberOfRunningProcesses() {
  string line;
  int result = 0;
  string name = "procs_running";
  ifstream stream = Util::getStream((Path::basePath() + Path::statPath()));
  while (std::getline(stream, line)) {
    if (line.compare(0, name.size(), name) == 0) {
      istringstream buf(line);
      istream_iterator<string> beg(buf), end;
      vector<string> values(beg, end);
      result += stoi(values[1]);
      break;
    }
  }
  return result;
}

bool ProcessParser::isPidExisting(string pid) {
  vector<string> _list = ProcessParser::getPidList();
  for (const auto &str : _list) {
    if (str.compare(0, pid.size(), pid) == 0) {
      return true;
    }
  }
  return false;
}

#endif // PROCESSPARSER_H