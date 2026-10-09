# (OXRWXR) OpenXR for WinlatorXR

**OXRWXR**, short for **OpenXR Runtime for WinlatorXR**, is a Windows-compatible OpenXR runtime designed to bring a desktop-style OpenXR feature set to standalone VR hardware such as Meta Quest and Pico headsets.

The runtime uses **XrAPI v0.6** as its interface layer and is specifically optimized for the requirements of **WinlatorXR**, allowing Windows applications and games running through WinlatorXR to communicate with the standalone headset's XR system through OpenXR.

OXRWXR is based on [OpenXR-Simulator](https://github.com/elliotttate/OpenXR-Simulator). The project was detached from the original repository because it now targets a substantially different use case. While OpenXR-Simulator provides a general-purpose simulation environment, OXRWXR focuses specifically on running Windows OpenXR applications on standalone Android-based VR hardware.

As part of this transition, a number of features from the original project that are not required by WinlatorXR were removed, while the runtime was adapted and extended around the needs of standalone VR. This results in a more focused implementation built specifically for the WinlatorXR environment and its goal of bringing PCVR software directly to standalone headsets without requiring a connected PC.
