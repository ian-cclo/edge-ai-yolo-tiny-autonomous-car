#define ENABLE_DRIVEDC_BRAKE_DELAY
#include "MatrixMiniR4.h"

float err;
float err_old;
float U;

void setup()
{
  MiniR4.begin();
  MiniR4.PWR.setBattCell(2);
  Serial.begin(9600);
  MiniR4.I2C0.MXLineTracer.begin();
  MiniR4.OLED.setTextSize(3);
  MiniR4.OLED.setTextColor(SSD1306_WHITE);
  MiniR4.DriveDC.begin(1, 2, false, true);
}

void loop()
{
  MiniR4.OLED.clearDisplay();
  MiniR4.OLED.setCursor(10, 10);
  MiniR4.OLED.print(MiniR4.PWR.getBattVoltage());
  MiniR4.OLED.display();
  if(MiniR4.BTN_UP.getState())
  {
    while(!MiniR4.BTN_DOWN.getState())
    {
      err = MiniR4.I2C0.MXLineTracer.getError();
      U = (err * 30) + ((err - err_old) * 20);
      err_old = err;
      MiniR4.DriveDC.Move((50 + U), (50 - U));
    }
    MiniR4.DriveDC.brake(true);
  }
  else
  {
  }

}