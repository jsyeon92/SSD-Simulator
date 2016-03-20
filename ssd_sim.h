//SSD의 크기를 80GB로 잡고하면 모든 LBA를 배열의 인덱스 값으로 넣을수 있다.
//version 1.0

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <time.h>
#include <sys/time.h>
#include <unistd.h>

#define TRUE 1
#define FALSE 0

#define FREE 1
#define GARBAGE 0

//#define time_log
#define jsyeon
//GC
static long dirty_block_num;//dirt_block_number
static long free_blk_num;
static long dirty_page_num;
///SSD configuration
static long ssd_size;
static long page_size;
static long over_ratio;
static long gc_size;
static long dirty_block_limit;
static long garbage_size;
static long blk_num;
static long page_num;
static long block_size;
static long blk_num1;
static long overpro_size;
static long page_write_count;
//SSD Log time
#ifdef time_log
static time_t start,end;
static double gap;
#endif 
typedef struct page_struct{
	int write_count;
	int valid;//valid =1 invalid =0
	int free;//free =1 not free = 0

	int num_of_block;///획기적인것이 필요하다
	long num_of_page;///me too
	long page_tbl_num; 

}page_struct;

typedef struct block_struct{
	int free;//free =1 not free = 0
	int valid;//valid =1 invalid =0
	
	int erase_count; 
	int write_page_count;//page in block write count (current) 
	int valid_page_count;//page in block valid count 
	int dirty_page_count;
	int num_of_block;
	page_struct page[b_in_page];	

}block_struct;

typedef struct FTL_struct{
///	page_struct * ppa;//physical page address
	long block_array_num;
	long page_array_num;
	long write_count;
}ftl_struct;

typedef struct garbage_struct{
	long num_of_block;
	long dirty_page_num;
	int flags;
}garbage_struct;

typedef struct free_block_node{
	long num_of_block;
	int num_of_write;
	struct free_block_node* next;
}list_node;

//free block list
static list_node * free_head=NULL;
static list_node * garbage_head=NULL;
static list_node * free_list_tail=NULL;
//get range
static long total_page; ///total page line number
static long ssd_page_size;//
//FTL 
static ftl_struct * page_tbl=NULL;
static block_struct *ssd_blk=NULL;
static garbage_struct *garbage_tbl=NULL;
static int *overpro_tbl=NULL;

FILE *openReadFile(char file_name[])
{
	FILE *fp;
	//fp = fopen("/home/js/ETRI/ssdsim_sim/data.txt","r");
	fp = fopen(file_name, "r");
	
	if(!fp)
	{
		printf("can not find file %s. \n", file_name);
		exit;
	}	
	return fp;
}
long get_range(FILE *trc_fp, long *p_vm_size, long *p_trc_len)
{
	long ref_page, off_set, bytes;
	long count = 0;
	long min_count;
	long min, max;

	fseek(trc_fp, 0, SEEK_SET);//파일의 맨 처음으로 포인터를 가지고 옴

	fscanf(trc_fp, "%ld", &ref_page);
	max=min= ref_page;

	while(!feof(trc_fp)){
		if (ref_page < 0)
			return FALSE;
		count++;
		if (ref_page > max)
			max = ref_page;
		if (ref_page < min)
		{
			min = ref_page;
			min_count = count;
		}	
		fscanf(trc_fp, "%ld", &ref_page);
	}
	fseek(trc_fp, 0, SEEK_SET);
	*p_vm_size = max;
	*p_trc_len = count;

//	printf("max : %ld ,min : %ld,count : %ld\n",max, min,count);

	return max/8;
}
void ArrayCopy(garbage_struct src[], garbage_struct dest[], int size)
{
	int i;
	for(i=0; i<size;i++)
	{
		dest[i].num_of_block = src[i].num_of_block;
		dest[i].dirty_page_num = src[i].dirty_page_num;
	}
}
void merge(int h, int m, garbage_struct Ldiv[], garbage_struct Rdiv[], garbage_struct g_tbl[])
{
	int i=0;
	int j=0;
	int k=0;
#ifdef jsyeon
	int left_blk;
	int right_blk;
#endif 

	while(i < h && j < m){
		if(Ldiv[i].dirty_page_num > Rdiv[j].dirty_page_num){
			g_tbl[k].dirty_page_num = Ldiv[i].dirty_page_num;
			g_tbl[k].num_of_block = Ldiv[i].num_of_block;
			++i;
		}
#ifdef jsyeon
			else if(Ldiv[i].dirty_page_num == Rdiv[j].dirty_page_num){
			left_blk  = Ldiv[i].num_of_block;
			right_blk = Rdiv[j].num_of_block;

				if(ssd_blk[left_blk].erase_count > ssd_blk[right_blk].erase_count){
					g_tbl[k].dirty_page_num = Rdiv[j].dirty_page_num;
					g_tbl[k].num_of_block = Rdiv[j].num_of_block;
					++j;
				}else{
					g_tbl[k].dirty_page_num = Ldiv[i].dirty_page_num;
					g_tbl[k].num_of_block = Ldiv[i].num_of_block;
					++i;
				}
	
			}
#endif
		else {
			g_tbl[k].dirty_page_num = Rdiv[j].dirty_page_num;
			g_tbl[k].num_of_block = Rdiv[j].num_of_block;
			++j;
		}
	k++;
	}

	if (i==h)
		ArrayCopy(Rdiv+j, g_tbl+k,m-j);
	else
		ArrayCopy(Ldiv+i, g_tbl+k,h-i);
}

void mergeSort(int size, garbage_struct g_tbl[])
{
	int h = size/2;
	int m = size-h;

	int L_index, R_index, key_index;
	garbage_struct *Ldiv, *Rdiv;

	if(size>1)
	{
		Ldiv = (garbage_struct *)malloc(h * sizeof(garbage_struct));
		Rdiv = (garbage_struct *)malloc(m * sizeof(garbage_struct));

		ArrayCopy(g_tbl, Ldiv, h);
		ArrayCopy(g_tbl+h, Rdiv, m);

		mergeSort(h,Ldiv);
		mergeSort(m,Rdiv);
		merge(h,m,Ldiv,Rdiv, g_tbl);

		free(Ldiv);
		free(Rdiv);
	}
}
void list_insert(int block_num)
{
	list_node *temp=NULL;
	list_node *ptr=NULL;
	int i=0;

	temp = calloc(1, sizeof(list_node));
	temp->num_of_block = block_num;
	temp->num_of_write = 0;
	temp->next = NULL;
	for(i=0; i< b_in_page; i++)
	{
		if(ssd_blk[block_num].page[i].free==FALSE)
		{
			printf("list insert error");
		}
	}	
	if(free_head==NULL)	{
		free_head = temp;
		free_list_tail=temp;
	}	
	else {
		free_list_tail->next=temp;
		free_blk_num++;
		free_list_tail=temp;
	}		
	//	printf("free_block insert : %ld \n", (long)block_num);
}	
void list_delete(int block_num)//맨첫번재 리스트 삭제 
{
	list_node * ptr=NULL;
	list_node * temp=NULL;
	
	if(block_num == free_head->num_of_block)
	{	
		ptr=free_head;
		temp=free_head;
		ptr=ptr->next;
		temp->num_of_block=-1;
		temp->num_of_write=-1;
		free(temp);
		free_head = ptr;
	}else
	{
		printf("list delete fail\n");
	}
}

void print_config()
{
	printf("SSD size      		: %ld\n", ssd_size);
	printf("page_size	 	: %ld\n", page_size);
	printf("block_in_page 		: %d\n", b_in_page);
	printf("blk_num	         	: %ld\n", blk_num);
	printf("dirty_blcok_limit       : %ld\n", dirty_block_limit);
	printf("garbage_size            : %ld\n", garbage_size);
	printf("overProvision 		: %ld\n", over_ratio);
	printf("Logical block count     : %ld\n", total_page);
	printf("dirty_page_count        : %ld\n",  dirty_page_num );
	printf("dirty_block_num 	: %ld\n", dirty_block_num);
}
#ifdef jsyeon1
void list_insert(int block_num)
{
	list_node *temp=NULL;
	list_node *ptr=NULL;

	temp = calloc(1, sizeof(list_node));
	temp->num_of_block = block_num;
	temp->num_of_write = 0;
	temp->next = NULL;	

	if(free_head==NULL)	{
		free_head = temp;
	}	
	else {
		ptr=free_head;
		while(1) {
			if(ptr->next==NULL) {
				ptr->next = temp;
				free_blk_num++;
				break;
			}
			ptr= ptr->next;
		}
	}	
}
#endif
#ifdef jsyeon1
void merge(int low, int mid, int high)
{
	int i,j,k;

	for(i=low;i<=high; i++)
	{
		temp [i] = key[i];
	}
	i = low;
	j= mid+1;
	k=low;

	while()
}
void mergeSort(int low, int high)
{
	int mid;

	if(low < high){
		mid = (low + high)/2;
		mergeSort(low, mid);
		mergeSort(mid+1, high);
		merge(low,mid, high);
}
void quicksort(garbage_struct * a[], int lo, int hi)
{
	int i=lo, j=hi, h;
	int x=a[(lo+hi)/2].dirty_page_num;

	do{
		while(a[i].dirty_page_num<x)i++;
		while(a[j].dirty_page_num>x)j--;
		if(i<=j){
			h=a[i]; a[i]=a[j]; a[j]=h;
			i++;j--;
		}
	}while(i<=j);

	if(lo<j)quicksort(a,lo,j);
	if(i<hi)quicksort(a,i,hi);
}
#endif
