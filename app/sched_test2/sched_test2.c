#include <hellfire.h>

void task(void){
	int32_t jobs, id;
	
	id = hf_selfid();
	for(;;){
		jobs = hf_jobs(id);
		printf("\n%s - id (%d) jobs [%d] dlm [%d]", hf_selfname(), id, hf_jobs(id), hf_dlm(id));
		while (jobs == hf_jobs(id));
	}
}

void ap_task(void){
	int32_t jobs, id;
	
	id = hf_selfid();
	
	jobs = hf_jobs(id);
	//delay_ms(1000);
	printf("\n-> Executando APERIODICA %s - (%d) jobs [%d] dlm [%d]", hf_selfname(), id, hf_jobs(id), hf_dlm(id));
	
	//hf_yield();
	while (jobs == hf_jobs(id));
}

void polling_server(void){
	
	int32_t jobs, id;
	
	id = hf_selfid();
	
	printf("\n POLLING SERVER - %s id (%d) jobs [%d] dlm [%d] cap [%d]", hf_selfname(), id, hf_jobs(id), hf_dlm(id),krnl_task->capacity);
	
	
	int32_t rc, qnt;
	volatile int32_t status;
	for(;;){	
		status = _di();// desabilita interrupcao
		krnl_task = &krnl_tcb[krnl_current_task];
		rc = setjmp(krnl_task->task_context);
		//printf("\n SALVANDO CONTEXTO.");
		
		if (rc){
			//printf("\n RETORNANDO CONTEXTO.");
			_ei(status); // habilita interrupcao
			return;
		}
		
		if (krnl_task->state == TASK_RUNNING)
			krnl_task->state = TASK_READY;
		if (krnl_task->pstack[0] != STACK_MAGIC)
			panic(PANIC_STACK_OVERFLOW);
		if (krnl_tasks > 0){
			if(hf_queue_count(krnl_pl_queue)){
					printf("\nFila AP = %d.", hf_queue_count(krnl_pl_queue));
					
					if(krnl_task->capacity == 0){
						hf_kill(hf_selfid());
					}else{
						krnl_task->capacity--;
					}
					
					krnl_task = hf_queue_remhead(krnl_pl_queue);
					krnl_current_task = krnl_task->id;
					//printf("\n Vai executar aperiodico - %s", hf_selfname());	
					krnl_task->state = TASK_RUNNING;
					//krnl_pcb.coop_cswitch++;  // pq tem isso aqui
					_restoreexec(krnl_task->task_context, krnl_task->state, krnl_current_task);
					panic(PANIC_UNKNOWN);
				
			}else{
				hf_yield();
			}
		}
	}
	

}


void create_ap(void)
{
	for(;;){
		delay_ms(200);
		hf_spawn(ap_task, 0, 1, 0, "task aperiódico", 2048);
		printf("\nqueue polling: %d (tick time %dus)\n", hf_queue_count(krnl_pl_queue), hf_ticktime());
		hf_yield();
	}
}

void app_main(void){
	hf_spawn(task, 10, 1, 10, "task a", 2048);
	hf_spawn(task, 10, 1, 10, "task b", 2048);
	hf_spawn(task, 10, 1, 10, "task c", 2048);
	hf_spawn(ap_task, 0, 1, 0, "task aperiódico a", 2048);
	hf_spawn(ap_task, 0, 1, 0, "task aperiódico b", 2048);
	hf_spawn(polling_server, 10, 5, 10, "task polling", 2048);
	hf_spawn(create_ap, 0, 0, 0, "task BE", 2048);
}
