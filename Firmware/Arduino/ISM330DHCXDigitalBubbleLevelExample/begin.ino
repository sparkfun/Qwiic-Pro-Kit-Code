// Ping an I2C address and see if it responds
bool isConnected(uint8_t deviceAddress)
{
  Wire.beginTransmission(deviceAddress);
  if (Wire.endTransmission() == 0)
    return true;
  return false;
}

void beginDisplay()
{
  // Connect to address display is on by pinging addresses
  // 0x3D is default on Qwiic board
  if (isConnected(0x3D) == true || isConnected(0x3C) == true)
  {
    //Init and display splash
    oled.begin();     // Initialize the OLED
    oled.display();   // Display splash screen
    delay(1200);
    oled.clear(PAGE); // Clear the display's internal memory

    oled.setCursor(15, 7); //x, y
    oled.setFontType(0); //Set font to smallest
    oled.print(F("Bubble"));
    oled.setCursor(19, 20); //x, y
    oled.print(F("Level"));

    oled.display();
    delay(1200);
  }
}
