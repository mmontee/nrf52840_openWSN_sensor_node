# nrf52840_openWSN_sensor_node</br>

For this project you are expected to have: </br>
XIAO nrf52840 Development board </br>
Nordic Semi nrf52840-DK </br>
SEEED STUDIO EXPANSION BASE XIAO or the ability to solder small jumper to the XIAO nrf52840 </br>


This project's goal is to implement an acoustic sensor node using the XIAO nrf52840 and openWSN. </br>

Install Segger Embedded Studio for ARM (legacy) V6.30. Link - https://www.segger.com/downloads/embedded-studio/#ESforARM </br>

![alt text](images/seggerIDE.png) </br>

Clone the openWSN openwsn-fw Link - https://github.com/openwsn-berkeley/openwsn-fw </br>

We want a specific branch "develop_FW-893" </br>

![alt text](images/openwsnBranch.png) </br>

To open the prioject in Segger Embedded Studio from the root folder if the repo navigate to openwsn-fw/projects/nrf52840-DK/  </br>

Open the projet file "nrf52840_dk.emProject". </br>

To program the XIAO board first the nrf52840-DK must be confugured for the task. </br>

Jump these two pins, Then using jumoper wires connect pinA to pinA then pinB to pinB as shown. </br>

Compile -> Build-Build 03oos_openwsn</br>

Program -> Target - Connect J-Link, Then Target - Download 03oos_openwsn</br>

