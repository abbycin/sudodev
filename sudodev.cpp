/*********************************************************
          File Name:sudodev.cpp
          Author: Abby Cin
          Mail: abbytsing@gmail.com
          Created Time: Sat 07 Nov 2015 09:05:59 PM CST
**********************************************************/

#include "sudodev.h"
#include <algorithm>
#include <iostream>
#include <sstream>
#include <fstream>
#include <vector>
#include <map>
#include <string>
#include <unistd.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/resource.h>
#include <sys/file.h>
#include <fcntl.h>
#include <dirent.h>
#include <string.h>
#include <sys/stat.h>
#include <libgen.h>
#include <stdio.h>
#include <pthread.h>
#include <signal.h>

using std::endl;
using std::cerr;

#define LIMITED_PATH_LEN	256

/*
 * global variable for writing logs
 */

std::ofstream logs;

/*
 * global varibales for program config
 */
const char *PID_PATH = "/var/run/sdev.pid";
const char *FSTAB_PATH = "/etc/fstab";
const char *SUDOERS_PATH = "/etc/sudoers";
const char *SUDO_CONF_PATH = "/etc/sudoers.d/sdev";
const char *GROUP_PATH = "/etc/group";
const char *SDEV_CONF_PATH = "/etc/sdev.conf";	// program configuration file path
const char *SDEV_GROUP_NAME = "sdevuser";
const char *INTERFACE_PATH = "/dev/disk/by-uuid";
const char *LOG_PATH = "/var/log/sdevd.log";

std::string DROP_IN = "#includedir /etc/sudoers.d";
const char *PRIVILAGE = "%sdevuser ALL=(ALL) NOPASSWD: ALL";

/*
 * global variables for devices discovering
 */
std::vector<std::pair<std::string, bool>> local_dev;
std::vector<std::string> all_dev;
std::vector<std::string> all_dev_uuid;
std::map<int, std::string> plugin_dev;
std::map<int, std::string> plugin_dev_uuid;

/*
 * global variables for status changing
 */
bool working = false;
bool exit_flag = false;
bool plugin_devs_ok = false;

/*
 * global value for signal handing
 */
bool sig_hup = false;             // for SIGHUP
bool sig_term = false;            // for SIGTERM

/*
 * uuid in /etc/sdev.conf
 */
std::string qualified_dev;

/*
 * no matter fstab is pure `/dev/xx` format or pure `UUID=xx`
 * format or even both, we convert them into `/dev/xx` format
 */
int get_local_dev()
{
	local_dev.clear();
	
	std::ifstream in(FSTAB_PATH);
	std::string line, key, path;
	std::stringstream ss;
        char buf[LIMITED_PATH_LEN] = {0};
	
	if(in.good())
	{
		while(std::getline(in, line))
		{
			ss.str(line);
			ss >> key;
                        if(key.find("UUID") != key.npos)
                        {
                                line = key.substr(strlen("UUID") + 1);
			        local_dev.push_back(std::make_pair(line, true));
                        }
                        else
			        local_dev.push_back(std::make_pair(key, false));
			ss.clear();
		}
	}
	else
		return -1;
	
        in.close();

        for(auto iter = local_dev.begin(); iter != local_dev.end(); ++iter)
        {
                if(iter->second)
                {
                        path = INTERFACE_PATH;
                        path += "/" + iter->first;
                        readlink(path.c_str(), buf, sizeof(buf));
                        iter->first = std::string(basename(buf));
                        memset(buf, 0, sizeof(buf));
                }
        }

	return 0;
}

/*
 * get all devices under /dev/disk/by-uuid in `/dev/xx`
 * format. 
 */
int get_all_dev()
{
	all_dev.clear();
        all_dev_uuid.clear();

	char buf[LIMITED_PATH_LEN] = {0};
	DIR *pdir = NULL;
	struct dirent *dp = NULL;
        std::string path;
	
	pdir = opendir(INTERFACE_PATH);
	
	if(pdir == NULL)
		return -1;

	while((dp = readdir(pdir)))
	{
                if(dp->d_name[0] == '.')
                        continue;
                
                if(dp->d_type && DT_BLK)
                {
                        all_dev_uuid.push_back(std::string(dp->d_name));
                        path = INTERFACE_PATH;
                        path += "/";
                        path += dp->d_name;
                        readlink(path.c_str(), buf, sizeof(buf));
                        all_dev.push_back(std::string(basename(buf)));
                        memset(buf, 0, sizeof(buf));
                }
	}
	
	closedir(pdir);
	
	return 0;
}

/*
 * compare local devices in fstab and devices under /dev/disk/by-uuid
 * to find out non-local devices
 */ 
void get_plugin_dev()
{
	for(const auto &local: local_dev)
	{
		for(size_t i = 0; i < all_dev.size(); ++i)
		{
			if(local.first.substr(0,3) == all_dev[i].substr(0,3))
                        {
                                all_dev_uuid[i] = "";
                                all_dev[i] = "";
                        }
		}
	}
	
	plugin_dev.clear();
        plugin_dev_uuid.clear();

        int idx = 0;
	for(size_t i = 0; i < all_dev.size(); ++i)
	{
                if(all_dev[i] != "")
                {
		        plugin_dev[idx] = all_dev[i];
                        plugin_dev_uuid[idx] = all_dev_uuid[i];
                        idx += 1;
                }
	}
}

/*
 * we monitor if one or more devices is connecting computer
 * in a infinite loop
 */
void *trigger(void *arg)
{
        logs << arg;
	while(!exit_flag && !sig_hup && !sig_term)
	{
		if(get_local_dev() == -1)
		{
			logs << "Can't get local devices list, exit..." << endl;
		}
		if(get_all_dev() == -1)
		{
			logs << "Can't get all devices list, exit..." << endl;
		}
		
		get_plugin_dev();
		
		plugin_devs_ok = plugin_dev.empty() ? false : true;

		sleep(1);
	}

        pthread_exit(0);
}

/*
 * privilege guard
 */
void *privilege_guard(void *arg)
{
        logs << arg << endl;

        while(!exit_flag)
        {
                while(!plugin_devs_ok)
                        sleep(1);
                while(!is_qulified_device())
                        sleep(1);

                if(privilege(true) == -1)
                        logs << "Can't grant privilege\n";
                else
                        logs << qualified_dev << " granted privilege\n";

                cerr << qualified_dev << endl;
                while(is_qulified_device())
                        sleep(1);
                privilege(false);
                logs << qualified_dev << " droped privilege\n";
        }

        pthread_exit(0);
}

/*
 * list all available plugin in devices
 */

void list_dev()
{
        for(auto &x: plugin_dev)
        {
                std::cout << "[" << x.first << "]" << ". "
                        << x.second << endl;
        }
}

/*
 * create a record in /etc/group 
 */
static auto split_str(std::string s, char delimiter) -> std::vector<std::string>
{
        std::stringstream ss(s);
        std::string word;
        std::vector<std::string> res;

        while(std::getline(ss, word, delimiter))
                res.push_back(word);

        return res;
}

int add_group()
{
        std::ifstream in(GROUP_PATH);
        std::vector<int> contents;
        std::string line;
        int gid;

        if(in.good())
        {
                while(std::getline(in, line))
                {
                        if(line.find(SDEV_GROUP_NAME) != line.npos)
                        {
                                in.close();
                                return 0;
                        }
                        contents.push_back(std::stod(split_str(line, ':')[2]));
                }
        }
        else
                return -1;
        in.close();

        auto beg = contents.begin();
        auto end = contents.end();

        for(int i = 1000; i < 2000; ++i)
        {
                if(std::find(beg, end, i) == contents.end())
                {
                        gid = i;
                        break;
                }
        }

        std::ofstream of;

        of.open(GROUP_PATH, of.out | of.app);

        if(of.good())
        {
                of << SDEV_GROUP_NAME << ":x:" << gid << ":" << endl;
        }
        else
                return -1;
        of.close();

        return 0;
}

/* 
 * enable drop-in
 */
int drop_in()
{
        std::fstream fd;
        std::string line;

        fd.open(SUDOERS_PATH, fd.in | fd.out | fd.app);
        if(fd.good())
        {
                while(std::getline(fd, line))
                {
                        if(line == DROP_IN)
                        {
                                fd.close();
                                return 0;
                        }
                }

                fd << DROP_IN << endl;
        }
        else
                return -1;

        fd.close();

        return 0;
}

/*
 * if a qualified device is in plugin_devs list
 * then return true
 *
 * @ call from daemon
 */
bool is_qulified_device()
{
        if(access(SDEV_CONF_PATH, F_OK) != 0)
                return false;

        std::ifstream in(SDEV_CONF_PATH);
        bool res = false;

        if(in.good())
        {
                std::getline(in, qualified_dev);
                if(qualified_dev == "")
                {
                        in.close();
                        return false;
                }
        }
        else
                return false;
        in.close();

        for(const auto &x: plugin_dev_uuid)
        {
                if(qualified_dev == x.second)
                {
                        res = true;
                        break;
                }
        }

        return res;
}

/*
 * signal handlers for daemon
 */
int install_handler(int sig, void (*handler)(int))
{
        struct sigaction sa;

        sigemptyset(&sa.sa_mask);
        sigaddset(&sa.sa_mask, sig);
#ifdef SA_RESTART
        sa.sa_flags = SA_RESTART;
#else
        sa.sa_flags = 0;
#endif
        sa.sa_handler = handler;

        return sigaction(sig, &sa, NULL);
}

void hup_handler(int sig)
{
        logs << "Cought signal " << sig << " restart..." << endl;
        sig_hup = true;
}

void term_handler(int sig)
{
        logs << "Cought signal " << sig << " exit..." << endl;
        sig_term = true;
        exit_flag = true;
}

/*
 * manipulate privilege
 */
int privilege(bool is_grant)
{
        // no matter file exsit or not, remvoe it
        // and ignore return value of `remove`
        if(!is_grant)
        {
                remove(SUDO_CONF_PATH);
                return 0;
        }

        std::ofstream of;

        if(access(SUDO_CONF_PATH, F_OK) == 0)
        {
                if(remove(SUDO_CONF_PATH) == -1)
                        return -1;
        }

        of.open(SUDO_CONF_PATH, of.out);

        if(of.good())
                of << PRIVILAGE << endl;
        else
                return -1;

        of.close();

        return 0;
}

/*
 * check if there's a daemon
 */

int record_pid()
{
        struct flock flk;
        pid_t pid = getpid();
        int fd = 0;

        memset(&flk, 0, sizeof(flk));
        flk.l_type = F_WRLCK;
        flk.l_whence = SEEK_SET;

        fd = open(PID_PATH, O_CREAT | O_RDWR, S_IRUSR | S_IWUSR);
        if(fd == -1)
        {
                logs << "Failed to create pid file, exit..." << endl;
                return -1;
        }
 
        if(fcntl(fd, F_SETLK, &flk) == -1)
        {
                logs << "A daemon is running, eixt..." << endl;
                close(fd);
                return -1;
        }
       
        if(ftruncate(fd, 0) == -1)
        {
                logs << "Can't truncate file\n";
                return -1;
        }

        if(write(fd, &pid, sizeof(pid_t)) == -1)
        {
                perror("Write");
                return -1;
        }

        close(fd);

        return 0;
}

/*
 * daemon
 */
int to_daemon()
{
        chdir("/");
        chroot("/");

        umask(0);

        int fd0, fd1, fd2;
        pid_t pid = 0;
        struct rlimit limit;
        size_t i = 0;

        if(getrlimit(RLIMIT_NOFILE, &limit) < 0)
                return -1;

        for(; i < limit.rlim_max; ++i)
                close(i);

        fd0 = open("/dev/null", O_RDWR);
        fd1 = dup(0);
        fd2 = dup(0);

        if(fd0 != 0 || fd1 != 1 || fd2 != 2)
                return -1;

        if((pid = fork()) < 0)
                return -1;
        else if(pid != 0)
                exit(1);

        setsid();

        if((pid = fork()) < 0)
                return -1;
        else if(pid != 0)
                exit(1);

        return 0;
}
