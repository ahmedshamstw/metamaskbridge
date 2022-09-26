#include <emscripten.h>
#include <stdlib.h>
#include <stdio.h>

// // Wrapper for our JavaScript function
// extern char* consoleLog(char* data);
// extern int isDisonnected();
// extern char* consoleLog(char* data);
// extern unsigned int curTime();
// extern void logProgress(double progress);

int main() { return 0; }

EMSCRIPTEN_KEEPALIVE
int itWorkshhh(int x,int y)
{
    int* pstr_cntxt = malloc(10000);
    printf("hello world");
	if(NULL != pstr_cntxt)return 9;
    return x+y;
}

// EMSCRIPTEN_KEEPALIVE
// void test_module_notify_connect(){
// 	consoleLog("notified connected from javascript to C returned to javascript");
// }
// EMSCRIPTEN_KEEPALIVE
// void test_module_notify_disconnect(){
// 	consoleLog("notified disconnected from javascript to C returned to javascript");
// }
// EMSCRIPTEN_KEEPALIVE
// int test_module_connect()
// {
// 	if(!isDisonnected())
// 		return 2;
// 	return 0;
// }
// EMSCRIPTEN_KEEPALIVE
// int test_module_disconnect()
// {
// 	if(isDisonnected())
// 		return 1;
// 	return 0;
// }

// EMSCRIPTEN_KEEPALIVE
// int * getRandom() {

//    static int  r[10];
//    for (int i = 0; i < 10; ++i) {
//       r[i] = i;
//    }
//    return r;
// }

// EMSCRIPTEN_KEEPALIVE
// int sumArrayInt32 (int *array, int length) {
//     int total = 0;

//     for (int i = 0; i < length; ++i) {
//         total += array[i];
//     }

//     return total;
// }

// EMSCRIPTEN_KEEPALIVE
// void addArraysInt32 (int *array1, int* array2, int* result, int length)
// {
//     for (int i = 0; i < length; ++i) {
//         result[i] = array1[i] + array2[i];
//     }
// }