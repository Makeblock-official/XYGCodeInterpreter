GCode Interpreter For XY-Plotter
==================
How To Use:

1.Download XYPlotter_GCode, unzip and upload the code to your arduino board.

2.Install the GCode GUI Application that supports windows (XYCodeGUI_For_Win_Installer ) and mac osx (XYCodeGUI_For_Mac_Installer). (Base on Adobe AIR)

3.Open the GCode GUI

4.Choose and Connect the Arduino Serial Port.

5.Send Your GCode on the App's Panel,You can also load the txt file that contains GCodes(GCode_Demo)

GCode Example:

Linear interpolation: G1 X-100.5 Y90.4 Z1 (move 100.5 opposite on X-axis,move 90.4 on Y-axis and the brush will be down)

Circular interpolation:G2 I10 Z1 (it will be draw a circle that radius is 10)

Switch to absolute coordinates:G90

Switch to relative coordinates:G91

The GCode "Z" will control the brush,down for "Z1",up for "Z0".

About Gcode: http://reprap.org/wiki/G-code26
