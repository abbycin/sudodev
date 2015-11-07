/*********************************************************
          File Name:sudodev.h
          Author: Abby Cin
          Mail: abbytsing@gmail.com
          Created Time: Sat 07 Nov 2015 08:49:23 AM CST
**********************************************************/

/*
 * executable name: sdev
 * 
 * this program support two types of fstab format
 * one is start with `/dev/xxx`, another is start 
 * with `UUID=xxx`
 */


#ifndef SUDO_DEV_H_
#define SUDO_DEV_H_


/*
 * no matter fstab is pure `/dev/xx` format or pure `UUID=xx`
 * format or even both, we convert them into `/dev/xx` format
 */
int get_local_dev();

/*
 * get all devices under /dev/disk/by-uuid in `/dev/xx`
 * format. 
 */
int get_all_dev();

/*
 * compare local devices in fstab and devices under /dev/disk/by-uuid
 * to find out non-local devices
 */ 
void get_plugin_dev();

/*
 * we monitor if one or more devices is connecting computer
 * in a infinite loop
 */
void *trigger(void *arg);

/*
 * privilege guard, grant or drop privilege
 */
void *privilege_guard(void *arg);

/*
 * list all available plugin in devices
 */
void list_dev();

/*
 * create a group
 */
int add_group();

/*
 * enable drop-in file
 */
int drop_in();

/*
 * determine if a device is qualified
 */
bool is_qulified_device();

/*
 * signal handlers for daemon
 */
int install_handler(int , void (*handler)(int));

void hup_handler(int);

void term_handler(int);

/*
 * manipulate privilege
 */
int privilege(bool is_grant);

/*
 * check if there's a daemon is running
 */
bool is_locked();

/*
 * record daemon's pid
 */
int record_pid();

/*
 * make a dameon
 */
int to_daemon();

#endif