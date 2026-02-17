#include <iostream>
#include <fcntl.h>
#include <unistd.h>
#include <thread>
#include <vector>

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
			while(nMsgCount){
				std::string s;

				s = "From pid: " + std::to_string(getpid()) + "-> HI there! Hello! " + std::to_string(nMsgCount);

				wr_len = write(fp, s.c_str(), s.length());
				if (wr_len > 0){
					Log("Producer-"<<id<<": " << s); 
					nMsgCount--;
				}
			}
			Log(" - exit");
		}

		void wait(){
			while(!tProducer.empty()){
				if (tProducer.back().joinable()){
					tProducer.back().join();
					tProducer.pop_back();
				}
			}
		}


	private:
		int fp;
		int wr_len=0;
		int nProdCount=0;
		int nProdCountSave=0;
		int nMsgCount=0;

		std::vector<std::thread> tProducer;
};

int main(){

	SimpleIPC ipc(1, 30);

	ipc.wait();
	return 0;
}
