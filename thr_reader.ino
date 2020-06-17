
void playThr(String fileName) {
  if (!playingFile){
    Serial.print("Start playing new file: ");
    Serial.println(fileName);
    file = SPIFFS.open(fileName, "r");
    
    if (!file) {
      Serial.println("File doesn't exist.");
      
    }else{
      playingFile = true;
    }
    currentInstruction = 0;
  }
  
}

void playNextThrLine() {
    if (file.available() && playingFile) {
      //Lets read line by line from the file
      float deg = radiansToDegrees(file.readStringUntil(' ').toFloat());
      float rad = file.readStringUntil('\n').toFloat();
      Serial.print(" ============ nextInstruction ============ DEG: ");
      Serial.print(deg);
      Serial.print(" RAD: ");
      Serial.println(rad);
      nextInstruction(deg,rad);
//      delay(1000);
      
//      if (currentInstruction ==0 )nextInstruction(0.0,0.0);
//      if (currentInstruction ==1 )nextInstruction(4.0,0.2);
//      if (currentInstruction ==9 ){nextInstruction(2.0,0.0);currentInstruction = -1;}
//      currentInstruction++;
    }else{
      playingFile = false;
    }
}
