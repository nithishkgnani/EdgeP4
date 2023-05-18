# EdgeP4

Repository for the EdgeP4 code

## Cyber-Pysical System Testbed

<img src="media/Testbed-wide-NoBorder.png" width="auto">

The devices run the code found in the following directories:  
1. Host 1: [Host1-Operator](Host1-Operator/) - Runs the code for Geomagic touch. Uses OpenHaptics and C programming.
2. Host 2: [Host2-Teleoperator](Host2-Teleoperator/) - Runs the code for UR3 root. Uses ROS 1 and Python
3. Edge Switch 1: [Tremor_Reduction](Tremor_Reduction/) - Runs the code to eliminate packets linked to unintenetional hand tremors. Uses P4 for SmartNIC.
4. Edge Switch 2: [Grip_Correction](Grip_Correction/) - Runs the code to perform automatic grip correction while holding a tool. Uses P4 for SmartNIC.
