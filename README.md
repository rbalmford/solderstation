# solderstation

Arduino-based soldering iron temperature controller.

It has been designed to control a 24v 50W iron, in my case an Antex TC50, although any iron with a thermocouple sensor should be suitable.

Features:

- Closed-loop temperature control, setting 25-450C (adjustable in code)
- IDLE mode maintains a lower temperature (set in code), also closed-loop
- OPEN loop mode with selectable constant power
- Display shows mode/set temperature, and current power level

The list of parts used is as follows:

- Arduino UNO R3 (any clone will do) + prototype shield/PCB
- MOSFET driver module (D4148)
- MAX6675 thermocouple interface
- 24v to 5v buck converter module
- 16x2 LCD display with serial backpack
- 3 buttons
- Connector for soldering iron
- Enclosure of your choice
- 24v 3A SMPS + socket

The controller starts in IDLE mode, set to 150C.
A short press of the mode button toggles between this and ON mode, where the temperature can be adjusted via the up/down buttons.
A long press of the mode button selects toggles between these and OPEN loop mode, where the up/down buttons adjusts the power in the sequence 0-1-2-4-8-15, where 15 = full power.

If there is no valid temperature reading, closed-loop modes will set the power to zero and display 'OOR' (Out Of Range). Open-loop mode will work without a valid temperature reading, either as a backup or when manual control is desired.

The default temperatures and ranges can be modified in the code, as can the pins used for the various interfaces (The MOSFET driver should stay on pin9 as this is hardwired to the timer used).

## Transient behaviour:

![transient_1](https://user-images.githubusercontent.com/6553778/143453168-3dc61ea8-c763-4397-8236-60717914a795.png)

![transient_2](https://user-images.githubusercontent.com/6553778/143453198-d638395d-837c-45df-9e90-21f2bc6f0eb5.png)

## Example Build

![20210131_172906](https://user-images.githubusercontent.com/6553778/143454776-65fbd97f-c260-48c5-9376-1b4d9c282a0d.png)

![20210115_145444](https://user-images.githubusercontent.com/6553778/143454797-7ce82806-ff72-40fe-8cbc-887099fc1e87.png)
