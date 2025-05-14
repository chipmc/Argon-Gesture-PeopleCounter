# Argon-Gesture-PeopleCounter

This is a simple program to demonstrate the power of Edge AI and IoT.  


## Introduction

The idea was to show how to make a simple demo and provide instructions that would make it "open" to collaboration and extension.

## Hardware

Hardware requirements are: 
- Particle Argon 
- DFRobot Gensture and Face Detection sensor - SEN0626
- I am using a See Insights carrier board to help with the mounting but this is optional


## Getting Started

Once connected to Particle, the Argon will "breathe cyan" on its RGB status light indcating a connection

The sensor will show a white light when a person (face and shoulders) is detected and the second RGB Led will show the following if a hand gesture is detected:
    Print the gesture detection results
    - 1: LIKE (👍) - blue
    - 2: OK (👌) - green
    - 3: STOP (🤚) - red
    - 4: YES (✌) - yellow
    - 5: SIX (🤙) - purple


## Interacting with the demo

1) In-person
2) Via Particle API
3) Via a Webhook from Particle
4) From the serial data streaming service - Ubidots
