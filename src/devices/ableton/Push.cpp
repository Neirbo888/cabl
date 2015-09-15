/*
        ##########    Copyright (C) 2015 Vincenzo Pacella
        ##      ##    Distributed under MIT license, see file LICENSE
        ##      ##    or <http://opensource.org/licenses/MIT>
        ##      ##
##########      ############################################################# shaduzlabs.com #####*/

#include "devices/ableton/Push.h"
#include "comm/Driver.h"
#include "comm/Transfer.h"
#include "util/Functions.h"

#include <thread>

#include "gfx/LCDDisplay.h"
#include "gfx/displays/GDisplayDummy.h"

//!\todo delete debug includes
#include <iostream>
#include <iomanip>

//--------------------------------------------------------------------------------------------------

namespace
{
static const uint8_t kPush_epOut = 0x01;
static const uint8_t kPush_epInput = 0x81;
static const uint8_t kPush_manufacturerId = 0x47; // Akai manufacturer Id
}

//--------------------------------------------------------------------------------------------------

namespace sl
{
namespace kio
{
namespace devices
{

//--------------------------------------------------------------------------------------------------

enum class Push::Led : uint8_t
{
  F1,
  F2,
  F3,
  Control,
  Nav,
  BrowseLeft,
  BrowseRight,
  Main,
  Group, GroupR = Group, GroupG, GroupB,
  Browse,
  Sampling,
  NoteRepeat,
  Restart,
  TransportLeft,
  TransportRight,
  Grid,
  Play,
  Rec,
  Erase,
  Shift,
  Scene,
  Pattern,
  PadMode,
  View,
  Duplicate,
  Select,
  Solo,
  Mute,
  
  Pad13, Pad13R = Pad13, Pad13G, Pad13B,
  Pad14, Pad14R = Pad14, Pad14G, Pad14B,
  Pad15, Pad15R = Pad15, Pad15G, Pad15B,
  Pad16, Pad16R = Pad16, Pad16G, Pad16B,
  Pad9,  Pad9R  = Pad9,  Pad9G,  Pad9B,
  Pad10, Pad10R = Pad10, Pad10G, Pad10B,
  Pad11, Pad11R = Pad11, Pad11G, Pad11B,
  Pad12, Pad12R = Pad12, Pad12G, Pad12H,
  Pad5,  Pad5R  = Pad5,  Pad5G,  Pad5B,
  Pad6,  Pad6R  = Pad6,  Pad6G,  Pad6B,
  Pad7,  Pad7R  = Pad7,  Pad7G,  Pad7B,
  Pad8,  Pad8R  = Pad8,  Pad8G,  Pad8B,
  Pad1,  Pad1R  = Pad1,  Pad1G,  Pad1B,
  Pad2,  Pad2R  = Pad2,  Pad2G,  Pad2B,
  Pad3,  Pad3R  = Pad3,  Pad3G,  Pad3B,
  Pad4,  Pad4R  = Pad4,  Pad4G,  Pad4B,

  Unknown,
};

//--------------------------------------------------------------------------------------------------


enum class Push::Button : uint8_t
{
  Shift,
  Erase,
  Rec,
  Play,
  Grid,
  TransportRight,
  TransportLeft,
  Restart,

  MainEncoder = 11,
  NoteRepeat,
  Sampling,
  Browse,
  Group,

  Main,
  BrowseRight,
  BrowseLeft,
  Nav,
  Control,
  F3,
  F2,
  F1,

  Mute,
  Solo,
  Select,
  Duplicate,
  View,
  PadMode,
  Pattern,
  Scene,

  None,
};

//--------------------------------------------------------------------------------------------------

Push::Push(tPtr<DeviceHandle> pDeviceHandle_)
  : Device(std::move(pDeviceHandle_))
  , m_isDirtyLeds(false)
{
 m_buttons.resize(kPush_buttonsDataSize);
 m_leds.resize(kPush_ledsDataSize);
}

//--------------------------------------------------------------------------------------------------

Push::~Push()
{

}

//--------------------------------------------------------------------------------------------------

void Push::setLed(Device::Button btn_, const util::LedColor& color_)
{
  setLedImpl(getLed(btn_), color_);
}

//--------------------------------------------------------------------------------------------------

void Push::setLed(Device::Pad pad_, const util::LedColor& color_)
{
  setLedImpl(getLed(pad_), color_);
}

//--------------------------------------------------------------------------------------------------

void Push::sendMidiMsg(tRawData midiMsg_)
{
 //!\todo Use Push virtual midi port
}

//--------------------------------------------------------------------------------------------------

GDisplay* Push::getGraphicDisplay(uint8_t displayIndex_)
{
  static GDisplayDummy s_dummyDisplay;
  return &s_dummyDisplay;
}

//--------------------------------------------------------------------------------------------------

LCDDisplay* Push::getLCDDisplay(uint8_t displayIndex_)
{
  static LCDDisplay s_dummyLCDDisplay(0, 0);
  if (displayIndex_ > kPush_nDisplays)
  {
    return &s_dummyLCDDisplay;
  }

  return &m_displays[displayIndex_];
}

//--------------------------------------------------------------------------------------------------

bool Push::tick()
{
  static int state = 0;
  bool success = false;

  //!\todo enable once display dirty flag is properly set
  if (state == 0 && (
       m_displays[0].isDirty() ||
       m_displays[1].isDirty() ||
       m_displays[2].isDirty() ||
       m_displays[3].isDirty()
      )
   )
  {
    success = sendDisplayData();
  }

  else if (state == 1)
  {
    success = sendLeds();
  }
  else if (state == 2)
  {
    success = read();
  }

  if (++state >= 3)
  {
    state = 0;
  }

  return success;
}

//--------------------------------------------------------------------------------------------------

void Push::init()
{
  // Display
  initDisplay();

  // Leds
  m_isDirtyLeds = true;
}

//--------------------------------------------------------------------------------------------------

void Push::initDisplay() const
{
  //!\todo set backlight
  return;
}

//--------------------------------------------------------------------------------------------------

bool Push::sendDisplayData()
{
  bool result = true;
  tRawData sysexHeader{kPush_manufacturerId, 0x7F, 0x15, 0x18, 0x00, 0x45, 0x00 };
  
  for(uint8_t row = 0; row < m_displays[0].getNumberOfRows(); row++)
  {
    sysexHeader[3] = 0x18 + row;
    uint8_t nCharsPerRow = m_displays[0].getNumberOfCharsPerRow();
    tRawData data(m_displays[0].getNumberOfCharsPerRow() * kPush_nDisplays);
    for(uint8_t i = 0; i < kPush_nDisplays; i++)
    {
      std::copy_n( m_displays[i].getData().data() + (row*nCharsPerRow),
                   nCharsPerRow,
                   &firstPacket[i*nCharsPerRow]
      );
    }
    result = sendSysex({sysexHeader,data});    
  }
  
  return result;
}
//--------------------------------------------------------------------------------------------------

bool Push::sendLeds()
{
//  if (m_isDirtyLeds)
  {
    if(!getDeviceHandle()->write(Transfer({0x80}, &m_leds[0], 78), kPush_epOut))
    {
      return false;
    }
    m_isDirtyLeds = false;
  }
  return true;
}

//--------------------------------------------------------------------------------------------------

bool Push::read()
{
  Transfer input;
  for (uint8_t n = 0; n < 32; n++)
  {
    if (!getDeviceHandle()->read(input, kPush_epInput))
    {
      return false;
    }
    else if (input && input[0] == 0x01)
    {
      processButtons(input);
      break;
    }
    else if (input && input[0] == 0x20 && n % 8 == 0) // Too many pad messages, need to skip some...
    {
      processPads(input);
    }
/*
        std::cout << std::setfill('0') << std::internal;

        for( int i = 0; i < input.getSize(); i++ )
        {
          std::cout << std::hex << std::setw(2) << (int)input[i] <<  std::dec << " " ;
        }

        std::cout << std::endl << std::endl;*/
  }
  return true;
}

//--------------------------------------------------------------------------------------------------

void Push::processButtons(const Transfer& input_)
{
  bool shiftPressed(isButtonPressed(input_, Button::Shift));
  Device::Button changedButton(Device::Button::Unknown);
  bool buttonPressed(false);

  for (int i = 0; i < kPush_buttonsDataSize - 1; i++) // Skip the last byte (encoder value)
  {
    for (int k = 0; k < 8; k++)
    {
      uint8_t btn = (i * 8) + k;
      Button currentButton(static_cast<Button>(btn));
      if (currentButton == Button::Shift)
      {
        continue;
      }
      buttonPressed = isButtonPressed(input_, currentButton);
      if (buttonPressed != m_buttonStates[btn])
      {
        m_buttonStates[btn] = buttonPressed;
        changedButton = getDeviceButton(currentButton);
        if (changedButton != Device::Button::Unknown)
        {
      //    std::copy(&input_[1],&input_[kPush_buttonsDataSize],m_buttons.begin());
          buttonChanged(changedButton, buttonPressed, shiftPressed);
        }
      }
    }
  }

  // Now process the encoder data
  uint8_t currentEncoderValue = input_.getData()[kPush_buttonsDataSize];
  if (m_encoderValue != currentEncoderValue)
  {
    bool valueIncreased = ((m_encoderValue < currentEncoderValue) || 
      ((m_encoderValue == 0x0f) && (currentEncoderValue == 0x00)))
        && (!((m_encoderValue == 0x0) && (currentEncoderValue == 0x0f)));
      encoderChanged(Device::Encoder::Main, valueIncreased, shiftPressed);
    m_encoderValue = currentEncoderValue;
  }
}

//--------------------------------------------------------------------------------------------------

void Push::processPads(const Transfer& input_)
{
  //!\todo process pad data
  for (int i = 1; i <= kPush_padDataSize; i += 2)
  {
    uint16_t l = input_[i];
    uint16_t h = input_[i + 1];
    uint8_t pad = (h & 0xF0) >> 4;
    m_padsRawData[pad].write(((h & 0x0F) << 8) | l);
    m_padsAvgData[pad] = (((h & 0x0F) << 8) | l);

    Device::Pad btn(Device::Pad::Unknown);

#define M_PAD_CASE(value, pad) \
  case value:                  \
    btn = Device::Pad::pad; \
    break

    switch (pad)
    {
      M_PAD_CASE(0, Pad13);
      M_PAD_CASE(1, Pad14);
      M_PAD_CASE(2, Pad15);
      M_PAD_CASE(3, Pad16);
      M_PAD_CASE(4, Pad9);
      M_PAD_CASE(5, Pad10);
      M_PAD_CASE(6, Pad11);
      M_PAD_CASE(7, Pad12);
      M_PAD_CASE(8, Pad5);
      M_PAD_CASE(9, Pad6);
      M_PAD_CASE(10, Pad7);
      M_PAD_CASE(11, Pad8);
      M_PAD_CASE(12, Pad1);
      M_PAD_CASE(13, Pad2);
      M_PAD_CASE(14, Pad3);
      M_PAD_CASE(15, Pad4);
    }

#undef M_PAD_CASE

    if (m_padsAvgData[pad] > 1000)
    {
      padChanged(btn, m_padsAvgData[pad], m_buttonStates[static_cast<uint8_t>(Button::Shift)]);
    }    
  }
}

//--------------------------------------------------------------------------------------------------

void Push::setLedImpl(Led led_, const util::LedColor& color_)
{
  uint8_t ledIndex = static_cast<uint8_t>(led_);

  if(Led::Unknown == led_)
  {
    return;
  }

  if (isRGBLed(led_))
  {
    uint8_t currentR = m_leds[ledIndex];
    uint8_t currentG = m_leds[ledIndex + 1];
    uint8_t currentB = m_leds[ledIndex + 2];

    m_leds[ledIndex] = color_.getRed();
    m_leds[ledIndex + 1] = color_.getGreen();
    m_leds[ledIndex + 2] = color_.getBlue();

    m_isDirtyLeds = m_isDirtyLeds ||
     (currentR != color_.getRed() || currentG != color_.getGreen() || currentB != color_.getBlue());
  }
  else
  {
    uint8_t currentVal = m_leds[ledIndex];
    uint8_t newVal = color_.getMono();

    m_leds[ledIndex] = newVal;
    m_isDirtyLeds = m_isDirtyLeds || (currentVal != newVal);
  }
}

//--------------------------------------------------------------------------------------------------

bool Push::isRGBLed(Led led_) const noexcept
{
  if (Led::Group == led_ || Led::Pad1  == led_ || Led::Pad2  == led_ || Led::Pad3  == led_ ||
      Led::Pad4  == led_ || Led::Pad5  == led_ || Led::Pad6  == led_ || Led::Pad7  == led_ ||
      Led::Pad8  == led_ || Led::Pad9  == led_ || Led::Pad10 == led_ || Led::Pad11 == led_ ||
      Led::Pad12 == led_ || Led::Pad13 == led_ || Led::Pad14 == led_ || Led::Pad15 == led_ || 
      Led::Pad16 == led_
  )
  {
    return true;
  }

  return false;
}

//--------------------------------------------------------------------------------------------------

Push::Led Push::getLed(Device::Button btn_) const noexcept
{
#define M_LED_CASE(idLed)     \
  case Device::Button::idLed: \
    return Led::idLed

  switch (btn_)
  {
    M_LED_CASE(F1);
    M_LED_CASE(F2);
    M_LED_CASE(F3);
    M_LED_CASE(Control);
    M_LED_CASE(Nav);
    M_LED_CASE(BrowseLeft);
    M_LED_CASE(BrowseRight);
    M_LED_CASE(Main);
    M_LED_CASE(Group);
    M_LED_CASE(Browse);
    M_LED_CASE(Sampling);
    M_LED_CASE(NoteRepeat);
    M_LED_CASE(Restart);
    M_LED_CASE(TransportLeft);
    M_LED_CASE(TransportRight);
    M_LED_CASE(Grid);
    M_LED_CASE(Play);
    M_LED_CASE(Rec);
    M_LED_CASE(Erase);
    M_LED_CASE(Shift);
    M_LED_CASE(Scene);
    M_LED_CASE(Pattern);
    M_LED_CASE(PadMode);
    M_LED_CASE(View);
    M_LED_CASE(Duplicate);
    M_LED_CASE(Select);
    M_LED_CASE(Solo);
    M_LED_CASE(Mute);
    default:
    {
      return Led::Unknown;
    }
  }

#undef M_LED_CASE
}

//--------------------------------------------------------------------------------------------------

Push::Led Push::getLed(Device::Pad pad_) const noexcept
{
#define M_PAD_CASE(idPad)     \
  case Device::Pad::idPad: \
    return Led::idPad

  switch (pad_)
  {
    M_PAD_CASE(Pad13);
    M_PAD_CASE(Pad14);
    M_PAD_CASE(Pad15);
    M_PAD_CASE(Pad16);
    M_PAD_CASE(Pad9);
    M_PAD_CASE(Pad10);
    M_PAD_CASE(Pad11);
    M_PAD_CASE(Pad12);
    M_PAD_CASE(Pad5);
    M_PAD_CASE(Pad6);
    M_PAD_CASE(Pad7);
    M_PAD_CASE(Pad8);
    M_PAD_CASE(Pad1);
    M_PAD_CASE(Pad2);
    M_PAD_CASE(Pad3);
    M_PAD_CASE(Pad4);
    default:
    {
      return Led::Unknown;
    }
  }

#undef M_PAD_CASE
}

//--------------------------------------------------------------------------------------------------

Device::Button Push::getDeviceButton(Button btn_) const noexcept
{
#define M_BTN_CASE(idBtn) \
  case Button::idBtn:     \
    return Device::Button::idBtn

  switch (btn_)
  {
    M_BTN_CASE(F1);
    M_BTN_CASE(F2);
    M_BTN_CASE(F3);
    M_BTN_CASE(Control);
    M_BTN_CASE(Nav);
    M_BTN_CASE(BrowseLeft);
    M_BTN_CASE(BrowseRight);
    M_BTN_CASE(Main);
    M_BTN_CASE(Group);
    M_BTN_CASE(Browse);
    M_BTN_CASE(Sampling);
    M_BTN_CASE(NoteRepeat);
    M_BTN_CASE(Restart);
    M_BTN_CASE(TransportLeft);
    M_BTN_CASE(TransportRight);
    M_BTN_CASE(Grid);
    M_BTN_CASE(Play);
    M_BTN_CASE(Rec);
    M_BTN_CASE(Erase);
    M_BTN_CASE(Shift);
    M_BTN_CASE(Scene);
    M_BTN_CASE(Pattern);
    M_BTN_CASE(PadMode);
    M_BTN_CASE(View);
    M_BTN_CASE(Duplicate);
    M_BTN_CASE(Select);
    M_BTN_CASE(Solo);
    M_BTN_CASE(Mute);
    M_BTN_CASE(MainEncoder);
    default:
    {
      return Device::Button::Unknown;
    }
  }

#undef M_LED_CASE
}

//--------------------------------------------------------------------------------------------------

bool Push::isButtonPressed(Button button_) const noexcept
{
  uint8_t buttonPos = static_cast<uint8_t>(button_);
  return ((m_buttons[buttonPos >> 3] & (1 << (buttonPos % 8))) != 0);
}

//--------------------------------------------------------------------------------------------------

bool Push::isButtonPressed(
  const Transfer& transfer_, 
  Button button_
) const noexcept
{
  uint8_t buttonPos = static_cast<uint8_t>(button_);
  return ((transfer_[1 + (buttonPos >> 3)] & (1 << (buttonPos % 8))) != 0);
}

//--------------------------------------------------------------------------------------------------

} // devices
} // kio
} // sl