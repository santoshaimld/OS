#include <iostream>
#include <unistd.h>
#include <atomic>
#include <vector>
#include <sys/epoll.h>
#include <fcntl.h>

#define Log(log) {std::cout << getpid() << "::"<< __func__ << ":" << __LINE__ << " " << log << std::endl;}

class SimpleEpollIPC {
	public:
		SimpleEpollIPC(int prodCount, int msgCount): nProdCount(prodCount), nMsgCount(msgCount){
			fp = open("/dev/ipc_dev", O_RDWR | O_NONBLOCK);
			if(fp <= 0){
				Log("\n open failed");
				throw std::invalid_argument("fopen failed");
			}		
			epfd = epoll_create1(0);

			ev.events = EPOLLIN | EPOLLOUT;
			ev.data.fd = fp;
			epoll_ctl(epfd, EPOLL_CTL_ADD, fp, &ev);

			events.resize(prodCount);
		}

	void epollIpc(){
		int i=0;

		while(!isExit){
			nfds = epoll_wait(epfd, events.data(), nProdCount*nMsgCount, -1);
			i = (i++)%nfds;
		
			if (events[i].events & EPOLLOUT){
				std::string s;

				nTotalMsgWr++;
				s = "From pid: " + std::to_string(getpid()) + "-> HI there! Hello! " + std::to_string(i) + " wr: " + std::to_string(nTotalMsgWr.load()) + " rd: " + std::to_string(nTotalMsgRd.load()) ;
	
				int n = write(events[i].data.fd, s.c_str(), s.length());
				Log(" Produce id: " << i << " " << s );
			}
			if (nTotalMsgWr.load() == 100){
				isExit = true;
			}
		}	
	}

	private:
		bool isExit = false;
		int fp;
		int nProdCount=0;
		int nMsgCount=0;
		std::atomic<int> nTotalMsgWr{0};
		std::atomic<int> nTotalMsgRd{0};
		struct epoll_event ev;
		std::vector<struct epoll_event> events;
		int nfds;
		int epfd;
};

int main(){

	SimpleEpollIPC ipc(10, 10);

	ipc.epollIpc();
	return 0;
}
