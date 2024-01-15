# Escendo-Hackathon-2024

![Susstainable & Inclusive Technology](https://img.shields.io/badge/Susstainable%20&%20Inclusive%20Technology-8CE67F?style=for-the-badge&logoColor=white)
![IoT](https://img.shields.io/badge/IoT%20-1976D2?style=for-the-badge&logoColor=white)
![Micron Technology](https://img.shields.io/badge/Micron%20Technology-C71A36?style=for-the-badge&logoColor=white)
![Espressif](https://img.shields.io/badge/espressif-E7352C.svg?style=for-the-badge&logo=espressif&logoColor=white)
![AWS](https://img.shields.io/badge/AWS-%23FF9900.svg?style=for-the-badge&logo=amazon-aws&logoColor=white)
![C++](https://img.shields.io/badge/c++-%2300599C.svg?style=for-the-badge&logo=c%2B%2B&logoColor=white)

This repository contains the relevant code and prototype for the Escendo Hackathon 2024, hosted by NTU garage@EEE, a student-led maker space in the School of Electrical and Electronic Engineering.
<br>

This project is the recipient of Micron Sponsor's Choice Award.

---
<br>
<img src="https://github.com/J0JIng/Escendo-Hackathon-2024/blob/main/doc/UN_17_SDG.png" alt="UN_17_SDG" width="9000" height="500">


### Theme:
The theme for this year's hackathon is "Sustainability in Action," aligning with the UN 17 sustainability development goals and emphasising progressivity, environmentality, and well-being. Participants are required to formulate a problem statement related to this theme and devise the corresponding solution.
  
### Overview & Problem Statement:
Migrant workers are exposed to the extreme heat on a daily basis and regulations do not favor them as their heat stress is determined by employers solely. How can we create technology that protects the more vulnerable populations in the world such as migrant workers from the extreme heat we are experiencing currently and preserves their rights in the process?

### Solution - Tipping Hat:
We developed a lightweight IoT device equipped with sensors to measure both the Wet Bulb Globe Temperature (WBGT) and core body temperature of workers. This device would then be mounted on tha back of the worker's hard hat. When these measurements surpass a predefined threshold set by the Ministry of Manpower, the device triggers visual and auditory alerts using a buzzer and a flashing red LED. The worker acknowledges the alert by using a touch sensor, prompting an immediate halt to work activities. This not only enhances worker safety but also empowers them in the process. As additional safety measures, we incorporated GPS tracking and a localized group warning system. In the event of heat stress, this system alerts all workers within the same group, promoting collective well-being.

### Prototype & implementation:
<br>
<div>
    <img src="https://github.com/J0JIng/Escendo-Hackathon-2024/blob/main/doc/Master.jpg" alt="UN_17_SDG" width="400" height="306">
  <img src="https://github.com/J0JIng/Escendo-Hackathon-2024/blob/main/doc/Slave_A.jpg" alt="UN_17_SDG" width="400">
</div>

Our team successfully created a prototype, depicted above. In this system, the Master unit(M) receives data collected by the Slave unit(A), then efficiently transmits this information to the cloud through AWS IoT. Once in the cloud, the data undergoes processing to derive valuable insights and analysis. 

<br>
<img src="https://github.com/J0JIng/Escendo-Hackathon-2024/blob/main/doc/Device_connection.png" alt="UN_17_SDG" width="900" height="500">



