# Digital_Oscilloscope
Digital Oscilloscope designed for EK-TM4C1294XL Board with LCD and Desktop App

Project is designed to work as a digital oscilloscope with the following features:
- LCD Touchscreen: Touch input depends on menu screen. Option selected is updated in internal firmware state and synchronised with software application
- Software Application: Packets sent/received over Ethernet to update internal state. Application interface includes plots of oscilloscope input on time graph.  (Note: Application code not included)
- ADC: 2 ADC modules designed to use uDMA to sample and store @ 1M samples/sec when a trigger event occurs.
- Triggers: Rising Edge, Falling Edge, Level in Continuous, Single and Auto Modes.
- Wave Generator: Output on single GPIO port. Wave forms incl. Square, Ramp, Triangle, Sine


![LCD screen](https://cloud.githubusercontent.com/assets/9069253/16540464/26bbbaaa-40a9-11e6-9c10-241508ad1747.jpg?raw=true "Title")
![LCD screen](https://cloud.githubusercontent.com/assets/9069253/16540471/50f3a1c0-40a9-11e6-84c1-ec484b2eb845.jpg?raw=true "Title")
