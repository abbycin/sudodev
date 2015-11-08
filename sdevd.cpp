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
#include <map>

using std::cerr;
using std::endl;

extern const char *PID_PATH;
extern const char *LOG_PATH;
extern std::ofstream logs;
extern bool exit_flag;
extern bool plugin_devs_ok;
extern std::map<int, std::string> plugin_dev;
extern std::string qualified_dev;

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

        int countdown = 0;

        while(!exit_flag)
        {
                countdown = 2;
                if(get_local_dev() == -1 || get_all_dev() == -1)
                {
                        logs << "Can't get devices list, exit..." << endl;
                        logs.close();
                        exit(1);
                }

                get_plugin_dev();

                plugin_devs_ok = plugin_dev.empty() ? false : true;

                while(!plugin_devs_ok && countdown)
                {
                        countdown -= 1;
                        sleep(1);
                }

                if(!countdown)
                        continue;

                while(!is_qualified_device() && countdown)
                {
                        countdown -= 1;
                        sleep(1);
                }
                
                if(!countdown)
                        continue;

                privilege(true);

                logs << qualified_dev << " granted privilege!\n";

                while(is_qualified_device() && !exit_flag)
                        sleep(1);

                privilege(false);

                logs << "Dropped privilege!\n";
        }

        logs.close();
        unlink(PID_PATH);
        return 0;
}
