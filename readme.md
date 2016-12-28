# General Info
This project is aimed at using an XSens MTw Sensor for controling a mouse on a PC.
More information about XSens MTw:
https://www.xsens.com/products/mtw-development-kit-lite/mtw/

# Using without installing VisualStudio
- Download and install MTw Software Suite for Drivers from https://www.xsens.com/mt-software-suite-mtw-awinda/
- Download XSensMouse.zip from here: https://github.com/klues/XSensMouse/blob/master/Examples/xda_c_cpp/bin/XSensMouse.zip
- Extract the XSensMouse.zip to a folder
- Run XSensMouse.exe (tested and works on Win10, 64bit)

# Using VisualStudio to compile and develop
- Install Microsoft VisualStudio
- open dll_example_mtw_cpp.vcxproj ( https://github.com/klues/XSensMouse/blob/master/Examples/xda_c_cpp/dll_example_mtw_cpp.vcxproj )

# Example Files
**General:**

XYZ: x;y;z - coordinates

Euler: Roll;Pitch;Yaw

**Content of example files (.csv):**
- **left-right:** XSens was moved left-right several times (on a plane table)
- **front-back:** XSens was moved forward and back several times (on a plane table)
- **up-down:** XSens was lifted up and put back down several times
- **circle:** XSens was moved in a circle several times (on a plane table)
