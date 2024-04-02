# TEF-6GBlur



## Getting started

To make it work:
```
mkdir build
cd build
cmake ..
cd ..
./ns3 configure -d release --enable-examples --enable-tests
./nse build 
```

## All important modifications are made in:

- [ ] :hammer_and_wrench: [Traffic-Control](https://www.nsnam.org/docs/models/html/traffic-control.html) The Traffic Control layer sits in between the NetDevices (L2) and any network protocol (e.g. IP). It is in charge of processing packets and performing actions on them: scheduling, dropping, marking, policing, etc.
![My image](/docs/tfl.png)

## Documentation

- [ ] [Manual](https://www.nsnam.org/docs/release/3.39/manual/html/index.html) 
- [ ] [Model-Library](https://www.nsnam.org/docs/release/3.39/models/html/index.html)


