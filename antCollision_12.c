#include <stdio.h>
#include <stdlib.h>
#include <memory.h>
#include <time.h>
#include <math.h>



#define MAX_EVENT_NUM		4096										// 可容纳的最大事件数量
/*

#define initialDevNum		50										// 初始设备数量
#define Pmute				((double) 1)							// 静默概率
#define P					((unsigned int)250000)					// 每个时钟发生事件的概率（倒数）
#define K					((unsigned int)8)						// 第一次碰撞，产生随机数的范围
#define S					0										// 在S秒之后有Sn个新设备加入
#define Sn					20										// 若S为0则不会加入新的设备
#define R					50										// 数据率KHz
#define REG_AT_START		1										// 一开始是否注册
#define RUN_TIME			((unsigned long)(R * 1000 * 5))			// 运行时间
#define BIT_ADD				32
*/

typedef struct Device{
	char			muteFlag;
	char 			eventFlag;
	unsigned int	eventCnt;
	char 			sending;
	unsigned long 	sendBitRemain;
	unsigned long 	rollbackNum;
	int 			collisionCnt;
	unsigned int 	clkWaitToSend;
	unsigned long 	eventTime[MAX_EVENT_NUM];
	unsigned long 	eventRspnTime[MAX_EVENT_NUM];
	unsigned long 	totalRspnTime;
	char 			muteRecord;
}Device_t, *pDevice_t;

pDevice_t dev[512];


int initialDevNum;// 初始设备数量
double Pmute;				// 静默概率
unsigned int P;					// 每个时钟发生事件的概率（倒数）
unsigned int Pstep = 200;
unsigned int K;					// 第一次碰撞，产生随机数的范围
unsigned int T;					// 运行时间
int S;					// 在S秒之后有Sn个新设备加入
int Sn;					// 若S为0则不会加入新的设备
int R;					// 数据率KHz
int REG_AT_START;		// 一开始是否注册
unsigned long RUN_TIME;			// 运行时间
unsigned long BIT_ADD;				


unsigned long totalClk = 0;
unsigned int upLinkTotal = 0;				// 当前正在占用上行信道的设备数
unsigned long totalSendBit = 0;
double throughput = 0;
unsigned long totalEvent = 0;
unsigned long totalBlockBit = 0;
unsigned long totalBlockBitPre = 0;
unsigned long totalUsedClk = 0;
unsigned long totalTimeDelay = 0;
double aveDelay = 0;
unsigned long printEntCnt = 0;
int devNum = 0;
unsigned long firstSendOutTime = 0; // 第一次把所有比特发送完的时间
unsigned long totalCollision = 0;


int devNumMin;
int devNumMax;
double pMuteMin;
double pMuteMax;
unsigned int pMin;
unsigned int pMax;
unsigned long bitAddMin;
unsigned long bitAddMax;
unsigned int kMin;
unsigned int kMax;
int varParaMask = 0;
unsigned int regCnt = 0;




void initParameter(void){
	int ans;
	printf("请输入initialDevNum:");
	scanf("%d", &initialDevNum);
	printf("请输入静默概率Pmute:");
	scanf("%lf", &Pmute);
	printf("请输入事件概率P:");
	scanf("%u", &P);
	printf("请输入第一次碰撞随机数范围K:");
	scanf("%u", &K);
	printf("请输入运行时间T（秒）:");
	scanf("%u", &T);
	printf("请输入加入新设备时间S（秒）:");
	scanf("%d", &S);
	printf("请输入加入设备数量Sn:");
	scanf("%d", &Sn);
	printf("请输入速率R:");
	scanf("%d", &R);
	printf("请输入一开始是否注册REG_AT_START:");
	scanf("%d", &REG_AT_START);
	printf("请输入事件比特数BIT_ADD:");
	scanf("%lu", &BIT_ADD);
	RUN_TIME=((unsigned long)(R * 1000 * T));// 运行时间


	printf("initialDevNum: %d\n", initialDevNum);
	printf("Pmute: %.2f\n", Pmute);
	printf("P: %u\n", P);
	printf("K: %u\n", K);
	printf("T: %u\n", T);
	printf("S: %d\n", S);
	printf("Sn: %d\n", Sn);
	printf("R: %d\n", R);
	printf("REG_AT_START: %d\n", REG_AT_START);
	printf("BIT_ADD: %lu\n", BIT_ADD);
	printf("RUN_TIME: %lu\n",RUN_TIME);

	printf("请选择变参: 0 设备数量，1 静默概率，2 事件比特数，3 首次碰撞范围，4 事件概率\n");
	scanf("%d", &ans);
	printf("请输入最小值和最大值：\n");

	devNumMin = initialDevNum, devNumMax = initialDevNum;
	pMuteMin = Pmute, pMuteMax = Pmute;
	bitAddMin = BIT_ADD, bitAddMax = BIT_ADD;
	kMin = K, kMax = K;
	pMin = P, pMax = P;
	varParaMask = 0;
	switch(ans){
		case 0:
			varParaMask = 1;
			scanf("%d %d", &devNumMin, &devNumMax);
			printf("获取成功，devNumMin=%d devNumMax=%d\n", devNumMin, devNumMax);
			break;
		case 1:
			varParaMask = 2;
			scanf("%lf %lf", &pMuteMin, &pMuteMax);
			printf("获取成功，pMuteMin=%lf pMuteMax=%lf\n", pMuteMin, pMuteMax);
			break;
		
		case 2:
			varParaMask = 4;
			scanf("%lu %lu", &bitAddMin, &bitAddMax);
			printf("获取成功，bitAddMin=%lu bitAddMax=%lu\n", bitAddMin, bitAddMax);
			break;
		case 3:
			varParaMask = 8;
			scanf("%u %u", &kMin, &kMax);
			printf("获取成功，kMin=%d kMax=%d\n", kMin, kMax);
			break;

		case 4:
			varParaMask = 16;
			scanf("%u %u", &pMin, &pMax);
			printf("获取成功，pMin=%d pMax=%d\n", pMin, pMax);
			break;

	}
}


int isEventCreate(unsigned int probability){
	unsigned rslt;
	rslt = 1 + random() % probability;
	if(rslt == 1)
		return 1;
	return 0;
}

int isMute(double pMute){
	int ans;
	ans = (1 + random() % 1000);
	if(ans <= 1000 * pMute){
		return 1;
	}
	return 0;
}

void printDevInfo(pDevice_t* dev, int devNum, unsigned long period, char eventFlag, unsigned int upLinkTotal, double throughput){
	int ans;
	printf("period    ");
	printf("totalUsedClk ");
	printf("eventFlag ");
	printf("eventCnt ");
	printf("totalEvent ");
	printf("devID ");
	printf("sending ");
	printf("upLinkTotal ");
	printf("totalSendBit ");
	printf("totalBlockBit ");
	printf("sendBitRemain ");
	printf("rollbackNum ");
	printf("collisionCnt ");
	printf("muteRecord ");
	printf("muteFlag ");
	printf("clkWaitToSend ");
	printf("throughput\n");
	for(ans = 0; ans < devNum; ans++){
		printf("%-10lu", period);
		printf("%-13lu", totalUsedClk);
		printf("%-10d ", dev[ans]->eventFlag);
		printf("%-8d ", dev[ans]->eventCnt);
		printf("%-10lu ", totalEvent);
		printf("%-5d ", ans);
		printf("%-7d ", dev[ans]->sending);
		printf("%-11u ", upLinkTotal);
		printf("%-12lu ", totalSendBit);
		printf("%-13lu ", totalBlockBit);
		printf("%-13lu ", dev[ans]->sendBitRemain);
		printf("%-11lu ", dev[ans]->rollbackNum);
		printf("%-12d ", dev[ans]->collisionCnt);
		printf("%-10d ", dev[ans]->muteRecord);
		printf("%-8d ", dev[ans]->muteFlag);
		printf("%-13u ", dev[ans]->clkWaitToSend);
		printf("%-2.5f\n", throughput);
	}
}
#define DEBUG 0
void reset(void){
	totalClk = 0;
	upLinkTotal = 0;				// 当前正在占用上行信道的设备数
	totalSendBit = 0;
	throughput = 0;
	totalEvent = 0;
	totalBlockBit = 0;
	totalUsedClk = 0;
	totalTimeDelay = 0;
	printEntCnt = 0;
	regCnt = 0;
	devNum = initialDevNum;
	firstSendOutTime = 0;
	totalCollision = 0;
}

int main(int argc, char **argv){
	int ans;
	
	char eventFlag;
	unsigned long i = 0;
	unsigned long j = 0;
	
	
	srand(time(NULL));
	//printf("在这个编程环境中能产生0~%d的随机数.\n",       RAND_MAX);
	initParameter();
	
	printf("initialDevNum, ");
	printf("Pmute, ");
	printf("P, ");
	printf("K, ");
	printf("regAtStart, ");
	printf("S,");
	printf("Sn,");
	printf("rate,");
	printf("throughput, ");
	printf("perEntBitNum, ");
	printf("totalEvent, ");
	printf("totalTimeDelay, ");
	printf("aveDelay, ");
	printf("totalBlockBit, ");
	printf("firstSendOutTime, ");
	printf("totalCollision, ");
	printf("runTime\n");
	
	devNum = devNumMin, Pmute = pMuteMin, BIT_ADD = bitAddMin, K =kMin, P = pMin;
	while(initialDevNum <= devNumMax && ((int)(Pmute * 100)) <= ((int)(pMuteMax * 100)) && BIT_ADD <= bitAddMax && K <= kMax && P <= pMax){
#if(DEBUG)
		printf("initialDevNum: %d\n", initialDevNum);
		printf("Pmute: %.2f\n", Pmute);
		printf("P: %u\n", P);
		printf("BIT_ADD: %d\n", BIT_ADD);
		printf("K: %u\n", K);
#endif

		reset();

		
		for(ans = 0; ans < initialDevNum; ans++){
			dev[ans] = (Device_t *)malloc(sizeof(Device_t));
			if(dev[ans] == NULL){
				printf("malloc error!\n");
				return -1;
			}
			//printf("malloc %d ok.\n", ans);
			memset(dev[ans], 0, sizeof(Device_t));
			dev[ans]->clkWaitToSend = 0;
			dev[ans]->sendBitRemain = REG_AT_START ? BIT_ADD : 0;
			dev[ans]->eventCnt = REG_AT_START ? 1 : 0;
			dev[ans]->collisionCnt = 1;
		}
		totalEvent = REG_AT_START ? initialDevNum : 0;

	//printf("entID,entRspnTime,entHappenTime,entDueTime\n");
	//printf("period,totalBlockBit\n");

		while(1){
			/* 以一定概率产生事件 */
			for(ans = 0; ans < devNum; ans++){
				if(totalClk && (dev[ans]->eventFlag = isEventCreate(P))){			// 事件产生, 增加要发送的字节数
					dev[ans]->sendBitRemain += BIT_ADD;								// 每次事件产生固定的比特数
					dev[ans]->eventCnt++;
					totalEvent++;
					if(dev[ans]->eventCnt > MAX_EVENT_NUM){
						printf("事件数量超过容量!\n");
						getchar();
						return -1;
					}
					dev[ans]->eventTime[(dev[ans]->eventCnt) - 1] = totalClk;
					//printf("\n设备%d发生了事件, 导致剩余比特为: %lu\n", ans, dev[ans]->sendBitRemain);
					//getchar();
				}
			}

			// 在一定时间后加入新设备
			if(S && totalClk == S * R * 1000){
				for(ans = 0; ans < Sn; ans++){
					dev[devNum + ans] = (Device_t *)malloc(sizeof(Device_t));
					if(dev[devNum + ans] == NULL){
						printf("malloc error!\n");
						return -1;
					}
					memset(dev[devNum + ans], 0, sizeof(Device_t));
					dev[devNum + ans]->eventCnt = 1;
					totalEvent++;
					dev[devNum + ans]->sendBitRemain = BIT_ADD;
					dev[devNum + ans]->eventTime[dev[devNum + ans]->eventCnt - 1] = totalClk;
					dev[devNum + ans]->collisionCnt = 1;
					//printf("加入新设备 %d ok.\n", devNum + ans);
				}
				devNum += Sn;
				//getchar();
			}

			for(ans = 0; ans < devNum; ans++){
				if(dev[ans]->sending && upLinkTotal >= 2){				// 和其它设备产生碰撞(collision)
					dev[ans]->sending = 0;
					dev[ans]->sendBitRemain += dev[ans]->rollbackNum;
					totalSendBit -= dev[ans]->rollbackNum;
					dev[ans]->rollbackNum = 0;
					dev[ans]->clkWaitToSend += (1 + random() % (K * dev[ans]->collisionCnt));	// 随机增加等待的时间
					dev[ans]->collisionCnt *= 2;
					if(dev[ans]->collisionCnt > 16)
						dev[ans]->collisionCnt = 16;
					totalCollision++;
					//printf("\n设备%d发生了碰撞, 导致剩余时钟为: %u\n", ans, dev[ans]->clkWaitToSend);
				}
				else if(dev[ans]->sendBitRemain && !dev[ans]->clkWaitToSend){		// 发送时间到
					if(!dev[ans]->muteFlag){
						dev[ans]->sendBitRemain--;
						totalSendBit++;
						dev[ans]->rollbackNum++;
						dev[ans]->sending = 1;
					}
				}
				else if(!dev[ans]->sendBitRemain){									// 没有要发送的数据了
					// 在这里统计响应时间
					
					if(dev[ans]->rollbackNum){
						//printf("设备%d共发生了%d次事件\n", ans, dev[ans]->eventCnt);
						for(i = totalClk - dev[ans]->rollbackNum, j = 0; i < totalClk; i += BIT_ADD, j++){
							dev[ans]->eventRspnTime[j] = i - dev[ans]->eventTime[j];
							//printf("设备%d第%lu次事件时间为%lu\n", ans, j + 1, dev[ans]->eventTime[j]);
							//printf("设备%d的%lu号事件的响应时间为%lu\n", ans, j, dev[ans]->eventRspnTime[j]);
							totalTimeDelay += dev[ans]->eventRspnTime[j];
							//printf("%lu,%lu, %lu, %lu\n", printEntCnt++, dev[ans]->eventRspnTime[j], dev[ans]->eventTime[j], i);
							//getchar();
						}
					}
					
					
					dev[ans]->rollbackNum = 0;
					dev[ans]->sending = 0;
					dev[ans]->eventCnt = 0;
					dev[ans]->collisionCnt = 1;						// 碰撞回合结束了
				}
			}

			for(ans = 0; ans < devNum; ans++){
				if(!dev[ans]->sending && upLinkTotal >= 1 && dev[ans]->muteRecord){			// 有其它设备占用上行信道, 进入静默模式(mute)
					dev[ans]->muteFlag = 1;													// 有一定的概率进入静默模式
					dev[ans]->clkWaitToSend = dev[ans]->clkWaitToSend;
				}
				else{
					dev[ans]->muteRecord = isMute(Pmute);
					dev[ans]->muteFlag = 0;
					if(dev[ans]->clkWaitToSend)
						dev[ans]->clkWaitToSend--;
				}
			}

			// 统计上行传输数量, 为下一周期判定碰撞做准备
			for(upLinkTotal = 0, ans = 0; ans < devNum; ans++){
				upLinkTotal += dev[ans]->sending;
			}

			// 吞吐率
			throughput = totalSendBit / ((double)totalClk + 1);

			totalBlockBitPre = totalBlockBit;
			for(ans = 0, totalBlockBit = 0; ans < devNum; ans++){
				totalBlockBit += dev[ans]->sendBitRemain;
			}

			totalUsedClk = (totalBlockBit) ? (totalUsedClk + 1) : totalUsedClk;		// 有用时钟数

			if(totalBlockBitPre && totalBlockBit == 0 &&  firstSendOutTime == 0 && totalClk != 0){
				firstSendOutTime = totalClk;
				//printf("firstSendOutTime = %lu\n", firstSendOutTime);
			}
			//getchar();
			//printDevInfo(dev, devNum, totalClk, eventFlag, upLinkTotal, throughput);

			totalClk++;
			if(totalClk == RUN_TIME){

				aveDelay = (totalEvent > 0) ? totalTimeDelay / (double)totalEvent : 0;
				printf("%d, ", initialDevNum);
				printf("%.2f, ", Pmute);
				printf("%u, ", P);
				printf("%d, ", K);
				printf("%d, ", REG_AT_START);
				printf("%d, ", S);
				printf("%d, ", Sn);
				printf("%d, ", R);
				printf("%2.5f, ", throughput);
				printf("%lu, ", BIT_ADD);
				printf("%lu, ", totalEvent);
				printf("%lu, ", totalTimeDelay);
				printf("%5.2f, ", aveDelay);
				printf("%lu, ", totalBlockBit);
				printf("%lu, ", firstSendOutTime);
				printf("%lu, ", totalCollision);
				printf("%lu\n", RUN_TIME);
				#if(DEBUG)
				printf("\n");
				#endif
				//printf("\n#############\n");
				for(ans = 0; ans < devNum; ans++){
					free(dev[ans]);
					//printf("free %d ok.\n", ans);
				}
				//printf("\n*************\n");
				break;
			}
		}


		initialDevNum   = (varParaMask == 1) ? initialDevNum + ceil(initialDevNum*0.05) : initialDevNum;
		Pmute    = (varParaMask == 2) ? Pmute + 0.01 : Pmute;
		BIT_ADD  = (varParaMask == 4) ? BIT_ADD + 8 : BIT_ADD;
		K        = (varParaMask == 8) ? K + 1 : K;
		//P        = (varParaMask == 16) ? P + Pstep : P;
		P        = (varParaMask == 16) ? P * 1.05: P;
		
	}
	return 0;
}




/*
有A,B,C,D四个设备
初始条件: 当前时刻为0(第0周期的开端), A剩余3个周期开始传送数据, B剩余4个周期开始传送数据, C剩余9个周期开始传送数据, D剩余
          11个周期开始传送数据.
模拟运行:
		  在第3周期, A开始传送第1bit数据.
		  在第4周期, B开始传动数据, 与此同时, 下行传送mute脉冲.
		  在第5周期, C和D接收到mute脉冲, 倒数计数暂停, C还剩4个周期, D还剩6个周期.
		             收发机探测到多于一个设备在传送数据, 发回collision脉冲.
		  在第6周期, A和B接收cllision脉冲, 停止发送, 并生成碰撞等待随机数, A回滚3个比特数据量, B回滚2个比特数据量.
问题: 在第6周期, C和D是否开始重新计数?
*/

























