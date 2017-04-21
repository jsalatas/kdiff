# KDiff

KDiff is a graphical difference viewer for the KDE platform, that allows you to visualize changes to a file.
 
It is based and inspired by the [Kompare Application](https://www.kde.org/applications/development/kompare/) and it actually uses a fork of libkomparediff2 for comparing files.

A major difference between Kompare and KDiff is that in KDiff the diff viewer component is based on [KDE's KTextEditor](https://api.kde.org/frameworks/ktexteditor/html/) allowing for (destination) file editing.
  
#### Build
`````
cd ~
git clone https://github.com/jsalatas/kdiff.git
cd kdiff
mkdir build
cd build
cmake  -DCMAKE_INSTALL_PREFIX=/usr ..
make
sudo make install
`````

##### Screenshot
![KDiff Screenshot](http://jsalatas.ictpro.gr/kdiff.png "KDiff Screenshot")


#### Reporting Bugs and Wishes 

Please report any bugs or wishes to [kdiff's issues page at github](https://github.com/jsalatas/kdiff/issues).

Pull requests are more than welcomed!


