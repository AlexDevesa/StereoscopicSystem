#include <SoftwareSerial.h>
#include <Servo.h>

SoftwareSerial BT(10,11);
Servo YAW,PITCH;
char buffer1[10];
int yaw=0,pitch=0,pb=0,mul=1,cadena[10],yant=0,pant=0;
byte dolar,slash, pos=0;
boolean dato=false;

void setup() {
  BT.begin(9600);
  Serial.begin(9600);
  YAW.attach(5);
  PITCH.attach(6);
  YAW.write(90);
  PITCH.write(90);
}

void loop() {
  //if(Serial.available()){
  if(BT.available()){
    memset(buffer1,0,sizeof(buffer1));
    //while(Serial.available()>0){
    while(BT.available()>0){
      delay(5);
      //char c=Serial.read();
      char c=BT.read();
      cadena[pos]=c;
      buffer1[pos]=c;
      pos++;
    }
    pos=0;
    if(buffer1[0]==':'){
      for(int i=1;i<11;i++){
        if(buffer1[i]=='$'){dolar=i;}
        if(buffer1[i]=='/'){slash=i;}
      }
       dato=true; 
    }
  }
  if(dato==true){
   for(int i=dolar-1;i>0;i--){
          pb=cadena[i]-48;
          yaw=yaw+pb*mul;
          mul=mul*10;
         // Serial.println(yaw);
        }
    mul=1;
    for(int i=slash-1;i>dolar;i--){
          pb=cadena[i]-48;
          pitch=pitch+pb*mul;
          mul=mul*10;
          //Serial.println(pitch);
    }
    mul=1;
    dato=false;
    dolar=0;
    slash=0;
    YAW.write(yaw);
    PITCH.write(pitch);
    //Serial.println(yaw);
    //Serial.println(pitch);
    yant=yaw;
    pant=pitch;
    yaw=0;
    pitch=0;
  }

}
