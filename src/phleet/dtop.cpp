//********************************************************
// Syzygy is licensed under the BSD license v2
// see the file SZG_CREDITS for details
//********************************************************

// unix "top"-style color display a la dps

#include "arPrecompiled.h"
#define SZG_DO_NOT_EXPORT

#include "arSZGClient.h"

#ifdef AR_USE_WIN_32
int main(int argc, char** argv) {
  cerr << "sorry, dtop unavailable under win32.\n";
}
#else
#ifdef AR_USE_SGI
int main(int argc, char** argv) {
  cerr << "sorry, dtop unavailable under Irix.\n";
}
#else

extern "C"{
#include <curses.h>
}
#include <stdio.h> // for sscanf

WINDOW* ww = NULL; // if this fails, type: sudo apt-get install libncurses-dev
int fColor = 0;

bool fDone = false;
void messageTask(void* pClient)
{
  arSZGClient* cli = (arSZGClient*)pClient;
  string messageType, messageBody;
  while (cli->running()) {
    // if receiveMessage fails, szgserver may have died, but it probably doesn't matter.
    if (!cli->receiveMessage(&messageType, &messageBody) ||
        messageType=="quit") {
      fDone = true;
      return;
    }
  }
}

// Handle window-resize (from putty or xterm).
#include <signal.h>
void resizeHandler(int /*sig*/)
{
  int nh, nw;
  getmaxyx(stdscr, nh, nw);
  endwin();
  resizeterm(nh, nw);
  refresh();
}

void update(const string& lines)
{
  int y = 0;

  // todo: use STL for all this
  const int chost = 60;
  const int cchMax = 30;
  const int ctask = 15;
  char hosts[chost][cchMax];                // list of hosts
  char tasks[chost][ctask][cchMax];        // tasks of each host
  char taskOrder[ctask][cchMax];        // color them consistently
  char cchTask = 0;
  clear();

  memset(hosts, 0, sizeof(hosts));
  memset(tasks, 0, sizeof(tasks));
  memset(taskOrder, 0, sizeof(taskOrder));

  for (unsigned iline=0; iline < lines.length(); ++iline) {

    // Stuff buf[] with the next line.
    int ich = 0;
    char buf[256];
    while (lines[iline] != ':' && iline < lines.length())
      buf[ich++] = lines[iline++];
    buf[ich] = '\0';

    // Parse the components of buf[].
    char host[256];
    char task[256];
    char id[256];
    sscanf(buf, "%[^/]/%[^/]/%s", host, task, id);

    // strip domain name from host -- that's more clutter than useful.
    char* pch = strchr(host, '.');
    if (pch)
      *pch = '\0';

    // Store host[] and task[] in a data structure.

    if (!strcmp(host, "NULL"))
      continue;

    int ihost = 0;
    for (ihost=0; ihost<chost; ihost++)
      {
      if (!*hosts[ihost])
        {
        // new host
        strcpy(hosts[ihost], host);
        break;
        }
      if (!strcmp(host, hosts[ihost]))
        // previously inserted
        break;
      }
    for (ihost=0; ihost<chost; ++ihost)
      {
      if (strcmp(host, hosts[ihost]))
        continue;
      // Found the host.  Append task to that host's list.
      for (int itask=0; itask<ctask; ++itask)
        {
        if (!*tasks[ihost][itask])
          {
          strcpy(tasks[ihost][itask], task);
          goto LBreak;
          }
        }
      }
LBreak:
    for (int itask=0; itask<ctask; ++itask)
      {
      if (!*taskOrder[itask])
        {
        // new task
        strcpy(taskOrder[itask], task);
        int cch = strlen(task);
        if (cch > cchTask)
          cchTask = cch;
        break;
        }
      if (!strcmp(task, taskOrder[itask]))
        // previously inserted
        break;
      }
  }

  // Traverse the data structure and print it out.
  for (int ihost=0; ihost<chost; ihost++) {
    if (!*hosts[ihost])
      // end of list
      break;

    if (fColor) {
      color_set(7, NULL);
    }
    else{
      standout();
    }
    mvprintw(++y, 1, "%s", hosts[ihost]);
    if (fColor) {
      color_set(0, NULL);
    }
    else{
      standend();
    }
    for (int itask=0; itask<ctask; ++itask)
      {
      const char* task = tasks[ihost][itask];
      if (!*task)
        break;
      if (fColor)
        {
        // find out which color it is
        int taskColor = 1;
        for (int i=0; i<ctask; i++)
          {
          if (!strcmp(task, taskOrder[i]))
            {
            taskColor = i;
            // colors 0..7 are bk r g y bl magenta cyan white
            // bk is background, white is hostname,
            // blue is invisible on black background.
            // So use r g y mag cyan, 1 2 3 5 6.
            const int _[5]= {1, 2, 3, 5, 6};
            taskColor = _[taskColor%5];
            break;
            }
          }
        color_set(taskColor, NULL);
        }
      const int x = 12 + itask * (cchTask+1);
      mvprintw(y, x, "%s ", task);
      }
    }
  move(y+1, 1);
  refresh();
}

int main(int argc, char** argv) {
  // ncurses incorrectly assumes a black background.
  // These unfortunately doesn't correct for that:
  //   setenv("NCURSES_ASSUMED_COLORS", "-1,-1", 0);
  //   assume_default_colors(-1, -1);

  signal(SIGWINCH, &resizeHandler);
  arSZGClient szgClient;
  const bool fInit = szgClient.init(argc, argv);
  if (!szgClient)
    return szgClient.failStandalone(fInit);

  if (argc > 3) {
LUsage:
    // Compatible with unix "top".  d=set delay, q=no delay.
    // For stressTesting szgserver, t=nodisplay+nodelay (very fast).
    cerr << "usage: " << argv[0] << " [d milliseconds] | q | t\n";
    return 1;
  }

  int msec = -1;
  bool fVisible = true;
  switch (argc) {
  case 2:
    if (*argv[1] == 'q') {
      msec = 0;
    }
    else if (*argv[1] == 't') {
      fVisible = false;
      msec = 0;
    }
    else
      goto LUsage;
    break;
  case 3:
    if (*argv[1] == 'd') {
      msec = atoi(argv[2]);
      // atoi() has poor error handling.
      if (msec <= 0 && strcmp(argv[1], "0"))
        goto LUsage;
    }
    else
      goto LUsage;
    break;
  }

  if (msec < 0) {
    msec = 500;
  }

  if (fVisible) {
    // initialize the screen
    ww = initscr();
    start_color();
    fColor = has_colors();
    cbreak();
    noecho();
    nonl();
    intrflush(stdscr, FALSE);
    keypad(stdscr, TRUE);
    timeout(msec); // getch() will wait this long.
    for (int i=1; i<=7; i++)
      init_pair(i, i, COLOR_BLACK);
    bkgdset(COLOR_PAIR(7));
  }

  arThread dummy(messageTask, &szgClient);
  while (!fDone) {
    if (fVisible) {
      update(szgClient.getProcessList());
      if (getch() == 'q')
        break;
    }
  }

  endwin();
  szgClient.messageTaskStop();
  return 0;
}

#endif
#endif
