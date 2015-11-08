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
extern std::string qualified_dev;

static void device_helper()
{
        if(get_plugin_dev() == -1)
        {
                exit_flag = true;
                logs << "Can't get available devices,eixt...\n";
        }
        sleep(1);
}

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

        while(!exit_flag)
        {
                while(!is_qualified_device() && !exit_flag)
                        device_helper();

                if(!exit_flag)
                {
                        privilege(true);
                        logs << qualified_dev << " granted privilege!\n";
                }

                while(is_qualified_device() && !exit_flag)
                        device_helper();

                privilege(false);

                logs << "Dropped privilege!\n";
        }

        logs.close();
        unlink(PID_PATH);
        return 0;
}
