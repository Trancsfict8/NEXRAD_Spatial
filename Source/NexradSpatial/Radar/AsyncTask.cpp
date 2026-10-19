#include "AsyncTask.h"




#include <functional>




#include <thread>

void AsyncTaskRunner::Start(bool autoDeleteTask) {
	lock.lock();
	if(running){
		fprintf(stderr, "Error: tried to start AsyncTaskRunner multiple times\n");
		lock.unlock();
		return;
	}
	running = true;
	finished = false;
	canceled = false;
	this->autoDelete = autoDeleteTask;
	lock.unlock();

	std::thread thread([this] {
		InternalTask();
	});
	thread.detach();
}


void AsyncTaskRunner::Start() {
	AsyncTaskRunner::Start(false);
}

void AsyncTaskRunner::Cancel() {
	lock.lock();
	canceled = true;
	lock.unlock();
}

void AsyncTaskRunner::Delete() {
	Cancel();
	lock.lock();
	if(finished){
		clearedForDeletion = true;
		lock.unlock();
		delete this;
	}else{
		autoDelete = true;
		lock.unlock();
	}
}

void AsyncTaskRunner::InternalTask() {
	lock.lock();
	if(!canceled){
		lock.unlock();
		Task();
	}else{
		lock.unlock();
	}
	
	#ifdef UE_GAME
	#else
	if(0){
		// clean up futures
		std::vector<std::future<void>> stillPendingFutures;
		//fprintf(stderr, "stillPending before %i\n", (int)pendingFutures.size());
		for(auto& future : pendingFutures){
			if(future.wait_for(std::chrono::seconds(0)) != std::future_status::ready){
				stillPendingFutures.push_back(std::move(future));
			}
		}
		//fprintf(stderr, "stillPendingFutures %i\n", (int)stillPendingFutures.size());
		pendingFutures.clear();
		for(auto& future : stillPendingFutures){
			pendingFutures.push_back(std::move(future));
		}
		//fprintf(stderr, "stillPending after %i\n", (int)pendingFutures.size());
	}
	#endif
	
	
	
	lock.lock();
	if (autoDelete) {
		clearedForDeletion = true;
		lock.unlock();
		delete this;
	}else{
		finished = true;
		running = false;
		lock.unlock();
	}
}

AsyncTaskRunner::~AsyncTaskRunner(){
	if(!clearedForDeletion){
		fprintf(stderr, "WARNING!!!! AsyncTaskRunner was freed improperly, this will likely lead to a crash\n");
		fflush(stderr);
	}
}
