#include <stdio.h>
// converts a file of all floats or ints from big-endian to little-endian
// or vice-versa

void main(){
  FILE* convertFrom = fopen("convertFrom","r");
  FILE* convertTo = fopen("convertTo","w");
  char buffer[4];
  char outBuffer[4];
  int success = 1;
  int howMany = 0;
  while (success>0){
    success = fread(buffer,1,4,convertFrom);
    outBuffer[3] = buffer[0];
    outBuffer[2] = buffer[1];
    outBuffer[1] = buffer[2];
    outBuffer[0] = buffer[3];
    fwrite(outBuffer,1,4,convertTo);
    howMany++;
  }
  fclose(convertFrom);
  fclose(convertTo);
}
