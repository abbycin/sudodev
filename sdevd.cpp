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
#include <time.h>
#include <map>

using std::cerr;
using std::endl;

static std::ofstream logs;
static bool exit_flag;

void hup_handler(int sig)
{
        logs << "Cought signal " << sig << " restart..." << endl;
}

void term_handler(int sig)
{
        logs << "Cought signal " << sig << " exit..." << endl;
        exit_flag = true;
}

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
        if(getuid())
        {
                cerr << "Please run as root! exit..." << endl;
                exit(1);
        }

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
        logs << "++++++++  program started! ++++++++" << endl;

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

        std::string msg;

        if(record_pid(msg) == -1)
        {
                logs << msg << endl;
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
        
        std::string qualified_dev;
        
        while(!exit_flag)
        {
                if(log_truncate() == -1)
                        break;
                while(!is_qualified_device(qualified_dev) && !exit_flag)
                        device_helper();

                if(!exit_flag)
                {
                        privilege(true);
                        logs << qualified_dev << " granted privilege!\n";
                }

                while(is_qualified_device(qualified_dev) && !exit_flag)
                        device_helper();

                privilege(false);

                logs << "Dropped privilege!\n";
        }

        time_t tm = time(NULL);
        logs << "Program exited: " << ctime(&tm) << endl;
        logs.close();

        unlink(PID_PATH);

        return 0;
}
