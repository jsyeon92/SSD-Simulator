#include "ssd_sim.h"

void page_tbl_init(int max);
void ssd_blk_allocate(int lpn);
int page_tbl_search(long  lpn);
int read_trace(FILE * trc_fp);
void free_blk_list_init();
int garbage_collection();
void print_result();
FILE* ssd_init(char* argv[]);

main(int argc, char* argv[])
{
	FILE *trc_fp;
	int re=0;
	int count=0;
	int i=0;
	
	trc_fp=ssd_init(argv);
	while(count <10000)
	{
		re=read_trace(trc_fp);///simulatoer 시작 
		fseek(trc_fp,0, SEEK_SET);
		if(re == FALSE)
		{
			printf("alloc fail");
			break;
		}
		count++;
		
	}
	print_result();
	free(ssd_blk);
	free(page_tbl);
	return;
}
FILE * ssd_init(char * argv[])
{
	int max;
	FILE *trc_fp;	
	int i;
	
	trc_fp = openReadFile(argv[1]);
	max=get_range(trc_fp, &ssd_page_size, &total_page);

	ssd_size=atoi(argv[2]);
	page_size=atoi(argv[3]);
	over_ratio=atoi(argv[4]);
	gc_size=atoi(argv[5]);
	
	blk_num = (ssd_size *1024 * 1024) / (page_size *b_in_page);
	blk_num1= blk_num;
	page_num = blk_num * b_in_page;
	block_size = page_size * b_in_page;

	dirty_block_limit=((blk_num)*(100-over_ratio)/100);//overprovision ratio = free_block_limit
	garbage_size = ((blk_num)*gc_size/100);
	overpro_size=garbage_size * b_in_page;	
	
	ssd_blk= (block_struct *)calloc((int)blk_num, sizeof(block_struct));//ssd 사이즈 만큼 블록 생성
	page_tbl = (ftl_struct *)calloc((int)max+1,sizeof(ftl_struct));//LBA중 가장큰값을 배열의 크기로 잡는다.
	page_tbl_init(max);
	free_blk_list_init();
	
	return trc_fp;
}
void free_blk_list_init()
{
	int i=0;
	int j=0;
		
	for(i=0; i< blk_num;i++) {
		for(j=0; j< 64; j++) {
			ssd_blk[i].page[j].write_count=0;
			ssd_blk[i].page[j].free =TRUE;
			ssd_blk[i].page[j].valid= FALSE;
			ssd_blk[i].page[j].num_of_page=j;
			ssd_blk[i].page[j].num_of_block =i;
			ssd_blk[i].page[j].page_tbl_num = -1;
		}
		ssd_blk[i].free = TRUE;
		ssd_blk[i].valid= TRUE;
		ssd_blk[i].erase_count=0;
		ssd_blk[i].write_page_count=0;
		ssd_blk[i].valid_page_count=0;
		ssd_blk[i].dirty_page_count=0;
		list_insert(i);		
	}
}

void page_tbl_init(int max)
{
	int i=0;

	for(i=0; i< max+1; i++) {
		page_tbl[i].write_count = 0;
		page_tbl[i].block_array_num =-1;
		page_tbl[i].page_array_num =-1;
	}
	dirty_block_num =0;
	dirty_page_num = 0;
}
int allocate_phy_page(int lpn)
{
	int i =0;
	list_node* free_blk_node; 
	long free_blk_num;
	int temp=-1;
	// free block management
	free_blk_node=free_head;
	free_blk_num=free_blk_node->num_of_block;
	if(ssd_blk[free_blk_num].write_page_count == 0)
	{
		for(i=0;i < b_in_page; i++ )
		{
			if(ssd_blk[free_blk_num].page[i].free == FALSE)
			{
				printf("no!!!\n");
				exit(0);
			}
		}
	}
	// allocate free physical page
	for(i=0;i < 64 ; i++) {
		if(ssd_blk[free_blk_num].page[i].free==TRUE) {	
			ssd_blk[free_blk_num].page[i].write_count++;
			ssd_blk[free_blk_num].page[i].valid = TRUE;
			ssd_blk[free_blk_num].page[i].free = FALSE;
			ssd_blk[free_blk_num].page[i].page_tbl_num = lpn;	
			///block level
			ssd_blk[free_blk_num].write_page_count++;
			ssd_blk[free_blk_num].valid_page_count++;
			temp = ssd_blk[free_blk_num].write_page_count;
			page_tbl[lpn].block_array_num = free_blk_num;
			page_tbl[lpn].page_array_num =i;
			page_tbl[lpn].write_count++;
			page_write_count++;
			if(temp == b_in_page){
				list_delete(free_blk_num);
				dirty_block_num++;
				temp =0;
			}
			return TRUE;
		}
	}	
	return FALSE;
}
int page_tbl_search(long lpn)
{
	long temp_blk_num;
	long temp_page_num;
	int re=0;
	
	if(page_tbl[lpn].write_count <= 0) {
		re=allocate_phy_page(lpn);
		return re;
	}
	else { //rewrite	
		temp_blk_num  = page_tbl[lpn].block_array_num;
		temp_page_num = page_tbl[lpn].page_array_num;
		if(ssd_blk[temp_blk_num].page[temp_page_num].valid ==FALSE || ssd_blk[temp_blk_num].page[temp_page_num].free==TRUE)
		{
			printf("Free alloc error\n");
			exit(0);
		}	
		ssd_blk[temp_blk_num].page[temp_page_num].valid=FALSE;
		ssd_blk[temp_blk_num].page[temp_page_num].free=FALSE;
		ssd_blk[temp_blk_num].page[temp_page_num].page_tbl_num=-1;
		
		ssd_blk[temp_blk_num].dirty_page_count++;
		ssd_blk[temp_blk_num].valid_page_count--;
	
		page_tbl[lpn].block_array_num = -1;	
		page_tbl[lpn].page_array_num  = -1;
		
		re=allocate_phy_page(lpn);

		return re;
	}
}
int read_trace(FILE * trc_fp)
{
	long lpn;
	long block_num_temp;
	int arr_num;

	while(!feof(trc_fp)) {
		fscanf(trc_fp, "%ld", &lpn);
		lpn >>= 3;
		if(page_tbl_search(lpn) == FALSE) {
			return FALSE;
		}
		while(dirty_block_num > dirty_block_limit)
		{
			if(garbage_collection() == FALSE)
			{
				break;
			}	
		}
	}
	return TRUE;
}
int garbage_collection()
{
	int i=0;
	int re=TRUE;
	int j=0;
	int overpro_num=0;
	int blk_temp=-1;
	int temp=-1;
	int count=0;
	overpro_tbl = (int *)calloc((int)overpro_size, sizeof(int));
	garbage_tbl = (garbage_struct *)calloc((int)blk_num, sizeof(garbage_struct));	
	//평등한 상태 
	for(j=0;j< blk_num1 ;j++)
	{
		garbage_tbl[j].num_of_block   = j;
		garbage_tbl[j].dirty_page_num = ssd_blk[j].dirty_page_count;
	}
	mergeSort(blk_num1, garbage_tbl);//?
	for(i=0;i< garbage_size;i++)//block clean
	{
		blk_temp = garbage_tbl[i].num_of_block;
		if(garbage_tbl[i].dirty_page_num ==0){//no Garbage block 
			break;
		}
		for(j=0; j< b_in_page;j++)//page cleani
		{	
			if(ssd_blk[blk_temp].page[j].valid ==TRUE){//valid page copy
				overpro_tbl[overpro_num]=ssd_blk[blk_temp].page[j].page_tbl_num;
				page_write_count++;
				overpro_num++;
			}
			ssd_blk[blk_temp].page[j].free  = TRUE;
			ssd_blk[blk_temp].page[j].valid = FALSE;
			ssd_blk[blk_temp].page[j].page_tbl_num=-2;
		}
		ssd_blk[blk_temp].free = TRUE;
		ssd_blk[blk_temp].erase_count++;//지운 숫자 증가 
		ssd_blk[blk_temp].write_page_count =0;
		ssd_blk[blk_temp].valid_page_count =0;
		ssd_blk[blk_temp].dirty_page_count =0;
	
		list_insert(blk_temp);//free block list에 추가하는 부분
		dirty_block_num--;
	}	

	for(j=0;j < overpro_num ; j++){//re allocation
		temp=overpro_tbl[j];
		re=allocate_phy_page(temp);
		if(re == FALSE){
			printf("clean remapping error\n");
			exit(0);
		}
	}
	free(overpro_tbl);		
	free(garbage_tbl);
#ifdef test
	printf("blk_num         %ld\n", blk_num);
	printf("dirty_blk_num   %ld\n", dirty_block_num);
	printf("dirty_blk_limit %ld\n", dirty_block_limit);
#endif
	if(ssd_blk[blk_temp].dirty_page_count ==0){
		return FALSE;
	}else return TRUE;
}
void print_result()
{
	int i=0;
	long gc_sum=0;
	printf("page_write_count : %ld\n", page_write_count);
	for(i=0; i<blk_num1; i++)
	{
		gc_sum +=ssd_blk[i].erase_count;
	}
	printf("Erase Count : %ld : %ld \n", gc_sum, blk_num1);
	for(i=0; i<blk_num1; i++)
	{
		printf("%d\n", ssd_blk[i].erase_count);
	}
}
