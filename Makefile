CXX=g++ -std=c++11 -Wall
ctrl = sdevctl
daemon = sdevd

all: $(ctrl) $(daemon)


$(ctrl): sdev_ctl.o sudodev.o
	$(CXX) $^ -o $@
	strip $@

$(daemon): sudodev.o sdevd.o
	$(CXX) $^ -o $@
	strip $@

install:
	@sudo install $(ctrl) /bin
	@sudo install $(daemon) /bin
	@echo "Install finished!"

uninstall:
	@sudo rm -f /bin/$(ctrl) /bin/$(daemon)
	@sudo rm -f /etc/sdev.conf
	@sudo rm -f /etc/sudoers.d/sdev
	@echo "Uninstall finished!"

clean:
	rm *.o $(ctrl) $(daemon)
