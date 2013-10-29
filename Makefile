APP = epoll_server
CXX = g++ -g -Wall -O3 -march=native -std=c++0x -pthread

OBJS = main.o ListenThread.o TaskThread.o iThread.o iEpoll.o iSocket.o 
TEST = Test.o iThread.o iEpoll.o iSocket.o 

all: ${APP} test

${APP}: ${OBJS}
	${CXX} ${OBJS} -o $@
	
test: ${TEST}
	${CXX} ${TEST} -o $@

main.o: main.cpp
ListenThread.o: ListenThread.cpp
TaskThread.o: TaskThread.cpp
iThread.o: iThread.cpp
iEpoll.o: iEpoll.cpp
iSocket.o: iSocket.cpp

Test.o: Test.cpp

clean:
	rm -rf *.o *.a *.so ${APP} test
