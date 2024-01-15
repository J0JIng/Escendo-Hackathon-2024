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

The prototype for this project was implemented on the ESP32/-S3 development board (shown above). The ESP-NOW proprietary protocol was used due to its improved latency and range over BLE [[1]](#1), and simplistic protocol stack as compared to Wi-Fi's OSI model [[2]](#2).
In this system, the Master unit(M) receives data collected by the Slave unit(A), then efficiently transmits this information to the cloud through AWS IoT. Once in the cloud, the data undergoes processing to derive valuable insights and analysis.

In order to ensure Wi-Fi/ESP-NOW concurrency on the Master unit, AP/STA mode was used, as well as automatic channel detection. FreeRTOS was used to facilitate asynchronous operation of the program logic.

<br>
<img src="https://github.com/J0JIng/Escendo-Hackathon-2024/blob/main/doc/Device_connection.png" alt="UN_17_SDG" width="900" height="500">

The image above provides an overview of the framework we have established. This schematic illustrates the connections and interactions between the Master unit, Slave unit, and the cloud, showcasing the flow of data within our solution.<br>

We note that the ESP-NOW protocol does not limit the number of peer-to-peer connections. The system could be scaled up (in network range) through use of repeater nodes, or propagating messages to form a mesh network.

## References
<a id="1">[1]</a> D. Eridani, A. F. Rochim and F. N. Cesara, "Comparative Performance Study of ESP-NOW, Wi-Fi, Bluetooth Protocols based on Range, Transmission Speed, Latency, Energy Usage and Barrier Resistance," 2021 International Seminar on Application for Technology of Information and Communication (iSemantic), Semarangin, Indonesia, 2021, pp. 322-328, doi: 10.1109/iSemantic52711.2021.9573246.

<br>
<a id="2">[2]</a> Urazayev, Dnislam & Eduard, Aida & Ahsan, Muhammad & Zorbas, Dimitrios. (2023). Indoor Performance Evaluation of ESP-NOW. 

