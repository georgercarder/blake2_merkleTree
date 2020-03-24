#include <stdlib.h>
#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include <limits.h>
#include <pthread.h>
// -lpthread

#include <fstream>
#include <iostream>
#include <string>
#include <vector>

#include "hash_functions/blake2.h"

void generateTestData(std::vector<std::string> &filenames); // for test

long int Power(const long int base, const long int exp) {
	if (exp < 0) {
		printf("Error: exp must be non-neg\n");
		return 1;
	} 
	if (exp == 0) return 1;
	long int one = (long int)1;
	return base * Power(base, exp - one);
}

int GetNextPowerOf2(const long int num) {
	long int next = 0;
	long int base = (long int)2;
	for (long int i = 1; i < INT_MAX; ++i) {
		next = Power(base, i);		
		if (next >= num) {
			return next;	
		}
	}
	printf("Error: next power of 2 not found.");
	return next;
}

bool IsPowerOf2(long int num) {
	if (num < 0 || num % 2 == 1) {
		return false;
	}
	long int scratch;
	while (num != 1) {
		scratch = 2 * (num / 2);
		if (scratch != num) {
			return false;	
		}
		num = num / 2;	
	}
	return true;
}

int GenBlockLength(int height, long int dataLen) {
	long int numberOfLeaves = Power((long int)2, (long int)height);
	if (numberOfLeaves == -1) {
		printf("Error: error in exponentiating.\n");	
	}
	return dataLen / numberOfLeaves;
}

class Payload {
	private:
	uint8_t *payload = NULL;
	int payloadLen = -1;
	public:
	Payload() {}
	Payload(uint8_t *pl, int pLen) {
		payload = pl;
		payloadLen = pLen;	
	}
	Payload(Payload *a, Payload *b) {
		payloadLen = a->payloadLen + b->payloadLen;
		payload = (uint8_t*)malloc(payloadLen * sizeof(uint8_t));	
		memcpy(payload, a->payload, a->payloadLen);
		memcpy(payload + a->payloadLen, b->payload, b->payloadLen);
	}
	Payload Hash() {
  		int outputLen = 32;;
		uint8_t *output = (uint8_t*)malloc(outputLen * sizeof(uint8_t));
		blake2s(output, outputLen * sizeof(uint8_t), payload, payloadLen * sizeof(uint8_t), NULL, 0);
		Payload *pl = new(Payload);
		*pl = Payload(output, outputLen);
		return *pl;		
	}
	uint8_t* GetPayload() {
		uint8_t *ret = (uint8_t*)malloc(payloadLen*sizeof(payloadLen));
		memcpy(ret, payload, payloadLen);
		return ret;
	}
	int GetPayloadLen() {
		return payloadLen;
	}

};

class Merkle {
	private:
	uint8_t *payload = NULL; 
	int payloadLen = -1;
	
	Merkle *master = NULL; // master is the "root"
	std::vector<Merkle*> *leaves = NULL;

	Merkle *parent = NULL;
	Merkle *lhs = NULL;
	Merkle *rhs = NULL;
	int height = -1;
	int level = -1;

	public:
	Merkle() {
		/*
		leaves = new(std::vector<Merkle*>);*/
	}

	Merkle(const int hheight) {
		int llevel = 0;
		leaves = new(std::vector<Merkle*>);
		master = new(Merkle);
		master = this;
		parent = NULL; //
		height = hheight;
		level = llevel;
		if (height > 0) {
		  lhs = new(Merkle);
		  *lhs = Merkle(height - 1, level + 1, this, master);
		  rhs = new(Merkle);
	  	  *rhs = Merkle(height - 1, level + 1, this, master);
		}
	}

	Merkle(const int hheight, const int llevel, Merkle* prnt, Merkle* mstr) {
	  height = hheight;
	  level = llevel;
	  parent = new(Merkle);
	  parent = prnt;
	  master = new(Merkle);
	  master = mstr;
	  if (height > 0) {
	  	lhs = new(Merkle);
		*lhs = Merkle(height - 1, level + 1, this, master);
		rhs = new(Merkle);
	  	*rhs = Merkle(height - 1, level + 1, this, master);
	  }
	  if (height == 1) {
		  PassLeaf(lhs);
		  PassLeaf(rhs);
	  }
	}

	Merkle(const Merkle& other) {
		payload = other.payload;
		parent = other.parent;
		lhs = other.lhs;
		rhs = other.rhs;
		level = other.level;	
	}

	~Merkle() {
		// TODO MORE?
	}

	void SetData(uint8_t* data, long int dataLen) {
		if (parent == NULL) {
			if (!IsPowerOf2(dataLen)) {
				// pad the data
				long int powerOf2Length = GetNextPowerOf2(dataLen);
				uint8_t* padded = (uint8_t*)malloc(powerOf2Length * sizeof(uint8_t));
				memcpy(padded, data, dataLen);	
				data = padded;	
				dataLen = powerOf2Length;
			}
				
			int blockLength = GenBlockLength(height, dataLen);
			for (int i = 0; i < leaves->size(); ++i) {
				leaves->at(i)->SetData(data + i * blockLength, blockLength);
			}
		} else if (height == 0) { // is leaf
			// this means we are at a leaf
			payload = (uint8_t*)malloc(dataLen*sizeof(uint8_t));
			memcpy(payload, data, dataLen);
			payloadLen = dataLen;	
			return;
		}
	}

	void PassLeaf(Merkle *leaf) {
		if (parent != NULL) {
			parent->PassLeaf(leaf);	
		} else {
			SetLeaf(leaf);
		}	
	}

	void SetLeaf(Merkle *leaf) {
		if (parent == NULL) { 
			Merkle *ptr = new(Merkle);
			ptr = leaf;
			leaves->push_back(ptr);
		} else {
			printf("Error: SetLeaf is only called on master.\n");
		}
	}

	void CombineLeaves() {
			Payload concattedPayloads = Payload(lhs->GetPayload(), rhs->GetPayload());
			Payload hashedPayload = concattedPayloads.Hash();	
			payload = hashedPayload.GetPayload();
			payloadLen = hashedPayload.GetPayloadLen();
			delete(lhs);
			lhs = NULL;
			delete(rhs);
			rhs = NULL;
	}

	static void *Reduce(void* ptr) {
		Merkle *m = (Merkle*)ptr;
		if (m->lhs->lhs == NULL) { // we only check lhs since everything is done symmetrically
			m->CombineLeaves();
			void *death;	
			return death;
		}
		// spawn threads
		pthread_t ltId;
		int ltRet = pthread_create(&ltId, NULL, Reduce, (void*)m->lhs);
		if (ltRet != 0) {
			printf("Error: ltRet thread %d \n", ltRet);
		}
		pthread_t rtId;
		int rtRet = pthread_create(&rtId, NULL, Reduce, (void*)m->rhs);
		if (rtRet != 0) {
			printf("Error: rtRet thread %d \n", rtRet);
		}
		pthread_join(ltId, NULL);
		pthread_join(rtId, NULL);
		// wait
		m->height--;
		void *death;
		return death;
	}

	void ReduceRoot() {
		if (lhs->lhs == NULL) { // we only check lhs since everything is done symmetrically
			CombineLeaves();	
			return;
		}
		// spawn threads
		pthread_t ltId;
		int ltRet = pthread_create(&ltId, NULL, Reduce, (void*)lhs);
		if (ltRet != 0) {
			printf("Error: ltRet thread %d \n", ltRet);
		}
		pthread_t rtId;
		int rtRet = pthread_create(&rtId, NULL, Reduce, (void*)rhs);
		if (rtRet != 0) {
			printf("Error: rtRet thread %d \n", rtRet);
		}
		pthread_join(ltId, NULL);
		pthread_join(rtId, NULL);
		// wait
		
		height--;
	}

	int GetHeight() {
		return height;
	}

	Payload* GetPayload() {
		Payload *pl = new(Payload);
		*pl = Payload(payload, payloadLen);
		return pl;	
	}

	uint8_t* GetRoot() {
		Payload concattedPayloads = Payload(lhs->GetPayload(), rhs->GetPayload());
		Payload hashedPayload = concattedPayloads.Hash();	
		payload = hashedPayload.GetPayload();
		payloadLen = hashedPayload.GetPayloadLen();
		uint8_t* ret = (uint8_t*)malloc(payloadLen*sizeof(uint8_t));
		memcpy(ret, payload, payloadLen);	
		return ret;
	}



};

int main() {
	
	
	//int lengthOfData = 124875291;
	//uint8_t *data = (uint8_t*)malloc(lengthOfData * sizeof(uint8_t));
	
  	std::string testDataDir = "testData/";
	std::vector<std::string> filenames = {testDataDir + "_1MBfile.dat",
					      testDataDir + "_10MBfile.dat",
					      testDataDir + "_100MBfile.dat",
					      testDataDir + "_1GBfile.dat"/*,
					  testDataDir + "_10GBfile.dat"*/};
  	generateTestData(filenames); 

	for (int i = 0; i < filenames.size(); ++i) {
		
		Merkle *m = new(Merkle);
		int height = 2;
		*m = Merkle(height);
		
		std::cout << "\nBenchmark for " << filenames.at(i) << "\n";
		std::ifstream file(filenames.at(i));
		file.seekg(0, file.end);
		long int fileLength = file.tellg();
		file.seekg(0, file.beg);
		uint8_t *dataBuf = (uint8_t*)malloc(fileLength*sizeof(uint8_t));
		file.read((char*)dataBuf, fileLength);
	
		m->SetData(dataBuf, fileLength);
		
		float merkleHashStartTime = 1000 * (float)clock() / CLOCKS_PER_SEC;
		while (m->GetHeight() > 1) {
			m->ReduceRoot();
		}

		const char* root = (const char*)m->GetRoot();
		float merkleHashEndTime = 1000 * (float)clock() / CLOCKS_PER_SEC;
		float merkleHashElapsedTime = merkleHashEndTime - merkleHashStartTime;
		printf("Time to hash: %f milliseconds\n", merkleHashElapsedTime);
	}

	return 0;
}
