#include <iostream>
#include <fcntl.h>
#include <unistd.h>
#include <thread>
#include <vector>
#include <atomic>

#define Log(log) {std::cout << std::endl << getpid() << "::"<< __func__ << ":" << __LINE__ << " " << log;}

class SimpleIPC{	
	public: 

		SimpleIPC(int prodCount, int msgCount): nProdCount(prodCount), nMsgCount(msgCount){
			fp = open("/dev/ipc_dev", O_RDWR, 0);
			if(fp <= 0){
				Log("\n open failed");
				throw std::invalid_argument("fopen failed");
			}		

			for (int i=0; i<nProdCount; i++){
				tProducer.emplace_back(std::thread(&SimpleIPC::producer, this, i));
			}
		}

		void producer(int id)
		{
			int nPerProducerMsgCount = nMsgCount;
			while(nPerProducerMsgCount>0){
				std::string s;

				s = "From pid: " + std::to_string(getpid()) + "-> HI there! Hello! " + std::to_string(id*nMsgCount + nPerProducerMsgCount-1);
				wr_len = write(fp, s.c_str(), s.length());
				if (wr_len > 0){
					nPerProducerMsgCount--;
					nTotalMsg++;
					Log("Producer-"<<id<<": " << s << " Total: " << nTotalMsg.load()); 
				}
			}
			Log(" - exit " << id);
		}

		void wait(){
			for (auto &t: tProducer){
				if (tProducer.back().joinable()){
					t.join();
				}
			}
			tProducer.clear();
		}


	private:
		int fp;
		int wr_len=0;
		int nProdCount=0;
		int nMsgCount=0;
		std::atomic<int> nTotalMsg{0};

		std::vector<std::thread> tProducer;
};

int main(){

	SimpleIPC ipc(10, 10);

	ipc.wait();
	return 0;
}
