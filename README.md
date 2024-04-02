# HQoS policies 
This repository hosts the ns-3 implementation of Differentiated Services Code Point (DSCP). Its modeling aims to enable performance evaluations of network slices with varying priority levels, as well as the convergence of technologies including backhaul and fronthaul.

## Building ns-3
```
./ns3 configure --disable-werror --disable-python --disable-tests --disable-examples --build-profile=release
./ns3 build
```
## Project Structere
- The folder [_scratch_](./ns-allinone-3.39/ns-3.39/scratch/) folder contains a generic scenario ([_juniper-setupnewlinks.cc_](./ns-allinone-3.39/ns-3.39/scratch/juniper-setupnewlinks.cc)) to evaluate HQoS as well as the JSON files to configurate traffic flows from backhaul and fronthaul.

- The [_scripts_](./ns-allinone-3.39/ns-3.39/scripts/) folder contains scripts needed to run different topologies. It also contains a Jupyter notebook to represent the outcomes.

## HQoS Skills:
- [Traffic-Control](https://www.nsnam.org/docs/models/html/traffic-control.html) The Traffic Control layer sits in between the NetDevices (L2) and any network protocol (e.g. IP). It is in charge of processing packets and performing actions on them: scheduling, dropping, marking, policing, etc. The code is available in [_traffic-control_](./ns-allinone-3.39/ns-3.39/src/traffic-control/) folder.


## Scenario
- [Runscen](./ns-allinone-3.39/ns-3.39/scripts/runscen.py)  is a script designed to conduct a sweep over the channel capacity using the selected JSON scenario from the [_scratch_](./ns-allinone-3.39/ns-3.39/scratch/) directory.

## Plots generation with Jupyter NoteBook
- [Analysis](./ns-allinone-3.39/ns-3.39/scripts/Analysis.ipynb) contains snipts of code extract the outcomes from datafiles generating in simulation with the use of the script.

__Disclaimer__: figures in manuscrip are generated with Tikz.

## NS-3 Documentation

-  [Manual](https://www.nsnam.org/docs/release/3.39/manual/html/index.html) 
-  [Model-Library](https://www.nsnam.org/docs/release/3.39/models/html/index.html)


## :envelope_with_arrow:  Contact 
* [Fátima Khan Blanco (khanf@unican.es)](mailto:khanf@unican.es)
* [Luis Diez (diezlf@unican.es)](mailto:diezlf@unican.es)
* [Ramón Agüero (agueroc@unican.es)](mailto:agueroc@unican.es)