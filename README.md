# Urano - former Thread and Boosting Frequency Throttling (TBFT)

Urano is an easy to use OpenMP framework that is completely transparent to both the developer and the end-user. Without any recompilation, code modification or OS installitions, it is capable of finding, automatically and at run-time, the optimal number of threads for the parallel regions and at the same time controls the turbo boosting mode to optimize different metrics, including Performance, Energy-Delay Product, Power Consumption and Temperature. 

### List of files contained in Urano
---

* boost.sh              -  Urano algorithm to control the turbo boosting techniques.
* urano.c               -  Urano functions implementation
* urano.h               -  Urano header
* urano_stats.h         -  Urano state machine headers
* env.c                 -  OpenMP internal controler variables
* libgomp.h             -  libgomp header
* libgomp_g.h           -  libgomp header
* parallel.c            -  libgomp header
* Makefile.in           -  OpenMP libgomp makefile.in
* Makefile.am           -  OpenMP libgomp makefile.am


### Urano's dependencies.

1. MSR Module (only in AMD Version).
2. Intel RAPL (only in Intel Version).
3. GCC 9.4+.

### How to install Urano?
---

1. Choose the version you are going to use based on your processor (Intel or AMD).
2. Copy all files into the gcc libgomp directory:
      - cp processor_headers/* /path/gcc-version/libgomp.
      - cp src/* /path/gcc-version/libgomp.
3. Compile the GCC using Make && Make install:
      - cd /path/gcc-version/libgomp
      - make
      - make install


### How to use Urano?
---

1. Export the library path, the full boost.sh path and set Urano's environment variable:
      - export LD_LIBRARY_PATH=/path-to-gcc-bin/lib64:$LD_LIBRARY_PATH
      - export OMP_URANO_BOOST_PATH=/gcc-version/libgomp
      - export OMP_URANO=METRIC
      
2. Execute the application.

3. (OPTIONAL) Export the IPC Threshold and Time Interval for the Turbo Engine.
      - export IPC_TARGET=0.6 (Can be any value in the format %d.%d)
      - export TIME_TARGET=1 (Can be any value in the format %d)
x
### Acknowledgement
---

Urano/TBFT has been mainly developed by Sandro Matheus Vila Nova Marques (sandro-matheus@hotmail.com) during his BSc/MSc. under supervision of Arthur Francisco Lorenzon (aflorenzon@unipampa.edu.br).

When using Urano/TBFT, please use the following reference:

XXXXXXX
