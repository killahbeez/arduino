struct hex_colors {
  uint8_t R;
  uint8_t G;
  uint8_t B;
};


struct hex_colors didi(char* RGB){
  struct hex_colors RGB_colors;
  
  char buffer[2];
  sprintf(buffer,"%c%c",RGB[0],RGB[1]);
  RGB_colors.R = (uint8_t)strtol(buffer, NULL, 16);
  sprintf(buffer,"%c%c",RGB[2],RGB[3]);
  RGB_colors.G = (uint8_t)strtol(buffer, NULL, 16);
  sprintf(buffer,"%c%c",RGB[4],RGB[5]);
  RGB_colors.B = (uint8_t)strtol(buffer, NULL, 16);

  return RGB_colors;
}        


void setup() {
  Serial.begin(115200);
  char* RGB = "FF0000";
  

  struct hex_colors RGB_colors = didi("FF55ff");
  Serial.println(RGB_colors.R);
  Serial.println(RGB_colors.G);
  Serial.println(RGB_colors.B);
  
}

  
void loop() {
  // put your main code here, to run repeatedly:

}
