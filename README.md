# 描述
[English](./README_en.md)

**sudodev** 是一个设备(特指U盘)让你使用`sudo`而不需要密码的程序   

看到[这里](https://github.com/Arondight/sudodev)后,用C++写了一遍
初学C++算是练习吧

影响的系统文件:
- /etc/sudoers
- /etc/group

# 如何工作
- 这个程序会创建一个`group`叫`sdevuser`
- 你必须将自己加入`sudoers`通过`visudo`或者其他方式，并且，你必须是`sdevusers`的成员
- 在你的`/etc/sudoers`文件中有以下一行代码
```plain
#includedir /etc/sudoers.d
```
如果没有，程序会试图添加进去，但是这不能保证你可以不用输入密码

有了以上条件，你就可以使用`sdev_ctl`的`add`或`del`选项来添加或删除你的外部设备，一旦`sdevd`监测到了你添加的设备，你就可以使用sudo而不需要输入密码了

# 建议
- 使用前请备份**影响的系统文件**
- 你可以将`sdevd`写入系统的启动脚本，具体方法参见你所用发行版的wiki,也可以参考[这里](https://github.com/Arondight/sudodev/tree/master/init)
- 如果`sdevd`意外结束导致程序不能工作，请删除`/var/run/sdevd.pid`

# 致谢
非常感谢学姐[@Arondight](https://github.com/Arondight)做了这个很棒的东西
