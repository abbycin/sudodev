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
#include <fcntl.h>
#include <dirent.h>
#include <string.h>
#include <sys/stat.h>
#include <libgen.h>
#include <stdio.h>
#include <signal.h>

using std::endl;
using std::cerr;

#define LIMITED_PATH_LEN	256

/*
 * global varibales for program config
 */
std::string DROP_IN = "#includedir /etc/sudoers.d";
std::string PRIVILAGE = "%sdevuser ALL=(ALL) NOPASSWD: ALL";

/*
 * global variables for devices discovering
 */
std::vector<std::string> local_dev;
std::vector<std::pair<std::string, std::string>> all_dev;  // <partition, uuid>
std::map<int, std::string> plugin_dev;
std::map<int, std::string> plugin_dev_uuid;

/*
 * no matter fstab is pure `/dev/xx` format or pure `UUID=xx`
 * format or even both, we convert them into `/dev/xx` format
 */
static std::string ltrim(std::string &line)
{
        std::string::iterator beg = line.begin();
        std::string::iterator end = line.end();
        std::string::iterator iter;
        size_t idx = 0;

        for(iter = beg; iter != end && std::isspace(*iter); ++iter)
                idx += 1;

        if(iter == end)
                line = "";
        else
                line = line.substr(idx);

        return line;
}

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
                        if(line[0] == '#' || line == "")
                                continue;
                        ltrim(line);
                        if(line[0] == '#' || line == "")
                                continue;

			ss.str(line);
			ss >> key;

                        memset(buf, 0, sizeof(buf));

                        if(key.find("UUID") != key.npos)
                        {
                                line = key.substr(strlen("UUID") + 1);
                                path = INTERFACE_PATH;
                                path += "/" + line;
                                readlink(path.c_str(), buf, sizeof(buf));
                        }
                        else
                                memcpy(buf, key.c_str(), key.length());

			local_dev.push_back(std::string(basename(buf)));
                        ss.clear();
		}
	}
	else
		return -1;
	
        in.close();

	return 0;
}

/*
 * get all devices under /dev/disk/by-uuid in `/dev/xx`
 * format. 
 */
int get_all_dev()
{
	all_dev.clear();

	char buf[LIMITED_PATH_LEN] = {0};
	DIR *pdir = NULL;
	struct dirent *dp = NULL;
        std::string path, key, val;
        struct stat st;

	pdir = opendir(INTERFACE_PATH);
	
	if(pdir == NULL)
		return -1;

        chdir(INTERFACE_PATH);
	while((dp = readdir(pdir)))
	{
                if(dp->d_name[0] == '.')
                        continue;
                
                memset(&st, 0, sizeof(st));
                if(lstat(dp->d_name, &st) == -1)
                        return -1;

                if(S_ISLNK(st.st_mode))
                {
                        val = dp->d_name;
                        path = INTERFACE_PATH;
                        path += "/";
                        path += dp->d_name;
                        readlink(path.c_str(), buf, sizeof(buf));
                        key = basename(buf);
                        all_dev.push_back(std::make_pair(key, val));
                        memset(buf, 0, sizeof(buf));
                }
	}
	
	closedir(pdir);
        chdir("/");

	return 0;
}

/*
 * compare local devices in fstab and devices under /dev/disk/by-uuid
 * to find out non-local devices
 */ 
int get_plugin_dev()
{
        if(get_local_dev() == -1 || get_all_dev() == -1)
                return -1;

        for(const auto &local: local_dev)
        {
                all_dev.erase(std::remove_if(all_dev.begin(), all_dev.end(),
                        [local](std::pair<std::string, std::string> x)
                        {
                                return (local.substr(0,3) == x.first.substr(0,3));
                        }), all_dev.end());
        }

	plugin_dev.clear();
        plugin_dev_uuid.clear();

        int idx = 0;
	for(const auto &x: all_dev)
	{
		plugin_dev[idx] = x.first;
                plugin_dev_uuid[idx] = x.second;
                idx += 1;
	}

        return 0;
}

/*
 * list all available plugin in devices
 */
void list_dev()
{
        for(const auto &x: plugin_dev)
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
        bool drop_in = true;

        struct stat st;

        memset(&st, 0, sizeof(st));

        stat(SUDOERS_PATH, &st);

        chmod(SUDOERS_PATH, st.st_mode | S_IWUSR);

        fd.open(SUDOERS_PATH, fd.in | fd.out | fd.app);
        if(!fd.good())
                return -1;

        while(std::getline(fd, line))
        {
                if(line == DROP_IN)
                {
                        fd.close();
                        drop_in = false;
                        break;
                }
        }

        if(drop_in)
                fd << DROP_IN << endl;

        fd.close();

        chmod(SUDOERS_PATH, st.st_mode & (~S_IWUSR));

        return 0;
}

/*
 * if a qualified device is in plugin_devs list
 * then return true
 *
 * @ call from daemon
 */
bool is_qualified_device(std::string &qualified_dev)
{
        get_plugin_dev();

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

        chmod(SUDO_CONF_PATH, S_IRUSR);

        return 0;
}

/*
 * check if there's a daemon
 */

int record_pid(std::string &msg)
{
        struct flock flk;
        pid_t pid = getpid();
        int fd = 0;
        char buf[10] = {0};

        memset(&flk, 0, sizeof(flk));
        flk.l_type = F_WRLCK;
        flk.l_whence = SEEK_SET;

        fd = open(PID_PATH, O_CREAT | O_RDWR, S_IRUSR | S_IWUSR);
        if(fd == -1)
        {
                msg = "Failed to create pid file, exit...";
                return -1;
        }
 
        if(fcntl(fd, F_SETLK, &flk) == -1)
        {
                msg = "A daemon is running, eixt...";
                return -1;
        }
       
        if(ftruncate(fd, 0) == -1)
        {
                msg = "Can't truncate file";
                return -1;
        }

        sprintf(buf, "%d", pid);
        if(write(fd, buf, strlen(buf)) == -1)
        {
                msg = std::string(strerror(errno));
                return -1;
        }

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
                exit(0);

        setsid();

        if((pid = fork()) < 0)
                return -1;
        else if(pid != 0)
                exit(0);

        return 0;
}

int log_truncate()
{
        struct stat st;
        memset(&st, 0, sizeof(st));

        stat(LOG_PATH, &st);

        if(st.st_size >= LOG_SIZE_LIMIT)
        {
                if(truncate(LOG_PATH, 0) == -1)
                        return -1;
        }
        return 0;
}
