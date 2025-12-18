## Requirements
This project uses CMake workflow presets and so it requires at least CMake version 3.25.
Ubuntu 22.04 and earlier Ubuntu versions require manually updating CMake to get version 3.25 or later.  It is possible to use an older version of CMake if the workflow presets are not used.

## Usage
Run CMake from the command line using the workflow presets to configure and build the project (inside the actual project directory) with
~~~
cmake --workflow --preset workflow_<TYPE>
~~~

Run CMake from the command line without using the workflow preset:
~~~
cmake --preset <TYPE> && cmake --build --preset build_<TYPE>
~~~

The "<TYPE>" can be either "release" or "debug".

For a single project, run the commands from the PROJECTNAME_cmake folder.  
For a solution run the command from the SOLUTIONNAME_cmake folder.

## User Source Files
User source files added to the project can be included in the build configuration by editing the CMakeLists.txt file in the sections indicated by comments in that file or by creating a .cmake file and including that file from CMakeLists.txt.
