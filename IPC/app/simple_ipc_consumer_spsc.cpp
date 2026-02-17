#include <iostream>
#include <fcntl.h>
#include <unistd.h>
#include <thread>
#include <vector>

#define Log(log) {std::cout << std::endl << getpid() << "::" << __func__ << ":" << __LINE__ << " " << log;}

class SimpleIPC{	
	public: 

		SimpleIPC(int consumerCount, int msgCount): nConsumerCount(consumerCount), nMsgCount(msgCount){
			fp = open("/dev/ipc_dev", O_RDWR, 0);
			if(fp <= 0){
				Log("\n open failed");
				throw std::invalid_argument("fopen failed");
			}		

			for (int i=0; i<nConsumerCount; i++){
				tConsumer.emplace_back(std::thread(&SimpleIPC::consumer, this, i));
			}
		}

		void consumer(int id)
		{
			try{
				while(nMsgCount){
					int rd_len = read(fp, buf, 100);
					if (rd_len > 0){
						nMsgCount--;
					}
					buf[rd_len] = '\0';
					Log("Consumer"<<"-"<<id<<": " << buf); 
					rd_len = 0;
				}
			}catch(std::exception &ex){
				Log(" exp: " << std::string(ex.what()));
			}
			Log(" - exit");
		}

		void wait(){
			while(!tConsumer.empty()){
				if (tConsumer.back().joinable()){
					tConsumer.back().join();
					tConsumer.pop_back();
				}
			}
		}


	private:
		char buf[1024];
		int fp;
		int nConsumerCount=0;
		int nMsgCount=0;

		std::vector<std::thread> tConsumer;
};

int main(){

	try{
		SimpleIPC ipc(1, 30);

		ipc.wait();
	}catch(std::exception &ex){
		Log(" exp: " << std::string(ex.what()));
	}
	return 0;
}
