#include <iostream>
#include <fcntl.h>
#include <thread>

#define IAMHERE {std::cout << std::endl << __func__ << ":" << __LINE__;}
class SimpleIPC{	
	public: 

		SimpleIPC(int nCount): nProdCount(nCount), nConsumerCount(nCount){
			fp = open("/dev/ipc_dev", "rw+");
			IAMHERE
			if(fp == NULL){
				printf("\n fopen failed");
				throw std::invalid_argument("fopen failed");
			}		
			IAMHERE
			setvbuf(fp, NULL, _IONBF, 0);
			IAMHERE

			tProducer = std::thread(&SimpleIPC::producer, this);
			tConsumer = std::thread(&SimpleIPC::consumer, this);
		}

		void producer()
		{
			while(nProdCount){
				printf("\n fwrite: %d - %d", wr_len, nProdCount);
				char msg[] =  "HI there! Hello!";
				wr_len = write(msg, 1, sizeof(msg)+1, fp);
				printf("\n fwrite1: %d - %d", wr_len, nProdCount);
				if (wr_len > 0){
					nProdCount--;
				}
				printf("\n fwrite3: %d - %d", wr_len, nProdCount);
			}
			printf("\n %s - exit", __func__);
		}

		void consumer()
		{
			while(nConsumerCount){
			IAMHERE
				int rd_len = read(buf, 1, 100, fp);
			IAMHERE
				if (rd_len > 0){
					nConsumerCount--;
				}
			IAMHERE
				buf[rd_len] = '\0';
				printf("\nmsg: %s - %d - %d\n", buf, rd_len, nConsumerCount);
				rd_len = 0;
			}
			printf("\n %s - exit", __func__);
		}

		void wait(){
			tProducer.join();
			tConsumer.join();
		}


	private:
		char buf[1024];
		FILE *fp;
		int wr_len=0;
		int nProdCount=0;
		int nConsumerCount=0;

		std::thread tProducer;
		std::thread tConsumer;
};

int main(){

	SimpleIPC ipc(10);

	ipc.wait();
	return 0;
}
