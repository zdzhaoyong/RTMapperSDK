# Compile on Linux

Please use Ubuntu 16.04 or Mint 18 to develop the software. Other platform are also support, please install correct packages.

1. Install building tools
```
sudo apt-get install build-essential git cmake
```

2. Install Qt if you want build example with Qt
```
sudo apt-get install libqt4-dev libqt4-dev-bin
```

3. Compile use cmake:

```
mkdir build; cd build; cmake ..; make ;
```
