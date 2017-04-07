// lib

void Print2 (int n) {
  if (n < 10) Serial.print('0');
  Serial.print(n);
}


void printCharArray(char * cArray, int iSize) {

  
    for (int k= 0; k < (iSize); k++) {
Print2(cArray[k]); Serial.print(' '); }
Serial.println();

}
