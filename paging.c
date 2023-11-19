#include<stdio.h>
#include<stdlib.h>
#include<stdbool.h>
#include "simos.h"

typedef struct {
	int pid;
	int page;
	char age;
	bool dirty;
	bool free;
}metaData;

typedef struct{
	int fno;
	struct freeList *next;
}freeList;

metaData **mframe;
freeList *fhead, *ftail;

int **pageTable,
void init_process_pagetable (int pid){
	int page;
	int nop =PCB[pid]->NOP;
    pageTable = (int*)calloc(nop,sizeof(int));


    PCB[pid]->PTptr= pageTable;
    for (page=0;page<nop;page++){
        pageTable[page]=-1;
    }
}


void update_process_pagetable (int pid, int page, int frame){
	PCB[pid] -> PTptr[page] = frame;
}


void initialize_mframe_manager (){
	int mf;

	mframe = (metaData**) calloc(numFrames,sizeof(metaData*)); 

	for(mf=2;mf<numFrames;mf++){
		mframe[mf]= (metaData*)calloc(1,sizeof(metaData));  //updating frame metadata 
		mframe[mf]->pid=-1;
		mframe[mf]->page=-1;
		mframe[mf]->dirty=0;
		mframe[mf]->free=1;
	}

	fhead=(freeList*)calloc(1,sizeof(freeList)); 
	fhead->fno=2;
	fhead->next=NULL;
	ftail=fhead;

	for(mf=3;mf<numFrames;mf++){//adding to free list
		freeList *fnode;	
	    fnode=(freeList*)calloc(1,sizeof(freeList));  
	    fnode->fno=mf;
	    fnode->next=NULL;
	    ftail->next=fnode;
	    ftail=fnode;    
	}

}

int calculate_memory_address (unsigned offset, int rwflag) // memory.c
{
	int frame,PMA;

	frame = *((PCB[CPU.Pid]->PTptr)+(offset/pageSize));
	PMA=frame*pageSize + offset % pageSize ;           // frame*PS + pageOffset % PS

	
	if(rwflag == 1 && (PCB[CPU.Pid]->progLen < offset)){    //read is set for unused Page
		return mError;
	}

	if(frame == -1){                                    
		set_interrupt(pFaultException);
		return mPFault;                           //page fault
	}
	else{
		if(rwflag == 2){
			mframe[frame]->dirty=1;
        }	
	}
	set_Left_AgeBit(frame);
	return PMA;  // return Physical Memory Address
}


void initialize_agescan (){
	int frame;
	for(frame=2;frame<numFrames;frame++){
		mframe[frame]->age=0;
	}         
	
}

void memory_agescan (){

	int mf;
      
  for(mf=2;mf<numFrames;mf++){
		mframe[mf]->age>>=1;
	}
		// if( (int*)(mframe[mf]->age)==0){
		// 	addto_free_frame(mf);

		// 	if(mframe[mf]->dirty==1){
  //              unsigned *buf= Memory+(mf*pageSize);
		// 	   write_swap_page (mframe->pid, mframe->page,buf)
		// 	}
	 //    }
}
void set_Left_AgeBit(int frameNo){ //need to add to simOS.h
	mframe[frameNo] -> age = ((mframe[frameNo] -> age) | (1<<7)) ;
}
void addto_free_frame(int fno){
	
	freeList *fnode;
	fnode=(freeList*)calloc(1,sizeof(freeList));
	fnode->fno=fno;
	fnode->next=NULL;
	ftail->next=fnode;
	ftail=fnode;
}



int get_free_frame(){
	int freeFrame;
	if(fhead == NULL){
		return -1;
    }
    
	freeFrame=fhead->fno;
	fhead=fhead->next;
	return freeFrame;
}


void free_process_memory (int pid){
    int mf;
    for(mf=2;mf<numFrames;mf++){
		if(mframe[mf]->pid==pid){
          mframe[mf]->pid=-1;
          mframe[mf]->page=-1;
          mframe[mf]->age=0;
          mframe[mf]->dirty=0;
          mframe[mf]->free=0;
          addto_free_frame(mf);
		}

	}
}


void swap_in_page (int pidin, int pagein, int finishact){
	unsigned *Buffer;  
	int luf, ff, mf, minage, minDirty; //luf = least used frame, ff=free frame, mf = mem frame
	bool ageZero = 0; //age zero flag to check any frames with age 0 to swap out if no free frames
	ff = get_free_frame();
	if(ff == -1){
		for(mf=2;mf<numFrames;mf++){ //checking age 0 frames
			if(mframe[mf]->age == 0){
				ageZero = 1;
				if(mframe[mf]-> dirty == 1){
					Buffer = Memory+(mf*pageSize); 
					insert_swapQ(mframe[mf]->pid, mframe[mf]->page, Buffer , actWrite, freeBuf);
				}
				PCB[mframe[mf]->pid] -> PTptr[mframe[mf]->page] = -1; //updating page table for freed frame with age = 0
				mframe[mf]->pid = -1; //updating frame metadata for freed frame with age = 0
				mframe[mf]->page = -1;
				mframe[mf] -> age = 0;
				mframe[mf] -> dirty = 0;
				mframe[mf] -> free = 1; 
				addto_free_frame(mf);
			}
		}
		if(ageZero == 0){ //if not found any frame with age equals zero
			minage = mframe[2] -> age; minDirty = mframe[2] -> dirty;
			for(mf=2;mf<numFrames;mf++){
				if(minage > mframe[mf]->age){
					minage = mframe[mf]->age;
					minDirty = mframe[mf]->dirty;
					luf = mf;
				}
				if(minage == mframe[mf]->age && mframe[mf]->dirty < minDirty){
					luf = mf;
					minDirty = mframe[mf]->dirty;
				}
			}
			if(minDirty == 1){
				Buffer = Memory+(luf*pageSize);
				insert_swapQ(mframe[luf]->pid, mframe[luf]->page, Buffer , actWrite, freeBuf);		
			}
			PCB[mframe[luf]->pid] -> PTptr[mframe[luf]->page] = -1; //updating page table for freed frame with not age 0
			mframe[luf]->pid = -1; //updating frame metadata for freed frame with not age  0
			mframe[luf]->page = -1;
			mframe[luf] -> age = 0;
			mframe[luf] -> dirty = 0;
			mframe[luf] -> free = 1; 
			addto_free_frame(luf);
		}
		ff = get_free_frame();
	}
	Buffer = Memory+(ff * pageSize);
	insert_swapQ(pidin, pagein, Buffer , actRead, finishact);
	PCB[pidin] -> PTptr[pagein] = ff; //updating page table
	mframe[ff]->pid = pidin; // updating frame info
	mframe[ff] -> page = pagein;
	mframe[ff] -> age = (1<<7);
	mframe[ff] -> dirty = 0;
	mframe[ff] -> free = 0; 
}






void page_fault_handler (){  
 	//identifying the page at fault
 	unsigned *buf;
	int pfault;
 	pfault = CPU.PC/pageSize;
 	// swap_in_page(CPU.Pid, pfault, toReady);
    buf=Memory+(pfault*pageSize);
 	insert_swapQ(CPU.Pid, pfault, buf , actRead, toReady);
}




void update_frame_info (int findex, int pid, int page){}


void dump_free_list (FILE *outf){
	int frame;
	fprintf(outf,"\t********** Memory Free Frame Dump **********\n");
	for(frame=0;frame<numFrames-1;frame++){
		if(mframe[frame]->free==0){
			fprintf(outf," %d,",frame);
		}
		fprintf(outf," %d.",frame);
	}
}

void dump_process_pagetable (FILE *outf, int pid){
	int page;
	for(page=0;page<PCB[pid]->NOP ;page++){
		fprintf("***Page/Frame:%d,%d",page,page<PCB[pid]->PTptr[page]);
	}
}
void dump_process_memory (FILE *outf, int pid){}

void dump_memory (FILE *outf){
	int frame, offset=0, fo;
	fprintf(outf,"\t********** Full Memory Dump **********\n");
 	for(frame=3; frame<=numFrames; frame++) {
 		fprintf(outf,"Frame %d :[%d,%d]:",frame,offset,offset+pageSize-1);
 		for(fo=0;fo<pageSize;fo++){
 			fprintf(outf," %x",Memory[offset++]);
 		}
 		fprintf(outf,"\n\n");
  	}
}
void dump_memoryframe_info (FILE *outf){}
