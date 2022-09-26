#include <emscripten.h>
#include <stdio.h>

// // Wrapper for our JavaScript function
// extern int isConnected();
// extern int isDisonnected();
// extern char* consoleLog(char* data);
// extern int sendMessage(int* result);
// extern void *allocateMemory(unsigned bytes_required);

int main(){return 0;}

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

EMSCRIPTEN_KEEPALIVE
int * getRandom() {

   static int  r[10];
   for (int i = 0; i < 10; ++i) {
      r[i] = i;
   }
   return r;
}

EMSCRIPTEN_KEEPALIVE
int sumArrayInt32 (int *array, int length) {
    int total = 0;

    for (int i = 0; i < length; ++i) {
        total += array[i];
    }

    return total;
}

EMSCRIPTEN_KEEPALIVE
void addArraysInt32 (int *array1, int* array2, int* result, int length)
{
    
    // sendMessage(result);
    result[0]=0x08;
    result[1]=0x01;
    result[2]=0xFE;
    result[3]=0x02;
    result[4]=0x00;
    result[5]=0x00;
    result[6]=0x04;
    result[7]=0x00;
    result[8]=0x00;

    // sendMessage();
}

EMSCRIPTEN_KEEPALIVE
__attribute__((used)) int* addArrays (int *array1, int* array2,int* result,int length)
{
//   int* result = allocateMemory(length * sizeof(int));

  for (int i = 0; i < length; ++i) {
    result[i] = array1[i] + array2[i];
  }

  return result;
}