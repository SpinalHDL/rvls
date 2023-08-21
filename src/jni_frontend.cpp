#ifdef RVLS_JNI

#include <jni.h>
#include <stdint.h>
#include <string>
#include <iostream>
#include <queue>
#include <sstream>
#include "context.hpp"
#include "config.hpp"
#include "hart.hpp"

#ifdef __cplusplus
extern "C" {
#endif

jclass userDataClass;
jmethodID methodId;

#define rvlsJni(n) JNIEXPORT void JNICALL Java_rvls_jni_Frontend_##n(JNIEnv * env, jobject obj, long handle
#define rvlsJniBool(n) JNIEXPORT bool JNICALL Java_rvls_jni_Frontend_##n(JNIEnv * env, jobject obj, long handle

#define c ((Context*)handle)
#define rv c->harts[hartId]

string toString(JNIEnv *env, jstring jstr){
    const char * chars;
    chars = env->GetStringUTFChars(jstr, NULL ) ;
    string str = string(chars);
    env->ReleaseStringUTFChars(jstr, chars);
    return str;
}

JNIEXPORT jlong JNICALL Java_rvls_jni_Frontend_newContext(JNIEnv * env, jobject obj, jstring jworkspace){
	string workspace = toString(env, jworkspace);
	auto *ctx = new Context();
	ctx->spikeLogs = fopen((workspace + "/spike.log").c_str(), "w");
    return (jlong)ctx;
}

rvlsJni(deleteContext)){
    delete (Context*)handle;
}

rvlsJni(spikeDebug), jboolean enable){
    c->config.spikeDebug = enable;
}

rvlsJni(spikeLogCommit), jboolean enable){
	c->config.spikeLogCommit = enable;
}
rvlsJni(time), unsigned long value){
    c->time = value;
}


rvlsJni(newCpuMemoryView),int viewId, long readIds, long writeIds){
	c->cpuMemoryViewNew(viewId, readIds, writeIds);
}

rvlsJni(newCpu),int hartId, jstring isa, jstring priv, int physWidth, int memoryViewId){
	c->rvNew(hartId, toString(env, isa), toString(env, priv), physWidth, memoryViewId, c->spikeLogs);
}

rvlsJni(loadElf), long offset, jstring path){
	c->loadElf(toString(env, path), offset);
}

rvlsJni(loadBin), long offset, jstring path){
	c->loadBin(toString(env, path), offset);
}

rvlsJni(setPc), int hartId, long pc){
	rv->setPc(pc);
}
rvlsJni(writeRf), int hartId, int rfKind, int address, long data){
	rv->writeRf(rfKind, address, data);
}
rvlsJni(readRf), int hartId, int rfKind, int address, long data) {
	rv->readRf(rfKind, address, data);
}

rvlsJniBool(commit), int hartId, long pc) {
	try{
		rv->commit(pc);
	} catch (const std::exception &e) {
		printf("commit error\n");
		printf("- %s\n", e.what());
	    return false;
	}
	return true;
}
rvlsJniBool(trap), int hartId, jboolean interrupt, int code) {
	try{
		rv->trap(interrupt, code);
	} catch (const std::exception &e) {
		printf("commit error\n");
		printf("- %s\n", e.what());
	    return false;
	}
	return true;
}
rvlsJni(ioAccess), int hartId, jboolean write, long address, long data, int mask, int size, jboolean error){
	TraceIo a;
	a.write = write;
	a.address = address;
	a.data = data;
	a.mask = mask;
	a.size = size;
	a.error = error;
    rv->ioAccess(a);
}

rvlsJni(setInterrupt), int hartId, int intId, jboolean value){
    rv->setInt(intId, value);
}
rvlsJni(addRegion), int hartId, int kind, long base, long size){
	Region r;
	r.type = (RegionType)kind;
	r.base = base;
	r.size = size;
    rv->addRegion(r);
}
rvlsJni(loadExecute), int hartId, long id, long addr, long len, long data){
    rv->memory->loadExecute(id, addr, len, (u8*)&data);
}
rvlsJni(loadCommit), int hartId, long id){
    rv->memory->loadCommit( id);
}
rvlsJni(loadFlush), int hartId){
    rv->memory->loadFlush();
}
rvlsJni(storeCommit), int hartId, long id, long addr, long len, long data){
    rv->memory->storeCommit(id, addr, len, (u8*)&data);
}
rvlsJni(storeBroadcast), int hartId, long id){
    rv->memory->storeBroadcast(id);
}
rvlsJni(storeConditional), int hartId, jboolean failure){
    rv->scStatus(failure);
}


#ifdef __cplusplus
}
#endif

#endif

