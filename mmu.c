#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#define BUFFER_SIZE 256
#define TLB_SIZE 16
#define MASK(x) ((x>> 8) & 0xFF)

int counter = 0, time[BUFFER_SIZE*BUFFER_SIZE], indx, value;
int freeframe=0, freeframes[BUFFER_SIZE];
long PHYSICAL_MEMORY_SIZE;
struct TLB TLB;
void getPageNumber(int logical, unsigned char* PageTable, char *MEMORY, int* OF, int* faults, int* TLBHITS, char* BACKING_STORE);
int ReadDisk(int num, char *MEMORY, int* OF, char* BACKING_STORE);
int findLRU();


struct TLB{
    int index;
    unsigned char page[TLB_SIZE],frame[TLB_SIZE];

};

int main(int argc, char* argv[]){
    int OF=0;

    PHYSICAL_MEMORY_SIZE = strtol(argv[1], NULL, 10); 

    unsigned char PageTable[PHYSICAL_MEMORY_SIZE];
    signed char MEMORY[PHYSICAL_MEMORY_SIZE][256];
    int faults=0, TLBHITS=0, cnt=0, LogicalAddress;

	memset(PageTable, 0, PHYSICAL_MEMORY_SIZE);	
	memset(TLB.page, -1, sizeof(TLB.page));
	memset(TLB.frame, -1, sizeof(TLB.frame));
    //*****************************************************//
    memset(freeframes,-1,sizeof(freeframes));
    memset(time, -1,sizeof(time));
    //*****************************************************//
    FILE *ptr = fopen(argv[3],"r"); 
    FILE *output;
    if(PHYSICAL_MEMORY_SIZE==128){
        output = fopen("output128.csv", "w+");
    }
    else{
        output = fopen("output256.csv", "w+");
    }
    
    if (ptr == NULL){
		printf("Could not open addresses.txt file\n");
		exit(0);
	}
    
    while (fscanf(ptr, "%d", &LogicalAddress)==1){
		cnt++;
        getPageNumber(LogicalAddress,PageTable, (char*)MEMORY, &OF, &faults, &TLBHITS, argv[2]); 
        fprintf(output,"%d,%d,%d\n",LogicalAddress,indx, value);
    	}
    float faultRate = (faults*1.0)/cnt, hitRate = (TLBHITS*1.0)/cnt;
    //printf("Page Faults Rate, %0.2f%%,\nTLB Hits Rate, %0.2f%%,\n",faultRate*100,hitRate*100);
    fprintf(output,"Page Faults Rate, %0.2f%%,\nTLB Hits Rate, %0.2f%%,",faultRate*100,hitRate*100);
	fclose(ptr);
    fclose(output);
    
    return 0;
}

int position(unsigned char* PageTable,unsigned char page){
    int i;
    for(i=0;i<PHYSICAL_MEMORY_SIZE;i++){
        if(PageTable[i]==page){return i;}
    }
    return -1;
}

void getPageNumber(int logical, unsigned char* PageTable, char *MEMORY, int* OF, int* faults, int* TLBHITS, char* BACKING_STORE){
    
    int frameno=0, NEW=0;
    bool HIT=false; 
    unsigned char page = MASK(logical), offset =(logical&0xFF);
    int i=0;
    bool flag=false;
    
    //check TLB page, HIT
    while(i<TLB_SIZE){
        if(TLB.page[i]==page){
            int pos = position(PageTable,page);
	    	time[pos] = counter;
            //printf("HIT page = %d\t",page);
            HIT=true;
            (*TLBHITS)++;
            frameno=pos;
            
            
        }
        i++;
    }
    //CHECK PageTable, then read from disk/LRU
    if(!HIT){//either the frame number is obtained from the page table, or a page fault occurs
        int frame = position(PageTable,page);
        if(frame!=-1){
            //(obtain frame number from the page table, no page fault.)
            frameno=frame; 
            //printf("FOUND IN PAGETABLE[%d] = %d\t",frameno,PageTable[frameno]);
            PageTable[frameno] = page;
            time[frameno] = counter;
            freeframes[frameno]=frameno;
        }
        else {//PAGE FAULT
                (*faults)++;                
                if(freeframe>PHYSICAL_MEMORY_SIZE-1){   //LRU
                    int pos = findLRU();
                    time[pos] = counter;
                    frameno=pos;
                    PageTable[pos]=page;
                    flag=true;
                    freeframes[frameno]=frameno;
                    //printf("MAX CAP,");
                    ReadDisk(page, MEMORY, &pos, BACKING_STORE);   
                    
                }
                if(!flag){  //Open new frame 
                    //printf("FAULT ");
                    NEW = ReadDisk(page, MEMORY, OF, BACKING_STORE);                  
                    freeframe=NEW;
                    frameno = freeframe;
                    freeframe++;
                    PageTable[frameno] = page;
                    time[frameno] = counter;
                    freeframes[frameno]=frameno;
                }        
        }
        //ADD TO TLB 
        TLB.page[TLB.index] = page;
        TLB.frame[TLB.index] =  frameno;
        TLB.index = (TLB.index +1)%TLB_SIZE;      
    }
    
    //printf("frameno = %d\t",frameno);
    indx = ((unsigned char)frameno*256)+offset;
    value =MEMORY[indx];//*(MEMORY+indx);
    //printf("PageTable[%d] = %d\toffset = %d\tlogical %d,\t Physical %d,\t value %d\n",frameno,PageTable[frameno],offset,logical,indx, value);
    counter++;
    //printf("%d,%d,%d\n",logical,indx, value);
    
}

int ReadDisk(int num, char *MEMORY, int* OF, char* BACKING_STORE){
    FILE *disk;
    disk = fopen(BACKING_STORE,"rb");
    if(disk == NULL){
        printf("Could not open BACKING_STORE.bin file\n");
        exit(0);
    }

    if(fseek(disk, num*256, SEEK_SET)!=0){
        printf("ERROR SEEKING");
    }
    if(fread(&MEMORY[*OF*256],sizeof(unsigned char),256,disk)==0)
        printf("ERROR READING\n");
  
    (*OF)++;
    //printf("returning %d ",*OF-1);
    return (*OF)-1;
    
}

int findLRU(){
    
	int i, minimum, pos=0; 
    for(int i=0;i<PHYSICAL_MEMORY_SIZE;i++){
        if(time[i]!=-1){
            minimum=time[i]; break;}
    }
    
	for(i = 0; i < PHYSICAL_MEMORY_SIZE; ++i){
       
		if(time[i] < minimum && time[i]!=0){
			minimum = time[i];
			pos = i;
		}
	}
	
	return pos;
}