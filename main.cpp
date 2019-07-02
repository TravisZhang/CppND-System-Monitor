#include "ProcessContainer.h"
#include "SysInfo.h"
#include "util.h"
#include <chrono>
#include <iomanip>
#include <iostream>
#include <ncurses.h>
#include <sstream>
#include <string>
#include <thread>
#include <time.h>
#include <vector>

using namespace std;

char *getCString(std::string str) {
  char *cstr = new char[str.length() + 1];
  std::strcpy(cstr, str.c_str());
  return cstr;
}

/*
WriteSysInfoToConsole is a function with two input parameters. The first
parameter is of SysInfo type. The second parameter is a dedicated ncurses object
pointer, WINDOW*. This window parameter specifies part of the terminal reserved
for the system data.
 */
void writeSysInfoToConsole(SysInfo sys, WINDOW *sys_win) {
  sys.setAttributes();

  mvwprintw(sys_win, 2, 2, getCString(("OS: " + sys.getOSName())));
  mvwprintw(sys_win, 3, 2,
            getCString(("Kernel version: " + sys.getKernelVersion())));
  mvwprintw(sys_win, 4, 2, getCString("CPU: "));
  wattron(sys_win, COLOR_PAIR(1));
  wprintw(sys_win, getCString(Util::getProgressBar(sys.getCpuPercent())));
  wattroff(sys_win, COLOR_PAIR(1));
  mvwprintw(sys_win, 5, 2, getCString(("Other cores:")));
  wattron(sys_win, COLOR_PAIR(1));
  std::vector<std::string> val = sys.getCoresStats();
  for (int i = 0; i < val.size(); i++) {
    mvwprintw(sys_win, (6 + i), 2, getCString(val[i]));
  }
  wattroff(sys_win, COLOR_PAIR(1));
  mvwprintw(sys_win, 10, 2, getCString(("Memory: ")));
  wattron(sys_win, COLOR_PAIR(1));
  wprintw(sys_win, getCString(Util::getProgressBar(sys.getMemPercent())));
  wattroff(sys_win, COLOR_PAIR(1));
  mvwprintw(sys_win, 11, 2,
            getCString(("Total Processes:" + sys.getTotalProc())));
  mvwprintw(sys_win, 12, 2,
            getCString(("Running Processes:" + sys.getRunningProc())));
  mvwprintw(sys_win, 13, 2,
            getCString(("Up Time: " + Util::convertToTime(sys.getUpTime()))));
  wrefresh(sys_win);
}

/*
This function prints the first 10 processes from the host machine, which are
stored in a string vector. They are distributed across 10 rows in a dedicated
window. Every invocation of the function refreshes the process list and prints
refreshed datas.

The function prototype has two input parameters: the ProcessContainer object,
which is a container wrapper around the Process object, and the pointer WINDOW
type from ncurses.
*/
void getProcessListToConsole(std::vector<string> processes, WINDOW *win) {

  wattron(win, COLOR_PAIR(2));
  mvwprintw(win, 1, 2, "PID:");
  mvwprintw(win, 1, 9, "User:");
  mvwprintw(win, 1, 16, "CPU[%%]:");
  mvwprintw(win, 1, 26, "RAM[MB]:");
  mvwprintw(win, 1, 35, "Uptime:");
  mvwprintw(win, 1, 44, "CMD:");
  wattroff(win, COLOR_PAIR(2));
  for (int i = 0; i < processes.size(); i++) {
    mvwprintw(win, 2 + i, 2, getCString(processes[i]));
  }
}
/*
The binding function for consistent display of data via ncurses is the
printMain() function. This function accepts two parameters: a SysInfo object and
a ProcessContainer. The function initializes the ncurses screen, separate
windows, and a coloring scheme.

The infinite loop is retrieves data and updates the terminal screen. With this
technique we achieve a live display of the machine state.

Every window is separately refreshed via wrefresh, and boundaries are set for
each window via the box method. These are ncurses methods.
*/
void printMain(SysInfo sys, ProcessContainer procs) {
  initscr();     /* Start curses mode 		  */
  noecho();      // not printing input values
  cbreak();      // Terminating on classic ctrl + c
  start_color(); // Enabling color change of text
  int yMax, xMax;
  getmaxyx(stdscr, yMax, xMax); // getting size of window measured in lines and
                                // columns(column one char length)
  WINDOW *sys_win = newwin(17, xMax - 1, 0, 0);
  WINDOW *proc_win = newwin(15, xMax - 1, 18, 0);

  init_pair(1, COLOR_BLUE, COLOR_BLACK);
  init_pair(2, COLOR_GREEN, COLOR_BLACK);
  int counter = 0;
  while (1) {
    box(sys_win, 0, 0);
    box(proc_win, 0, 0);
    procs.refreshList();
    std::vector<std::vector<std::string>> processes = procs.getList();
    writeSysInfoToConsole(sys, sys_win);
    getProcessListToConsole(processes[counter], proc_win);
    wrefresh(sys_win);
    wrefresh(proc_win);
    refresh();
    sleep(1);
    if (counter == (processes.size() - 1)) {
      counter = 0;
    } else {
      counter++;
    }
  }
  endwin();
}
/*
main() initializes the ProcessContainer and SysInfo objects. After that we
directly invoke printMain().
*/
int main(int argc, char *argv[]) {
  // Object which contains list of current processes, Container for Process
  // Class
  ProcessContainer procs;
  // Object which containts relevant methods and attributes regarding system
  // details
  SysInfo sys;
  // std::string s = writeToConsole(sys);
  printMain(sys, procs);
  return 0;
}
