CXX=clang++ -std=c++11 -Wall

all: sdev_ctl sdevd


sdev_ctl: sdev_ctl.o sudodev.o
	$(CXX) $^ -o $@ -lpthread

sdevd: sudodev.o sdevd.o
	$(CXX) $^ -o $@ -lpthread

install:
	@sudo install sdev_ctl /bin
	@sudo install sdevd /bin
	@echo "Install finished!"

uninstall:
	@sudo rm -f /bin/sdev_ctl /bin/sdevd
	@sudo rm -f /etc/sdev.conf
	@sudo rm -f /etc/sudoers.d/sdev
	@echo "Uninstall finished!"

clean:
	rm *.o sdev_ctl sdevd
