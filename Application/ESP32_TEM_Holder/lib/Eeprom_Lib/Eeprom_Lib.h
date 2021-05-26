#include <stdio.h>
#include <stdint.h>
#include <EEPROM.h>

#define MIN_HOLDER_POSITION 0       // Min value that the holder can have
#define MAX_HOLDER_POSITION 400     // Max value that the holder can have

enum Flash_Position {Pos_1 = 1, Pos_2 = 3, Pos_3 = 5};

class Eeprom_Holder_Pos{
private:
  Flash_Position eeprom_pos;
public:
  void set_pos(uint16_t holder_position, Flash_Position flash_pos);
  uint16_t get_pos(void);
  Flash_Position get_eeprom_pos(void);
};

void Eeprom_Holder_Pos::set_pos(uint16_t holder_position, Flash_Position flash_pos){
  uint8_t tmp_1 = holder_position;
  uint8_t tmp_2 = (holder_position>>8);
  uint8_t flash_pos_1;
  uint8_t flash_pos_2;

  switch(flash_pos){
    case Pos_1:
      EEPROM.write(0, Pos_1);
      flash_pos_1 = Pos_1;
      flash_pos_2 = Pos_1+1;
      break;
    case Pos_2:
      EEPROM.write(0, Pos_2);
      flash_pos_1 = Pos_2;
      flash_pos_2 = Pos_2+1;
      break;
    case Pos_3:
      EEPROM.write(0, Pos_3);
      flash_pos_1 = Pos_3;
      flash_pos_2 = Pos_3+1;
      break;
    default:
      EEPROM.write(0, Pos_1);
      flash_pos_1 = Pos_1;
      flash_pos_2 = Pos_1+1;
  }

  EEPROM.commit();

  EEPROM.write(flash_pos_1, tmp_1);
  EEPROM.commit();
  EEPROM.write(flash_pos_2, tmp_2);
  EEPROM.commit();
}

uint16_t Eeprom_Holder_Pos::get_pos(void){
  Flash_Position last_writing_pos = (Flash_Position)EEPROM.read(0);
  uint16_t last_pos; 

  if(last_writing_pos != Pos_1 && last_writing_pos != Pos_2 && last_writing_pos != Pos_3){
    Serial.println("Error reading last writing position! Recovering holder position from saved values...");
    eeprom_pos = Pos_1; 
    last_pos = ((EEPROM.read(Pos_1+1)<<8) | EEPROM.read(Pos_1));
    if(last_pos <= MAX_HOLDER_POSITION && last_pos >= MIN_HOLDER_POSITION){
      return last_pos;
    } else {
      last_pos = ((EEPROM.read(Pos_2+1)<<8) | EEPROM.read(Pos_2));
      if (last_pos <= MAX_HOLDER_POSITION && last_pos >= MIN_HOLDER_POSITION){
        return last_pos;
      } else {
        last_pos = ((EEPROM.read(Pos_2+1)<<8) | EEPROM.read(Pos_2));
        if (last_pos <= MAX_HOLDER_POSITION && last_pos >= MIN_HOLDER_POSITION){
        return last_pos;
        } else {
            Serial.println("Couldn't recover last values! Calibration is required_1");
            //Serial.println(last_pos);
            return 0;
        }
      }
    }
  } else{
     last_pos = ((EEPROM.read(last_writing_pos+1)<<8) | EEPROM.read(last_writing_pos));
     eeprom_pos = last_writing_pos;
     if(last_pos <= MAX_HOLDER_POSITION && last_pos >= MIN_HOLDER_POSITION){
       return last_pos;
     } else {
       switch (last_writing_pos){
        case Pos_1:
          last_pos = ((EEPROM.read(Pos_3+1)<<8) | EEPROM.read(Pos_3));
          break;
        case Pos_2:
          last_pos = ((EEPROM.read(Pos_1+1)<<8) | EEPROM.read(Pos_1));
          break;
        case Pos_3:
          last_pos = ((EEPROM.read(Pos_2+1)<<8) | EEPROM.read(Pos_2));
          break;
       }
       if(last_pos <= MAX_HOLDER_POSITION && last_pos >= MIN_HOLDER_POSITION){
         return last_pos;
       } else {
          switch (last_writing_pos){
            case Pos_1:
              last_pos = ((EEPROM.read(Pos_2+1)<<8) | EEPROM.read(Pos_2));
              break;
            case Pos_2:
              last_pos = ((EEPROM.read(Pos_3+1)<<8) | EEPROM.read(Pos_3));
              break;
            case Pos_3:
              last_pos = ((EEPROM.read(Pos_1+1)<<8) | EEPROM.read(Pos_1));
              break;
          }
          if (last_pos <= MAX_HOLDER_POSITION && last_pos >= MIN_HOLDER_POSITION){
            return last_pos;
          } else {
            Serial.println("Couldn't recover last values! Calibration is required_2");
            return 0;
          }
       }
     }
  }
}

Flash_Position Eeprom_Holder_Pos::get_eeprom_pos(void){
  Flash_Position tmp =  eeprom_pos;
  switch (eeprom_pos){
    case Pos_1:
      eeprom_pos = Pos_2;
      break;
    case Pos_2:
      eeprom_pos = Pos_3;
      break;
    case Pos_3:
      eeprom_pos = Pos_1;
      break;
    default:
        eeprom_pos = Pos_2;
        break;
  }
  return tmp;
}
