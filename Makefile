TITLE_COLOR = \033[33m
NO_COLOR = \033[0m

# when executing make, compile all exe's
all: sensor_gateway sensor_node file_creator

# When trying to compile one of the executables, first look for its .c files
# Then check if the libraries are in the lib folder
sensor_gateway : main.c connmgr.c datamgr.c sensor_db.c sbuffer.c lib/libdplist.so lib/libtcpsock.so
	@echo "$(TITLE_COLOR)\n***** CPPCHECK *****$(NO_COLOR)"
	cppcheck --enable=all --suppress=missingIncludeSystem main.c connmgr.c datamgr.c sensor_db.c sbuffer.c
	@echo "$(TITLE_COLOR)\n***** COMPILING sensor_gateway *****$(NO_COLOR)"
	gcc -c main.c      -Wall -std=c11 -Werror -DDEBUG -DSET_MIN_TEMP=10 -DSET_MAX_TEMP=20 -DTIMEOUT=5 -o main.o      -fdiagnostics-color=auto
	gcc -c connmgr.c   -Wall -std=c11 -Werror -DDEBUG -DSET_MIN_TEMP=10 -DSET_MAX_TEMP=20 -DTIMEOUT=5 -o connmgr.o   -fdiagnostics-color=auto
	gcc -c datamgr.c   -Wall -std=c11 -Werror -DDEBUG -DSET_MIN_TEMP=10 -DSET_MAX_TEMP=20 -DTIMEOUT=5 -o datamgr.o   -fdiagnostics-color=auto
	gcc -c sensor_db.c -Wall -std=c11 -Werror -DDEBUG -DSET_MIN_TEMP=10 -DSET_MAX_TEMP=20 -DTIMEOUT=5 -o sensor_db.o -fdiagnostics-color=auto
	gcc -c sbuffer.c   -Wall -std=c11 -Werror -DDEBUG -DSET_MIN_TEMP=10 -DSET_MAX_TEMP=20 -DTIMEOUT=5 -o sbuffer.o   -fdiagnostics-color=auto
	@echo "$(TITLE_COLOR)\n***** LINKING sensor_gateway *****$(NO_COLOR)"
	gcc main.o connmgr.o datamgr.o sensor_db.o sbuffer.o -ldplist -ltcpsock -lpthread -o sensor_gateway -Wall -L./lib -Wl,-rpath=./lib -lsqlite3 -fdiagnostics-color=auto

file_creator : file_creator.c
	@echo "$(TITLE_COLOR)\n***** COMPILE & LINKING file_creator *****$(NO_COLOR)"
	gcc file_creator.c -o file_creator -Wall -fdiagnostics-color=auto

sensor_node : sensor_node.c lib/libtcpsock.so
	@echo "$(TITLE_COLOR)\n***** COMPILING sensor_node *****$(NO_COLOR)"
	gcc -c sensor_node.c -D_GNU_SOURCE=1 -Wall -std=c11 -Werror -o sensor_node.o -fdiagnostics-color=auto
	@echo "$(TITLE_COLOR)\n***** LINKING sensor_node *****$(NO_COLOR)"
	gcc sensor_node.o -ltcpsock -o sensor_node -Wall -L./lib -Wl,-rpath=./lib -fdiagnostics-color=auto

# If you only want to compile one of the libs, this target will match (e.g. make liblist)
libdplist : lib/libdplist.so
libtcpsock : lib/libtcpsock.so

lib/libdplist.so : lib/dplist.c
	@echo "$(TITLE_COLOR)\n***** COMPILING LIB dplist *****$(NO_COLOR)"
	gcc -c lib/dplist.c -Wall -std=c11 -Werror -fPIC -o lib/dplist.o -fdiagnostics-color=auto
	@echo "$(TITLE_COLOR)\n***** LINKING LIB dplist< *****$(NO_COLOR)"
	gcc lib/dplist.o -o lib/libdplist.so -Wall -shared -lm -fdiagnostics-color=auto

lib/libtcpsock.so : lib/tcpsock.c
	@echo "$(TITLE_COLOR)\n***** COMPILING LIB tcpsock *****$(NO_COLOR)"
	gcc -c lib/tcpsock.c -Wall -std=c11 -Werror -fPIC -o lib/tcpsock.o -fdiagnostics-color=auto
	@echo "$(TITLE_COLOR)\n***** LINKING LIB tcpsock *****$(NO_COLOR)"
	gcc lib/tcpsock.o -o lib/libtcpsock.so -Wall -shared -lm -fdiagnostics-color=auto

# do not look for files called clean, clean-all or this will be always a target
.PHONY : clean clean-all run zip

clean:
	rm -rf *.o sensor_gateway sensor_node file_creator *~

clean-all: clean
	rm -rf lib/*.so
	rm -rf *.PID
	rm -rf gateway.log
	rm -rf room_sensor.map
	rm -rf sensor_data
	rm -rf sensor_data_recv

run_gateway:
	./sensor_gateway 3756

scalability : sensor_gateway sensor_node
			-- ./sensor_node 15 1 127.0.0.1 3756 & echo $$! > node1.PID  #save PID in .PID file
			-- ./sensor_node 21 2.5 127.0.0.1 3756 & echo $$! > node2.PID
	sleep 2
	-- ./sensor_node 37 3.4 127.0.0.1 3756 & echo $$! > node3.PID
	-- ./sensor_node 49 4.2 127.0.0.1 3756 & echo $$! > node4.PID
	-- ./sensor_node 112 6 127.0.0.1 3756 & echo $$! > node5.PID
	-- ./sensor_node 28 7 127.0.0.1 3756 & echo $$! > node6.PID    #to test invalid sensor node ID
	-- ./sensor_node 129 3 127.0.0.1 3756 & echo $$! > node7.PID
	@echo "All nodes are running"
	sleep 30
	if [ -e node7.PID ]; then \
    kill -TERM $$(cat node7.PID) || true; \
  fi;
	if [ -e node6.PID ]; then \
    kill -TERM $$(cat node6.PID) || true; \
  fi;
	if [ -e node5.PID ]; then \
    kill -TERM $$(cat node5.PID) || true; \
  fi;
	sleep 5
	if [ -e node4.PID ]; then \
    kill -TERM $$(cat node4.PID) || true; \
  fi;
	if [ -e node3.PID ]; then \
    kill -TERM $$(cat node3.PID) || true; \
  fi;
	sleep 5
	if [ -e node2.PID ]; then \
    kill -TERM $$(cat node2.PID) || true; \
  fi;
	if [ -e node6.PID ]; then \
    kill -TERM $$(cat node1.PID) || true; \
  fi;
durability : sensor_gateway sensor_node
				-- ./sensor_node 15 1 127.0.0.1 3756 & echo $$! > node1.PID  #save PID in .PID file
				-- ./sensor_node 21 2 127.0.0.1 3756 & echo $$! > node2.PID
		sleep 2
		-- ./sensor_node 37 3 127.0.0.1 3756 & echo $$! > node3.PID
		-- ./sensor_node 49 4 127.0.0.1 3756 & echo $$! > node4.PID
		-- ./sensor_node 112 8 127.0.0.1 3756 & echo $$! > node5.PID
		-- ./sensor_node 28 6 127.0.0.1 3756 & echo $$! > node6.PID    #to test invalid sensor node ID
		-- ./sensor_node 129 3 127.0.0.1 3756 & echo $$! > node7.PID
		@echo "All nodes are running"
		sleep 3600
		if [ -e node7.PID ]; then \
	    kill -TERM $$(cat node7.PID) || true; \
	  fi;
		if [ -e node6.PID ]; then \
	    kill -TERM $$(cat node6.PID) || true; \
	  fi;
		if [ -e node5.PID ]; then \
	    kill -TERM $$(cat node5.PID) || true; \
	  fi;
		sleep 5
		if [ -e node4.PID ]; then \
	    kill -TERM $$(cat node4.PID) || true; \
	  fi;
		if [ -e node3.PID ]; then \
	    kill -TERM $$(cat node3.PID) || true; \
	  fi;
		sleep 5
		if [ -e node2.PID ]; then \
	    kill -TERM $$(cat node2.PID) || true; \
	  fi;
		if [ -e node6.PID ]; then \
	    kill -TERM $$(cat node1.PID) || true; \
	  fi;

stress : sensor_gateway sensor_node
	
					-- ./sensor_node 15 10 127.0.0.1 3756 & echo $$! > node1.PID  #save PID in .PID file
					-- ./sensor_node 21 25 127.0.0.1 3756 & echo $$! > node2.PID
			sleep 1
			-- ./sensor_node 37 45 127.0.0.1 3756 & echo $$! > node3.PID
			-- ./sensor_node 49 55 127.0.0.1 3756 & echo $$! > node4.PID
			-- ./sensor_node 112 1300 127.0.0.1 3756 & echo $$! > node5.PID
			-- ./sensor_node 28 1200 127.0.0.1 3756 & echo $$! > node6.PID    #to test invalid sensor node ID
			-- ./sensor_node 129 75 127.0.0.1 3756 & echo $$! > node7.PID
			@echo "All nodes are running"
			sleep 5
			if [ -e node7.PID ]; then \
		    kill -TERM $$(cat node7.PID) || true; \
		  fi;
			if [ -e node6.PID ]; then \
		    kill -TERM $$(cat node6.PID) || true; \
		  fi;
			if [ -e node5.PID ]; then \
		    kill -TERM $$(cat node5.PID) || true; \
		  fi;
			sleep 1
			if [ -e node4.PID ]; then \
		    kill -TERM $$(cat node4.PID) || true; \
		  fi;
			if [ -e node3.PID ]; then \
		    kill -TERM $$(cat node3.PID) || true; \
		  fi;
			sleep 1
			if [ -e node2.PID ]; then \
		    kill -TERM $$(cat node2.PID) || true; \
		  fi;
			if [ -e node6.PID ]; then \
		    kill -TERM $$(cat node1.PID) || true; \
		  fi;


valgrind : sensor_gateway sensor_node
	-- ./sensor_node 15 2.4 127.0.0.1 3756 & echo $$! > node1.PID  #save PID in .PID file
	-- ./sensor_node 21 2.1 127.0.0.1 3756 & echo $$! > node2.PID
				@echo "All nodes are running"
				sleep 25
				if [ -e node2.PID ]; then \
			    kill -TERM $$(cat node2.PID) || true; \
			  fi;
				if [ -e node6.PID ]; then \
			    kill -TERM $$(cat node1.PID) || true; \
			  fi;

valgrind_gateway:
	valgrind --tool=memcheck --leak-check=yes --track-origins=yes --show-reachable=yes --num-callers=20 --track-fds=yes ./sensor_gateway 3756



zip:
	zip lab_final.zip main.c main.h code_review_checklist.ods errmacros.h connmgr.c connmgr.h datamgr.c datamgr.h sbuffer.c sbuffer.h sensor_db.c sensor_db.h config.h lib/dplist.c lib/dplist.h lib/tcpsock.c lib/tcpsock.h

zip2:
	zip lab_final_test_computer.zip main.c main.h errmacros.h connmgr.c connmgr.h file_creator.c sensor_node.c datamgr.c datamgr.h Makefile sbuffer.c sbuffer.h sensor_db.c sensor_db.h config.h lib/dplist.c lib/dplist.h lib/tcpsock.c lib/tcpsock.h