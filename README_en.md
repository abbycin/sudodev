# Description
[中文](./README.md)

**sudodev:** A device(specificly USB Drive) let you use `sudo` without password

After reading the code [here](https://github.com/Arondight/sudodev), I worte this program in C++, and I'm C++ beginner, so it's also a practice.

Affected Files:
- /etc/sudoers
- /etc/group

# How it works
- This program will create a group `sdevuser`
- You must add yourself to `sudoers` via `visudo` or other method, and you must be a member of `sdevuser`
- The following code shall exists in your `/etc/sudoers`
```plain
#includedir /etc/sudoers.d
```
if it's NOT exits, program will try to append it to your `/etc/sudoers`, but this can not guarantee this program will work correctly

If above conditions are all satisfied, you can run `sdev_ctl` with `add` or `del` option to add or delete your device. Once `sdevd` detected the device, you will find `sudo` doesn't need password anymore.

# Suggestions
- Before running, please backup all **affect files**
- You may want to  write a *[init](https://en.wikipedia.org/wiki/Init)* script, please read the wiki of your Linux distribution or take a look over [here](https://github.com/Arondight/sudodev/tree/master/init)
- If `sdevd` exits unexpectedly, please remove `/var/run/sdevd.pid` manually

# Thanks
[@Arondight](https://github.com/Arondight) Thanks for creating such a great project
