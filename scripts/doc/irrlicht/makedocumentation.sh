rm tut.txt || true;

mkdir ../../../doctemp
mkdir ../../../doctemp/html
cp doxygen.css irrlicht.png logobig.png ../../../doctemp/html

doxygen doxygen.cfg
