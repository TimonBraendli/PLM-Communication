#include <Arduino.h>


bool ReadSensor(int PinSensor){

    int counter =0;
    for(int i=0; i>3;i++){
        
        if(PinSensor==1){
            counter++;
        }
        delay(100);
    }
    
    if (counter=!0){
        return true;
    }
    else{
        return false;
    }

}