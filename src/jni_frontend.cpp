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
#include "disasm.h"


static disassembler_t disasm32 = disassembler_t(32);
static disassembler_t disasm64 = disassembler_t(64);

#ifdef __cplusplus
extern "C" {
#endif

jclass userDataClass;
jmethodID methodId;

#define rvlsJni(n) JNIEXPORT void JNICALL Java_rvls_jni_Frontend_##n(JNIEnv * env, jobject obj, long handle
#define rvlsJniBool(n) JNIEXPORT bool JNICALL Java_rvls_jni_Frontend_##n(JNIEnv * env, jobject obj, long handle
#define rvlsJniString(n) JNIEXPORT jstring JNICALL Java_rvls_jni_Frontend_##n(JNIEnv * env, jobject obj, long handle

#define c ((Context*)handle)
#define rv c->harts[hartId]

string toString(JNIEnv *env, jstring jstr){
    const char * chars;
    chars = env->GetStringUTFChars(jstr, NULL ) ;
    string str = string(chars);
    env->ReleaseStringUTFChars(jstr, chars);
    return str;
}

JNIEXPORT jlong JNICALL Java_rvls_jni_Frontend_newDisassemble(JNIEnv * env, jobject obj, int xlen){
    return  (jlong) new disassembler_t(xlen);
}

JNIEXPORT jstring JNICALL Java_rvls_jni_Frontend_disassemble(JNIEnv * env, jobject obj, long handle, long instruction){
	std::string str = ((disassembler_t*)handle)->disassemble(instruction);
	jstring result = env->NewStringUTF(str.c_str());
    return result;
}

JNIEXPORT void JNICALL Java_rvls_jni_Frontend_deleteDisassemble(JNIEnv * env, jobject obj, long handle){
	delete (disassembler_t*)handle;
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
	for(auto hart : c->harts){
		hart->proc->debug = enable;
	}
}

rvlsJni(spikeLogCommit), jboolean enable){
	c->config.spikeLogCommit = enable;
	for(auto hart : c->harts){
		if(enable)  hart->proc->enable_log_commits();
		if(!enable)  hart->proc->disable_log_commits();
	}
}
rvlsJni(time), unsigned long value){
    c->time = value;
}


rvlsJni(newCpuMemoryView),int viewId, long readIds, long writeIds){
	c->cpuMemoryViewNew(viewId, readIds, writeIds);
}

rvlsJni(newCpu),int hartId, jstring isa, jstring priv, int physWidth, int pmpNum, int memoryViewId){
	c->rvNew(hartId, toString(env, isa), toString(env, priv), physWidth, pmpNum, memoryViewId, c->spikeLogs);
}

rvlsJni(loadElf), long offset, jstring path){
	c->loadElf(toString(env, path), offset);
}

rvlsJni(loadBin), long offset, jstring path){
	c->loadBin(toString(env, path), offset);
}

rvlsJni(loadBytes), long offset, jbyteArray array){
	jbyte* bufferPtr = env->GetByteArrayElements(array, NULL);
	jsize lengthOfArray = env->GetArrayLength(array);
	c->loadBytes(offset, lengthOfArray, (u8*)bufferPtr);
	env->ReleaseByteArrayElements(array, bufferPtr, 0);
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
		c->lastErrorMessage = e.what();
	    return false;
	}
	return true;
}
rvlsJniBool(trap), int hartId, jboolean interrupt, int code) {
	try{
		rv->trap(interrupt, code);
	} catch (const std::exception &e) {
		c->lastErrorMessage = e.what();
	    return false;
	}
	return true;
}

rvlsJniString(getLastErrorMessage)) {
	std::string str = c->lastErrorMessage;
	jstring result = env->NewStringUTF(str.c_str());
	return result;
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
rvlsJniBool(loadExecute), int hartId, long id, long addr, long len, long data){
	try{
        rv->memory->loadExecute(id, addr, len, (u8*)&data);
	} catch (const std::exception &e) {
		c->lastErrorMessage = e.what();
	    return false;
	}
	return true;
}
rvlsJniBool(loadCommit), int hartId, long id){
	try{
        rv->memory->loadCommit( id);
	} catch (const std::exception &e) {
		c->lastErrorMessage = e.what();
	    return false;
	}
	return true;
}
rvlsJniBool(loadFlush), int hartId){
	try{
        rv->memory->loadFlush();
	} catch (const std::exception &e) {
		c->lastErrorMessage = e.what();
	    return false;
	}
	return true;
}
rvlsJniBool(storeExecute), int hartId, long id, long addr, long len, long data){
	try{
        rv->memory->storeExecute(id, addr, len, (u8*)&data);
	} catch (const std::exception &e) {
		c->lastErrorMessage = e.what();
	    return false;
	}
	return true;
}
rvlsJniBool(storeCommit), int hartId, long id){
	try{
        rv->memory->storeCommit(id);
	} catch (const std::exception &e) {
		c->lastErrorMessage = e.what();
	    return false;
	}
	return true;
}
rvlsJniBool(storeBroadcast), int hartId, long id){
	try{
        rv->memory->storeBroadcast(id);
	} catch (const std::exception &e) {
		c->lastErrorMessage = e.what();
	    return false;
	}
	return true;
}
rvlsJniBool(storeConditional), int hartId, jboolean failure){
    try{
        rv->scStatus(failure);
    } catch (const std::exception &e) {
        c->lastErrorMessage = e.what();
        return false;
    }
    return true;
}


#ifdef __cplusplus
}
#endif

#endif

