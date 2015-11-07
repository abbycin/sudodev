/*********************************************************
          File Name:sdevd.cpp
          Author: Abby Cin
          Mail: abbytsing@gmail.com
          Created Time: Sat 07 Nov 2015 04:06:27 PM CST
**********************************************************/

#include "sudodev.h"
#include <iostream>
#include <fstream>
#include <unistd.h>
#include <signal.h>
#include <string.h>
#include <errno.h>

using std::cerr;
using std::endl;

extern const char *PID_PATH;
extern const char *LOG_PATH;
extern std::ofstream logs;
extern bool exit_flag;
extern bool sig_hup;

int main()
{
        
        if(to_daemon() == -1)
        {
                cerr << "Can't turn into daemon, exit..." << endl;
                exit(1);
        }

        logs.open(LOG_PATH, logs.out | logs.app);
        if(!logs.good())
        {
                cerr << "Can't open log, exit..." << endl;
                exit(1);
        }

        logs << std::unitbuf;

        if(access(PID_PATH, F_OK) == 0)
        {
                logs << "A daemon is running or that daemon exit "
                        << "unexpectly, please manaully remove this"
                        << " lock file: " << PID_PATH << "\nexit..."
                        << endl;
                logs.close();
                exit(1);
        }

        if(add_group() == -1)
        {
                logs << "Can't add group, exit..." << endl;
                logs.close();
                exit(1);
        }

        if(drop_in() == -1)
        {
                logs << "Can't enbale `drop in` support, exit..."
                        << endl;
                logs.close();
                exit(1);
        }

        if(record_pid() == -1)
        {
                logs.close();
                exit(1);
        }

        if(install_handler(SIGHUP, hup_handler) == -1 || 
                install_handler(SIGTERM, term_handler) == -1)
        {
                logs << "Can't install signal handlers: " << strerror(errno)
                        << " exit..." << endl;
                logs.close();
                exit(1);
        }

        pthread_t tid;
        pthread_t worker;

        if(pthread_create(&worker, NULL, privilege_guard, NULL))
        {
                logs << "Can't create worker thread, exit..." << endl;
                logs.close();
                exit(1);
        }

        while(!exit_flag)
        {
                if(pthread_create(&tid, NULL, trigger, NULL))
                {
                        logs << "Can't create a worker thread, exit..." << endl;
                        logs.close();
                        exit(1);
                }
                pthread_join(tid, 0);
                sig_hup = false;
        }
        
        logs.close();
        unlink(PID_PATH);
        return 0;
}
