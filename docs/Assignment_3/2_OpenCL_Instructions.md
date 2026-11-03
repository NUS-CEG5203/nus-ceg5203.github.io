# OpenCL Configuration

!!! warning "Follow the Spirit"
    The instructions should be treated as a guideline rather than a very comprehensive manual. Figure out on your own how to solve issues, if any.

It is generally easier to run OpenCL in a Linux distribution. Instructions for Windows are also provided, but expect some rough edges.

Make sure your graphics drivers are up-to-date and supports OpenCL. The instructions assume an Intel GPU. For Nvidia and AMD GPUs, there are some differences in instructions, which you can figure out with a bit of Googling.

Use path names without special characters or spaces for all paths.

The template/data files are [here](https://github.com/NUS-CEG5203/assignments/tree/main/docs/Assignment_3/code_templates).

## Installing Build Tools and OpenCL

=== "Ubuntu Linux"

    Refresh package list and update packages. `sudo apt update && sudo apt upgrade`. You will be prompted for your Ubuntu password, and to confirm by pressing `y`.

    Install build tools, OpenCL libraries, and loader. `sudo apt install build-essential clinfo intel-opencl-icd ocl-icd-opencl-dev gdb` (assuming Intel GPU. For Nvidia / AMD, appropriate drivers may need to be installed).
    
    Depending on the system configuration, other dependencies may also need to be installed. `sudo apt -f install` can usually help install these additional dependencies.

    Before building your program for the first time, it may be a good idea to refresh the linker cache via `sudo ldconfig`.

    Run `clinfo` from the terminal and see if lists platforms and devices. If it lists platforms and devices, you are close! 
    
    If it shows number of platforms as 0, you likely have a new (e.g, Raptor Lake U) processor. You can follow the guidelines on [Intel website](https://dgpu-docs.intel.com/driver/client/overview.html#ubuntu-latest), reproduced below for convenience.

    Install the package for managing software repositories (likely already installed). `sudo apt-get install -y software-properties-common`.

    Add the intel-graphics PPA. `sudo add-apt-repository -y ppa:kobuk-team/intel-graphics`.

    Install Intel compute related packages. `sudo apt-get install -y libze-intel-gpu1 libze1 intel-metrics-discovery intel-gsc`

    Try `clinfo` again and it should show platforms and devices.

=== "Windows"

    *Install MSYS2*: Install [MSYS2](https://github.com/msys2/msys2-installer/releases/download/2025-08-30/msys2-x86_64-20250830.exe). This provides a minimal Linux-like setup for Windows.

    Run MSYS2. It takes a few seconds to open.

    *Install the compiler toolchain*: Via the MSYS2 command line, run the following.

    `pacman -Syu` to update the package manager.
    
    and then
    
    `pacman -S mingw-w64-ucrt-x86_64-gcc` to install the GCC compiler
    
    `pacman -S mingw-w64-ucrt-x86_64-gdb` to install the GNU debugger.

    By default, the toolchain is installed to C:\msys64\ucrt64. If you want to know the location, you can find out by typing `which g++`.

    *Add the toolchain to Windows PATH*: Win+S > search for 'Environment Variables' > Edit system environment variables > Advanced Tab > Environment Variables > System Variables > Path > Edit > New >. Add the bin folder of your UCRT64 installation, e.g.: `C:\msys64\ucrt64\bin and C:\msys64\ucrt64\include`. Click OK to save changes.

    *Verify Installation*: Open Command Prompt (in start menu, type cmd) and run: g++ --version
    If installed correctly, it should display version information. Only new command windows can see the updated path, so if it was already open, close and reopen.

    *Install OpenCL SDK*: Download and Unzip [Khronos OpenCL SDK](https://github.com/KhronosGroup/OpenCL-SDK/releases/download/v2025.07.23/OpenCL-SDK-v2025.07.23-Win-x64.zip) (a newer release, if available, should be fine too).

    Unzip to `C:\OpenCLSDK` such that `C:\OpenCLSDK\include` (contains CL/cl.h etc.) and `C:\OpenCLSDK\bin`(contains OpenCL.lib) exists. If you choose another path, modify command line build options and/or edit the VS Code config files accordingly.

## Configuring Visual Studio Code IDE

Install VS Code IDE. This can be done in multiple different ways depending on the OS and package manager.

Install the following extensions

* C/C++
* C/C++ Extension Pack
* CMake Tools (optional)
* OpenCL (optional, allows syntax highlighting of .cl files, has a built-in `clinfo` tool, etc.)

You can use the folder/workspace from [here](https://nus-ceg5203.github.io/assignments/Assignment_3/code_templates/OpenCLExample). Include the complete folder, including the .vscode subfolder with the .json files that has various settings. Make appropriate changes to the paths in the .json files if necessary. The source file is slightly enhanced from the one explained in lecture notes. This file has enhanced debugging options, as well as time measurement-related functions.

Build the project via Ctrl+Shift+B. This will create the executable `main.out` (Linux) or `main.exe` (Windows).

Ensure that `kernel.cl` is in the same folder as the executable `main.out` or `main.exe`

Press F5 to run it with debugging.

To run normally, go to the terminal of VS Code, ensure that you are in the folder with the `main.exe` / `main.out` (`cd` there if need be), and execute `.\main.exe` or `./main.out`.

Running in Windows is less stable, there is a higher likelihood of the runtime compilation of the kernel failing. Just run again if that happens.

## Compiling and Running from Command Line

This is optional, but can be faster than using an IDE. You can use the `main.cpp` from the archives above.

Build the program via

=== "Linux"

    `g++ main.cpp -lOpenCL -o main.out`

=== "Windows"

    `g++ -g main.cpp -IC:/OpenCLSDK/include -LC:/OpenCLSDK/lib -lOpenCL -o main.exe`

after doing a `cd` to the folder/directory with the source file. This will create the executable `main.out` or `main.exe`.

Copy `kernel.cl` from the archives above to be in the same folder as the executable `main.out` or `main.exe`, as it is read and compiled at runtime. You can now run the executable by entering the command `/main.out` or `.\main.exe`. It should print the result 2-10 seconds later. If it works, you are good to go!

If using `gcc` instead of `g++` and if you include `math.h`, `-lm` should be added to command line or VS Code `tasks.json`.

## OpenCL on CPU using PoCL

[Portable Computing Language (PoCL)](https://portablecl.org/) provides a portable OpenCL implementation that works with most CPUs and many GPUs. On CPUs, it can make use of all the cores/hardware threads. Installation in Windows is not straightforward, but possible.

In Linux, you can install it via `sudo apt install pocl-opencl-icd`. 

Make slight adjustments to the code, by selecting the correct platform and device type.

It works on the Kria board too if you boot Linux, and install PoCL. More instructions in [this](3_Pynq_OpenCL_Kria.md#opencl-on-kria) page.
