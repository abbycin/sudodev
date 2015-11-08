/*********************************************************
          File Name:sdev_ctl.cpp
          Author: Abby Cin
          Mail: abbytsing@gmail.com
          Created Time: Sat 07 Nov 2015 09:07:29 AM CST
**********************************************************/

#include "sudodev.h"
#include <iostream>
#include <fstream>
#include <unistd.h>
#include <string.h>
#include <map>
#include <string>

extern bool plugin_devs_ok;
extern bool exit_flag;
extern std::map<int, std::string> plugin_dev;
extern std::map<int, std::string> plugin_dev_uuid;

using std::cerr;
using std::cout;
using std::endl;

static int del_dev()
{
        if(access(SDEV_CONF_PATH, F_OK) == 0)
        {
                if(truncate(SDEV_CONF_PATH, 0) == -1)
                        return -1;
        }

        return 0;
}

static void usage(const char *arg)
{
        cerr << "Usage:\n";
        cerr << arg << " [add|del]" << endl;
        exit(1);
}

static std::string make_choice()
{
        std::string input;
        int choice;
        bool is_choice_ok = false;
        cout << "The following choices are available\n";

        while(!is_choice_ok)
        {
                cout << "[q]. quit" << endl;
                list_dev();
                cout << "Please enter a choice: ";
                std::cin >> input;

                if(input == "q")
                        exit(0);

                for(auto &x: input)
                {
                        if('0' > x || x > '9')
                        {
                                cerr << "Bad input, exit...\n";
                                exit(1);
                        }
                }
                choice = std::stod(input);


                for(auto &x: plugin_dev)
                {
                        if(choice == x.first)
                        {
                                is_choice_ok = true;
                                break;
                        }
                }
                if(!is_choice_ok)
                        cerr << "\nBad choice, try again!" << endl;
        }

        return plugin_dev_uuid[choice];
}

int main(int argc, char *argv[])
{
        if(argc != 2)
                usage(argv[0]);

        if(strcmp(argv[1], "del") == 0)
        {
                if(del_dev() == -1)
                {
                        cout << "Remove qualified device failed,"
                                << " exit..." << endl;
                        exit(1);
                }
                else
                {
                        cout << "Qualified device has been removed!\n";
                        exit(0);
                }
        }

        if(strcmp(argv[1], "add") != 0)
                usage(argv[0]);

        cout << "Scaning available devices..." << endl;

        int countdown = 2;
        while(!plugin_devs_ok && countdown)
        {
                if(get_plugin_dev() == -1)
                {
                        cerr << "Can't get local devices list, exit...\n";
                        exit(1);
                }

                plugin_devs_ok = plugin_dev.empty() ? false : true;
                countdown -= 1;
        }

        if(!countdown || !plugin_devs_ok)
        {
                cerr << "No available device! exit...\n";
                exit(0);
        }

        std::string chosen_dev = make_choice();

        // write chosen_dev to file
        std::ofstream of;
        of.open(SDEV_CONF_PATH, of.out | of.trunc);

        if(of.good())
                of << chosen_dev;
        else
        {
                cout << "Can't save chosen device, exit...\n";
                exit(1);
        }

        of.close();
        cout << "\e[35m" << chosen_dev << "\e[0m"
                << " has been activated!" << endl;

        return 0;
}
