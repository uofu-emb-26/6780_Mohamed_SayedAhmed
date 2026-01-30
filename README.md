This is a project template for ECE 5780, Embedded Systems.

# Install VSCode
Download and install Visual Studio Code.

https://code.visualstudio.com/

Install these VSCode extensions
https://marketplace.visualstudio.com/items?itemName=ms-vscode.cpptools-extension-pack

https://marketplace.visualstudio.com/items?itemName=stmicroelectronics.stm32-vscode-extension

https://marketplace.visualstudio.com/items?itemName=marus25.cortex-debug

# Toolchain installation
Students are recommended to work in the industry standard Unix environment for this course - this is a valuable skill you will need in your career.
If you wish, you may install Linux as your primary operating system, otherwise you can set up a virtual environment.
Mac OSX is a Unix variant, and can be used as the primary operating system.
Windows users can install the Windows Subsystem for Linux (WSL) which provides a Linux environment.
Tools are available for Windows, but not recommended.

# Linux installation
Linux can be installed as your primary operating system, dual boot with Windows or OSX, installed with the Windows Subsystem for Linux (WSL), or run in a virtual machine.

If you don't have a preferred distribution of Linux, Ubuntu or Mint are popular options. https://linuxmint.com/

For Debian based systems (including Ubuntu and Mint), you will need to install the following packages. You will need to run these commands as root using `sudo`.

```sudo apt install stlink-tools gcc-arm-none-eabi binutils-arm-none-eabi cmake make git openocd```

For other distributions, check your package manager for the appropriate installation.

## VirtualBox
VirtualBox creates a virtual machine, a full isolated computer environment with a separate operating system.

https://www.virtualbox.org

You will be working remotely connected to the virtual machine over ssh.

https://code.visualstudio.com/docs/remote/remote-overview

You will need to attach the USB stlink device to the virtual machine following these instructions:

https://www.virtualbox.org/manual/topics/BasicConcepts.html#usb-support

## WSL

The Windows Subsystem for Linux (WSL) allows installation of a Linux environment inside of Windows.

https://learn.microsoft.com/en-us/windows/wsl/install

Follow these instructions to connect VSCode to the WSL instance

https://learn.microsoft.com/en-us/windows/wsl/tutorials/wsl-vscode

You will need to attach the USB stlink device to the WSL environment following these instructions:

https://learn.microsoft.com/en-us/windows/wsl/connect-usb

# MacOS installation
You will need to install brew.

https://brew.sh/

From there, install the package dependencies.
```brew tap caskroom/cask
brew install cask gcc-arm-embedded
brew install openocd
brew install stlink
```
# Windows installation
We recommend that you use the Windows Subsystem for Linux (WSL). Instructions for installation are above.

If you still wish to work directly in Windows, you will need to install this additional software:

Install git

https://git-scm.com/downloads

Install PuTTY

https://www.chiark.greenend.org.uk/~sgtatham/putty/latest.html

The GNU build toolchain will be installed by the STM32Cube VSCode extension.

# STM32CubeMX
The MX tool is provided by ST to setup projects. You can try it out, but you will be using the template provided here.

https://www.st.com/en/development-tools/stm32cubemx.html#get-software

# Drivers
This repository contains libraries copied from https://github.com/STMicroelectronics/STM32CubeF0 at commit 165396863a295fe41640f721f8b8ba276572e083.
License information for these libraries is located inside library directories.
