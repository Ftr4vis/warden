TARGETS= warden warden_daemon
.PHONY: all clean

all: $(TARGETS)

clean:
	rm $(TARGETS)

warden: warden.cpp
	g++ -o warden warden.cpp

warden_daemon: warden_daemon.cpp
	g++ -o warden_daemon warden_daemon.cpp

